/*

imap.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/
 

#include <algorithm>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/regex.hpp>
#include <mailio/imap.hpp>


using std::find_if;
using std::invalid_argument;
using std::list;
using std::make_optional;
using std::make_shared;
using std::make_tuple;
using std::map;
using std::move;
using std::out_of_range;
using std::pair;
using std::shared_ptr;
using std::stoul;
using std::string;
using std::stringstream;
using std::to_string;
using std::tuple;
using std::vector;
using std::chrono::milliseconds;
using boost::system::system_error;
using boost::iequals;
using boost::regex;
using boost::regex_match;
using boost::smatch;
using boost::split;
using boost::trim;
using boost::algorithm::trim_copy_if;
using boost::algorithm::trim_if;
using boost::algorithm::is_any_of;


namespace mailio
{


string imap::search_condition_t::id_range_to_string(imap::search_condition_t::id_range_t id_pair)
{
    return to_string(id_pair.first) + (id_pair.second.has_value() ? ":" + to_string(id_pair.second.value()) : "");
}


imap::search_condition_t::search_condition_t(imap::search_condition_t::key_type condition_key, imap::search_condition_t::value_type condition_value) :
    key(condition_key), value(condition_value)
{
    try
    {
        switch (key)
        {
            case ALL:
                imap_string = "ALL";
                break;

            case ID_LIST:
            {
                imap_string = boost::join(std::get<list<id_range_t>>(value) | boost::adaptors::transformed(static_cast<string(*)(id_range_t)>(id_range_to_string)), ",");
                break;
            }

            case SUBJECT:
                imap_string = "SUBJECT \"" + std::get<string>(value) + "\"";
                break;

            case FROM:
                imap_string = "FROM \"" + std::get<string>(value) + "\"";
                break;

            case TO:
                imap_string = "TO \"" + std::get<string>(value) + "\"";
                break;

            case BEFORE_DATE:
                imap_string = "BEFORE " + imap_date_to_string(std::get<boost::gregorian::date>(value));
                break;

            case ON_DATE:
                imap_string = "ON " + imap_date_to_string(std::get<boost::gregorian::date>(value));
                break;

            case SINCE_DATE:
                imap_string = "SINCE " + imap_date_to_string(std::get<boost::gregorian::date>(value));
                break;

            default:
                break;
        }
    }
    catch (std::bad_variant_access&)
    {
        throw imap_error("Invaid search condition.");
    }
}


string imap::tag_result_response_t::to_string() const
{
    string result_s;
    if (result.has_value())
    {
        switch (result.value())
        {
        case OK:
            result_s = "OK";
            break;

        case NO:
            result_s = "MO";
            break;

        case BAD:
            result_s = "BAD";
            break;

        default:
            break;
        }
    }
    else
        result_s = "<null>";
    return tag + " " + result_s + " " + response;
}


imap::imap(const string& hostname, unsigned port, milliseconds timeout) :
    _dlg(new dialog(hostname, port, timeout)), _tag(0), _optional_part_state(false), _atom_state(atom_state_t::NONE),
    _parenthesis_list_counter(0), _literal_state(string_literal_state_t::NONE), _literal_bytes_read(0), _eols_no(2)
{
}


imap::~imap()
{
    try
    {
        _dlg->send(format("LOGOUT"));
    }
    catch (...)
    {
    }
}


void imap::authenticate(const string& username, const string& password, auth_method_t method)
{
    connect();
    if (method == auth_method_t::LOGIN)
        auth_login(username, password);
}


auto imap::select(const list<string>& folder_name, bool read_only) -> mailbox_stat_t
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    return select(folder_name_s);
}


void imap::fetch(const string& mailbox, unsigned long message_no, message& msg, bool header_only)
{
    select(mailbox);
    fetch(message_no, msg, header_only);
}


// Fetching literal is the only place where line is ended with LF only, instead of CRLF. Thus, `receive(true)` and counting EOLs is performed.
void imap::fetch(unsigned long message_no, message& msg, bool header_only, bool is_uid)
{
    const string RFC822_TOKEN = string("RFC822") + (header_only ? ".HEADER" : "");

    string cmd;
    if (is_uid)
        cmd.append("UID ");
    cmd.append("FETCH " + to_string(message_no) + " " + RFC822_TOKEN);
    _dlg->send(format(cmd));
    
    bool has_more = true;
    while (has_more)
    {
        reset_response_parser();
        string line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);
        
        if (parsed_line.tag == "*")
        {
            parse_response(parsed_line.response);

            auto msg_no_token = _mandatory_part.front();
            if (msg_no_token->token_type != response_token_t::token_type_t::ATOM || stoul(msg_no_token->atom) != message_no)
                throw imap_error("Fetching message failure.");
            _mandatory_part.pop_front();

            auto fetch_token = _mandatory_part.front();
            if (fetch_token->token_type != response_token_t::token_type_t::ATOM || !iequals(fetch_token->atom, "FETCH"))
                throw imap_error("Fetching message failure.");
            _mandatory_part.pop_front();

            shared_ptr<response_token_t> literal_token = nullptr;
            if (!_mandatory_part.empty())
                for (auto part : _mandatory_part)
                {
                    if (part->token_type == response_token_t::token_type_t::LIST)
                    {
                        bool rfc_found = false;
                        for (auto token = part->parenthesized_list.begin(); token != part->parenthesized_list.end(); token++)
                        {
                            if ((*token)->token_type == response_token_t::token_type_t::ATOM && iequals((*token)->atom, RFC822_TOKEN))
                            {
                                rfc_found = true;
                                continue;
                            }
                            
                            if ((*token)->token_type == response_token_t::token_type_t::LITERAL)
                            {
                                if (rfc_found)
                                {
                                    literal_token = *token;
                                    break;
                                }
                                else
                                    throw imap_error("Parsing failure.");
                            }
                        }
                    }
                }

            if (literal_token != nullptr)
            {
                // loop to read string literal
                while (_literal_state == string_literal_state_t::READING)
                {
                    string line = _dlg->receive(true);
                    if (!line.empty())
                        trim_eol(line);
                    parse_response(line);
                }
                // closing parenthesis not yet read
                if (_literal_state == string_literal_state_t::DONE && _parenthesis_list_counter > 0)
                {
                    string line = _dlg->receive(true);
                    if (!line.empty())
                        trim_eol(line);
                    parse_response(line);
                }
                msg.parse(literal_token->literal);
            }
            else
                throw imap_error("Parsing failure.");
        }
        else if (parsed_line.tag == to_string(_tag))
        {
            if (parsed_line.result.value() == tag_result_response_t::OK)
            {
                has_more = false;
            }
            else
                throw imap_error("Fetching message failure.");
        }
        else
            throw imap_error("Parsing failure.");
    }

    reset_response_parser();
}


void imap::fetch(const string& message_nos, map<unsigned long, message>& msgs, codec::line_len_policy_t line_policy, bool header_only, bool is_uids)
{
    const string RFC822_TOKEN = string("RFC822") + (header_only ? ".HEADER" : "");

    string cmd;
    if (is_uids)
        cmd.append("UID ");
    cmd.append("FETCH " + message_nos + " " + RFC822_TOKEN);
    _dlg->send(format(cmd));
    
    bool has_more = true;
    while (has_more)
    {
        reset_response_parser();
        string line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);
        
        if (parsed_line.tag == "*")
        {
            //if (!iequals(std::get<2>(tag_result_response), "FETCH"))
            //    throw imap_error("Fetching message failure.");

            parse_response(parsed_line.response);
            // Grab the message number, if we asked for them instead of uids.
            unsigned long msg_no = 0;
            if (!is_uids)
            {
                // TODO: Is the failure condition correct?
                // TODO: Check if it's atom at all.
                msg_no = stoul(_mandatory_part.front()->atom);
                _mandatory_part.pop_front();
                if (msg_no == 0)
                    throw imap_error("Fetching message failure.");
            }

            if (!iequals(_mandatory_part.front()->atom, "FETCH"))
                throw imap_error("Fetching message failure.");

            unsigned long uid = 0;
            shared_ptr<response_token_t> literal_token = nullptr;
            if (!_mandatory_part.empty())
                for (auto part : _mandatory_part)
                {
                    if (part->token_type == response_token_t::token_type_t::LIST)
                    {
                        bool key_found = false;
                        string key;

                        for (auto token = part->parenthesized_list.begin(); token != part->parenthesized_list.end(); token++)
                        {
                            if ((*token)->token_type == response_token_t::token_type_t::ATOM)
                            {
                                const string& atm = (*token)->atom;
                                if (key_found)
                                {
                                    if(iequals(key, "UID"))
                                    {
                                        uid = stoul(atm);
                                    }
                                    // Reset.
                                    key_found = false;
                                }
                                else
                                {
                                    key = atm;
                                    key_found = true;
                                }
                            }
                            else if ((*token)->token_type == response_token_t::token_type_t::LITERAL)
                            {
                                if (key_found)
                                {
                                    if (iequals(key, RFC822_TOKEN))
                                    {
                                        literal_token = *token;
                                        // Break because reading the string literal text should follow.
                                        break;
                                    }
                                    key_found = false;
                                }
                                // Anything else is an error.
                                throw imap_error("Parsing failure.");
                            }
                        }
                    }
                }

            if (literal_token != nullptr)
            {
                // loop to read string literal
                while (_literal_state == string_literal_state_t::READING)
                {
                    string line = _dlg->receive(true);
                    if (!line.empty())
                        trim_eol(line);
                    parse_response(line);
                }
                // closing parenthesis not yet read
                if (_literal_state == string_literal_state_t::DONE && _parenthesis_list_counter > 0)
                {
                    string line = _dlg->receive(true);
                    if (!line.empty())
                        trim_eol(line);
                    parse_response(line);
                }

                // If no UID was found, but we asked for them, it's an error.
                if (is_uids && uid == 0)
                {
                    throw imap_error("Parsing failure.");
                }

                message msg;
                // Set the decoder line policy.
                msg.line_policy(codec::line_len_policy_t::RECOMMENDED, line_policy);
                msg.parse(literal_token->literal);
                // Success. Put the message into the result map, indexed either by message number or uid if we asked for uids.
                msgs.insert(std::pair<unsigned long /*message_number_or_uid*/, message>(is_uids ? uid : msg_no, msg));
            }
            else
                throw imap_error("Parsing failure.");
        }
        else if (parsed_line.tag == to_string(_tag))
        {
            if (parsed_line.result.value() == tag_result_response_t::OK)
                has_more = false;
            else
                throw imap_error("Fetching message failure.");
        }
        else
            throw imap_error("Parsing failure.");
    }
        
    reset_response_parser();
}


auto imap::statistics(const string& mailbox, unsigned int info) -> mailbox_stat_t
{
    // It doesn't like search terms it doesn't recognize.
    // Some older protocol versions or some servers may not support them.
    // So unseen uidnext and uidvalidity are optional.
    string cmd = "STATUS \"" + mailbox + "\" (messages recent";
    if (info & mailbox_stat_t::UNSEEN)
        cmd += " unseen";
    if (info & mailbox_stat_t::UID_NEXT)
        cmd += " uidnext";
    if (info & mailbox_stat_t::UID_VALIDITY)
        cmd += " uidvalidity";
    cmd += ")";
    
    _dlg->send(format(cmd));
    mailbox_stat_t stat;
    
    bool has_more = true;
    while (has_more)
    {
        reset_response_parser();
        string line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);

        if (parsed_line.tag == "*")
        {
            parse_response(parsed_line.response);
            if (!iequals(_mandatory_part.front()->atom, "STATUS"))
                throw imap_error("Getting statistics failure.");
            _mandatory_part.pop_front();
            
            bool mess_found = false, recent_found = false;
            for (auto it = _mandatory_part.begin(); it != _mandatory_part.end(); it++)
                if ((*it)->token_type == response_token_t::token_type_t::LIST && (*it)->parenthesized_list.size() >= 2)
                {
                    bool key_found = false;
                    string key;
                    auto mp = *it;
                    for(auto il = mp->parenthesized_list.begin(); il != mp->parenthesized_list.end(); ++il)
                    {
                        const string& value = (*il)->atom;
                        if (key_found)
                        {
                            if (iequals(key, "MESSAGES"))
                            {
                                stat.messages_no = stoul(value);
                                mess_found = true;
                            }
                            else if (iequals(key, "RECENT"))
                            {
                                stat.messages_recent = stoul(value);
                                recent_found = true;
                            }
                            else if (iequals(key, "UNSEEN"))
                            {
                                stat.messages_unseen = stoul(value);
                            }
                            else if (iequals(key, "UIDNEXT"))
                            {
                                stat.uid_next = stoul(value);
                            }
                            else if (iequals(key, "UIDVALIDITY"))
                            {
                                stat.uid_validity = stoul(value);
                            }
                            key_found = false;
                        }
                        else
                        {
                            key = value;
                            key_found = true;
                        }
                    }
                }
            // The MESSAGES and RECENT are required.
            if (!mess_found || !recent_found)
                throw imap_error("Parsing failure.");
        }
        else if (parsed_line.tag == to_string(_tag))
        {
            if (parsed_line.result.value() == tag_result_response_t::OK)
                has_more = false;
            else
                throw imap_error("Getting statistics failure.");
        }
        else
            throw imap_error("Parsing failure.");
    }

    reset_response_parser();
    return stat;
}


void imap::remove(const string& mailbox, unsigned long message_no)
{
    select(mailbox);
    remove(message_no);
}


void imap::remove(unsigned long message_no, bool is_uid)
{
    string cmd;
    if (is_uid)
        cmd.append("UID ");
    cmd.append("STORE " + to_string(message_no) + " +FLAGS (\\Deleted)");
    _dlg->send(format(cmd));

    bool has_more = true;
    while (has_more)
    {
        reset_response_parser();
        string line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);
        
        if (parsed_line.tag == "*")
        {
            parse_response(parsed_line.response);
            auto token = _mandatory_part.front();
            if (token->token_type != response_token_t::token_type_t::ATOM || stoul(token->atom) != message_no)
                throw imap_error("Deleting message failure.");
            // TODO: Untagged FETCH response also to be checked?
            continue;
        }
        else if (parsed_line.tag == to_string(_tag))
        {
            if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
                throw imap_error("Deleting message failure.");
            else
            {
                reset_response_parser();
                _dlg->send(format("CLOSE"));
                string line = _dlg->receive();
                tag_result_response_t parsed_line = parse_tag_result(line);

                if (!iequals(parsed_line.tag, to_string(_tag)))
                    throw imap_error("Parsing failure.");
                if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
                    throw imap_error("Deleting message failure.");
            }
            has_more = false;
        }
        else
            throw imap_error("Parsing failure.");
    }
}


void imap::search(const list<imap::search_condition_t>& conditions, list<unsigned long>& results, bool want_uids)
{
    string cond_str;
    int elem = 0;
    for (const auto& c : conditions)
        if (elem++ < conditions.size() - 1)
            cond_str += c.imap_string + " ";
        else
            cond_str += c.imap_string;
    search(cond_str, results, want_uids);
}


bool imap::create_folder(const list<string>& folder_tree)
{
    string delim = folder_delimiter();
    string folder_str = folder_tree_to_string(folder_tree, delim);
    _dlg->send(format("CREATE \"" + folder_str + "\""));

    string line = _dlg->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag != to_string(_tag))
        throw imap_error("Parsing failure.");
    if (parsed_line.result.value() == tag_result_response_t::NO)
        return false;
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Creating folder failure.");
    return true;
}


auto imap::list_folders(const list<string>& folder_name) -> mailbox_folder
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    _dlg->send(format("LIST \"\" \"" + folder_name_s + "*\""));
    mailbox_folder mailboxes;

    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);
        parse_response(parsed_line.response);
        if (parsed_line.tag == "*")
        {
            auto token = _mandatory_part.front();
            _mandatory_part.pop_front();
            if (!iequals(token->atom, "LIST"))
                 throw imap_error("Listing folders failure.");

            if (_mandatory_part.size() < 3)
                throw imap_error("Parsing failure.");
            auto found_folder = _mandatory_part.begin();
            found_folder++; found_folder++;
            if (found_folder != _mandatory_part.end() && (*found_folder)->token_type == response_token_t::token_type_t::ATOM)
            {
                vector<string> folders_hierarchy;
                // TODO: May `delim` contain more than one character?
                split(folders_hierarchy, (*found_folder)->atom, is_any_of(delim));
                map<string, mailbox_folder>* mbox = &mailboxes.folders;
                for (auto f : folders_hierarchy)
                {
                    auto fit = find_if(mbox->begin(), mbox->end(), [&f](const std::pair<string, mailbox_folder>& mf){ return mf.first == f; });
                    if (fit == mbox->end())
                        mbox->insert(std::make_pair(f, mailbox_folder{}));
                    mbox = &(mbox->at(f).folders);
                }
            }
            else
                throw imap_error("Parsing failure.");
        }
        else if (parsed_line.tag == to_string(_tag))
        {
            has_more = false;
        }
        reset_response_parser();
    }

    return mailboxes;
}


bool imap::delete_folder(const list<string>& folder_name)
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    _dlg->send(format("DELETE \"" + folder_name_s + "\""));

    string line = _dlg->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag != to_string(_tag))
        throw imap_error("Parsing failure.");
    if (parsed_line.result.value() == tag_result_response_t::NO)
        return false;
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Deleting folder failure.");
    return true;
}


bool imap::rename_folder(const list<string>& old_name, const list<string>& new_name)
{
    string delim = folder_delimiter();
    string old_name_s = folder_tree_to_string(old_name, delim);
    string new_name_s = folder_tree_to_string(new_name, delim);
    _dlg->send(format("RENAME \"" + old_name_s + "\" \"" + new_name_s + "\""));

    string line = _dlg->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag != to_string(_tag))
        throw imap_error("Parsing failure.");
    if (parsed_line.result.value() == tag_result_response_t::NO)
        return false;
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Renaming folder failure.");
    return true;
}


void imap::connect()
{
    // read greetings message
    string line = _dlg->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);

    if (parsed_line.tag != "*")
        throw imap_error("Parsing failure.");
    if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Connection to server failure.");
}


void imap::auth_login(const string& username, const string& password)
{
    auto cmd = format("LOGIN " + username + " " + password);
    _dlg->send(cmd);
    
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);

        if (parsed_line.tag == "*")
            continue;
        if (parsed_line.tag != to_string(_tag))
            throw imap_error("Parsing failure.");
        if (parsed_line.result.value() != tag_result_response_t::OK)
            throw imap_error("Authentication failure.");
    
        has_more = false;
    }
}


auto imap::select(const string& mailbox, bool read_only) -> mailbox_stat_t
{
    string cmd;
    if (read_only)
        cmd = format("EXAMINE \"" + mailbox + "\"");
    else
        cmd = format("SELECT \"" + mailbox + "\"");
    _dlg->send(cmd);

    mailbox_stat_t stat;
    bool exists_found = false;
    bool recent_found = false;
    bool has_more = true;

    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = _dlg->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);
            parse_response(parsed_line.response);

            if (parsed_line.tag == "*")
            {
                const auto result = parsed_line.result;
                if (result.has_value() && result.value() == tag_result_response_t::OK)
                {
                    if (_optional_part.size() != 2)
                        continue;

                    auto key = _optional_part.front();
                    _optional_part.pop_front();
                    if (key->token_type == response_token_t::token_type_t::ATOM)
                    {
                        auto value = _optional_part.front();
                        if (iequals(key->atom, "UNSEEN"))
                        {
                            if (value->token_type != response_token_t::token_type_t::ATOM)
                                throw imap_error("Parsing failure.");
                            stat.messages_first_unseen = stoul(value->atom);
                        }
                        else if (iequals(key->atom, "UIDNEXT"))
                        {
                            if (value->token_type != response_token_t::token_type_t::ATOM)
                                throw imap_error("Parsing failure.");
                            stat.uid_next = stoul(value->atom);
                        }
                        else if (iequals(key->atom, "UIDVALIDITY"))
                        {
                            if (value->token_type != response_token_t::token_type_t::ATOM)
                                throw imap_error("Parsing failure.");
                            stat.uid_validity = stoul(value->atom);
                        }
                    }
                }
                else
                {
                    if (_mandatory_part.size() == 2 && _mandatory_part.front()->token_type == response_token_t::token_type_t::ATOM)
                    {
                        auto value = _mandatory_part.front();
                        _mandatory_part.pop_front();
                        auto key = _mandatory_part.front();
                        _mandatory_part.pop_front();
                        if (iequals(key->atom, "EXISTS"))
                        {
                            stat.messages_no = stoul(value->atom);
                            exists_found = true;
                        }
                        else if (iequals(key->atom, "RECENT"))
                        {
                            stat.messages_recent = stoul(value->atom);
                            recent_found = true;
                        }
                    }
                }
            }
            else if (parsed_line.tag == to_string(_tag))
            {
                if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
                    throw imap_error("Select or examine mailbox failure.");

                has_more = false;
            }
            else
                throw imap_error("Parsing failure.");
        }
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Parsing failure.");
    }

    // The EXISTS and RECENT are required, the others may be missing in earlier protocol versions.
    if (!exists_found || !recent_found)
        throw imap_error("Parsing failure.");

    reset_response_parser();
    return stat;
}


void imap::search(const string& conditions, list<unsigned long>& results, bool want_uids)
{
    string cmd;
    if (want_uids)
        cmd.append("UID ");
    cmd.append("SEARCH " + conditions);
    _dlg->send(format(cmd));

    bool has_more = true;
    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = _dlg->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);
            if (parsed_line.tag == "*")
            {
                parse_response(parsed_line.response);

                auto search_token = _mandatory_part.front();
                if (search_token->token_type == response_token_t::token_type_t::ATOM && !iequals(search_token->atom, "SEARCH"))
                    throw imap_error("Search mailbox failure.");
                _mandatory_part.pop_front();

                for (auto it = _mandatory_part.begin(); it != _mandatory_part.end(); it++)
                    if ((*it)->token_type == response_token_t::token_type_t::ATOM)
                    {
                        const unsigned long idx = stoul((*it)->atom);
                        if (idx == 0)
                            throw imap_error("Parsing failure.");
                        results.push_back(idx);
                    }
            }
            else if (parsed_line.tag == to_string(_tag))
            {
                if (parsed_line.result.value() != tag_result_response_t::OK)
                    throw imap_error("Search mailbox failure.");
 
                has_more = false;
            }
            else
            {
                throw imap_error("Parsing failure.");
            }
        }
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Parsing failure.");
    }
    reset_response_parser();
}


string imap::folder_delimiter()
{
    string delimiter;
    _dlg->send(format("LIST \"\" \"\""));
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);
        if (parsed_line.tag == "*" && delimiter.empty())
        {
            parse_response(parsed_line.response);
            if (!iequals(_mandatory_part.front()->atom, "LIST"))
                throw imap_error("Determining folder delimiter failure.");
            _mandatory_part.pop_front();

            if (_mandatory_part.size() < 3)
                throw imap_error("Determining folder delimiter failure.");
            auto it = _mandatory_part.begin();
            if ((*(++it))->token_type != response_token_t::token_type_t::ATOM)
                throw imap_error("Determining folder delimiter failure.");
            delimiter = trim_copy_if((*it)->atom, [](char c ){ return c == codec::QUOTE_CHAR; });
            reset_response_parser();
        }
        else if (parsed_line.tag == to_string(_tag))
        {
            if (parsed_line.result.value() != tag_result_response_t::OK)
                throw imap_error("Determining folder delimiter failure.");

            has_more = false;
        }
    }
    return delimiter;
}


auto imap::parse_tag_result(const string& line) const -> tag_result_response_t
{
    string::size_type tag_pos = line.find(' ');
    if (tag_pos == string::npos)
        throw imap_error("Parsing failure.");
    string tag = line.substr(0, tag_pos);

    string::size_type result_pos = string::npos;
    result_pos = line.find(' ', tag_pos + 1);
    string result_s = line.substr(tag_pos + 1, result_pos - tag_pos - 1);
    std::optional<tag_result_response_t::result_t> result = std::nullopt;
    if (iequals(result_s, "OK"))
        result = make_optional(tag_result_response_t::OK);
    if (iequals(result_s, "NO"))
        result = make_optional(tag_result_response_t::NO);
    if (iequals(result_s, "BAD"))
        result = make_optional(tag_result_response_t::BAD);

    string response;
    if (result.has_value())
        response = line.substr(result_pos + 1);
    else
        response = line.substr(tag_pos + 1);
    return tag_result_response_t(tag, result, response);
}


/*
Protocol defines response line as tag (including plus and asterisk chars), result (ok, no, bad) and the response content which consists of optional
and mandatory part. Protocol grammar defines the response as sequence of atoms, string literals and parenthesized list (which itself can contain
atoms, string literal and parenthesized lists). The grammar can be parsed in one pass by counting which token is read: atom, string literal or
parenthesized list:
1. if a square bracket is reached, then an optional part is found, so parse its content as usual
2. if a brace is read, then string literal size is found, so read a number and then literal itself
3. if a parenthesis is found, then a list is being read, so increase the parenthesis counter and proceed
4. for a regular char check the state and determine if an atom or string size/literal is read
  
Token of the grammar is defined by `response_token_t` and stores one of the three types. Since parenthesized list is recursively defined, it keeps
sequence of tokens. When a character is read, it belongs to the last token of the sequence of tokens at the given parenthesis depth. The last token
of the response expression is found by getting the last token of the token sequence at the given depth (in terms of parenthesis count). 
*/ 
void imap::parse_response(const string& response)
{
    list<shared_ptr<imap::response_token_t>>* token_list;
    
    if (_literal_state == string_literal_state_t::READING)
    {
        token_list = _optional_part_state ? find_last_token_list(_optional_part) : find_last_token_list(_mandatory_part);
        if (token_list->back()->token_type == response_token_t::token_type_t::LITERAL && _literal_bytes_read > token_list->back()->literal.size())
            throw imap_error("Parser failure.");
        unsigned long literal_size = stoul(token_list->back()->literal_size);
        if (_literal_bytes_read + response.size() < literal_size)
        {
            token_list->back()->literal += response + codec::CRLF;
            _literal_bytes_read += response.size() + _eols_no;
            if (_literal_bytes_read == literal_size)
                _literal_state = string_literal_state_t::DONE;
            return;
        }
        else
        {
            string::size_type resp_len = response.size();
            token_list->back()->literal += response.substr(0, literal_size - _literal_bytes_read);
            _literal_bytes_read += literal_size - _literal_bytes_read;
            _literal_state = string_literal_state_t::DONE;
            parse_response(response.substr(resp_len - (literal_size - _literal_bytes_read) - 1));
            return;
        }
    }
    
    shared_ptr<response_token_t> cur_token;
    for (auto ch : response)
    {
        switch (ch)
        {
            case codec::LEFT_BRACKET_CHAR:
            {
                if (_atom_state == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (_optional_part_state)
                        throw imap_error("Parser failure.");

                    _optional_part_state = true;
                }
            }
            break;
        
            case codec::RIGHT_BRACKET_CHAR:
            {
                if (_atom_state == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (!_optional_part_state)
                        throw imap_error("Parser failure.");

                    _optional_part_state = false;
                    _atom_state = atom_state_t::NONE;
                }
            }
            break;
            
            case codec::LEFT_PARENTHESIS_CHAR:
            {
                if (_atom_state == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    cur_token = make_shared<response_token_t>();
                    cur_token->token_type = response_token_t::token_type_t::LIST;
                    token_list = _optional_part_state ? find_last_token_list(_optional_part) : find_last_token_list(_mandatory_part);
                    token_list->push_back(cur_token);
                    _parenthesis_list_counter++;
                    _atom_state = atom_state_t::NONE;
                }
            }
            break;
            
            case codec::RIGHT_PARENTHESIS_CHAR:
            {
                if (_atom_state == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (_parenthesis_list_counter == 0)
                        throw imap_error("Parser failure.");

                    _parenthesis_list_counter--;
                    _atom_state = atom_state_t::NONE;
                }
            }
            break;
            
            case codec::LEFT_BRACE_CHAR:
            {
                if (_atom_state == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (_literal_state == string_literal_state_t::SIZE)
                        throw imap_error("Parser failure.");

                    cur_token = make_shared<response_token_t>();
                    cur_token->token_type = response_token_t::token_type_t::LITERAL;
                    token_list = _optional_part_state ? find_last_token_list(_optional_part) : find_last_token_list(_mandatory_part);
                    token_list->push_back(cur_token);
                    _literal_state = string_literal_state_t::SIZE;
                    _atom_state = atom_state_t::NONE;
                }
            }
            break;
            
            case codec::RIGHT_BRACE_CHAR:
            {
                if (_atom_state == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (_literal_state == string_literal_state_t::NONE)
                        throw imap_error("Parser failure.");

                    _literal_state = string_literal_state_t::WAITING;
                }
            }
            break;
            
            case codec::SPACE_CHAR:
            {
                if (_atom_state == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (cur_token != nullptr)
                    {
                        trim(cur_token->atom);
                        _atom_state = atom_state_t::NONE;
                    }
                }
            }
            break;

            case codec::QUOTE_CHAR:
            {
                if (_atom_state == atom_state_t::NONE)
                {
                    cur_token = make_shared<response_token_t>();
                    cur_token->token_type = response_token_t::token_type_t::ATOM;
                    token_list = _optional_part_state ? find_last_token_list(_optional_part) : find_last_token_list(_mandatory_part);
                    token_list->push_back(cur_token);
                    _atom_state = atom_state_t::QUOTED;
                }
                else if (_atom_state == atom_state_t::QUOTED)
                    _atom_state = atom_state_t::NONE;
            }
            break;
            
            default:
            {
                if (_literal_state == string_literal_state_t::SIZE)
                {
                    if (!isdigit(ch))
                        throw imap_error("Parser failure.");
                    
                    cur_token->literal_size += ch;
                }
                else if (_literal_state == string_literal_state_t::WAITING)
                {
                    // no characters allowed after the right brace, crlf is required
                    throw imap_error("Parser failure.");
                }
                else
                {
                    if (_atom_state == atom_state_t::NONE)
                    {
                        cur_token = make_shared<response_token_t>();
                        cur_token->token_type = response_token_t::token_type_t::ATOM;
                        token_list = _optional_part_state ? find_last_token_list(_optional_part) : find_last_token_list(_mandatory_part);
                        token_list->push_back(cur_token);
                        _atom_state = atom_state_t::PLAIN;
                    }
                    cur_token->atom += ch;
                }
            }
        }
    }
    if (_literal_state == string_literal_state_t::WAITING)
        _literal_state = string_literal_state_t::READING;
}

void imap::reset_response_parser()
{
    _optional_part.clear();
    _mandatory_part.clear();
    _optional_part_state = false;
    _atom_state = atom_state_t::NONE;
    _parenthesis_list_counter = 0;
    _literal_state = string_literal_state_t::NONE;
    _literal_bytes_read = 0;
    _eols_no = 2;
}


string imap::format(const string& command)
{
    return to_string(++_tag) + " " + command;
}


void imap::trim_eol(string& line)
{
    if (line.length() >= 1 && line[line.length() - 1] == codec::CR_CHAR)
    {
        _eols_no = 2;
        line.pop_back();
    }
    else
        _eols_no = 1;
}


string imap::folder_tree_to_string(const list<string>& folder_tree, string delimiter) const
{
    string folders;
    int elem = 0;
    for (const auto& f : folder_tree)
        if (elem++ < folder_tree.size() - 1)
            folders += f + delimiter;
        else
            folders += f;
    return folders;
}


string imap::imap_date_to_string(const boost::gregorian::date& gregorian_date)
{
    stringstream ss;
    boost::gregorian::date_facet* facet = new boost::gregorian::date_facet("%d-%b-%Y");
    ss.exceptions(std::ios_base::failbit);
    ss.imbue(std::locale(ss.getloc(), facet));
    ss << gregorian_date;
    return ss.str();
}



list<shared_ptr<imap::response_token_t>>* imap::find_last_token_list(list<shared_ptr<response_token_t>>& token_list)
{
    list<shared_ptr<response_token_t>>* list_ptr = &token_list;
    unsigned int depth = 1;
    while (!list_ptr->empty() && list_ptr->back()->token_type == response_token_t::token_type_t::LIST && depth <= _parenthesis_list_counter)
    {
        list_ptr = &(list_ptr->back()->parenthesized_list);
        depth++;
    }
    return list_ptr;
}


imaps::imaps(const string& hostname, unsigned port, milliseconds timeout) : imap(hostname, port, timeout)
{
}


void imaps::authenticate(const string& username, const string& password, auth_method_t method)
{
    if (method == auth_method_t::LOGIN)
    {
        switch_to_ssl();
        connect();
        auth_login(username, password);
    }
    else if (method == auth_method_t::START_TLS)
    {
        connect();
        start_tls();
        auth_login(username, password);
    }
}


void imaps::start_tls()
{
    _dlg->send(format("STARTTLS"));
    string line = _dlg->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag == "*")
        throw imap_error("Bad server response.");
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Start TLS refused by server.");

    switch_to_ssl();
}


void imaps::switch_to_ssl()
{
    dialog* d = _dlg.get();
    _dlg.reset(new dialog_ssl(move(*d)));
}


} // namespace mailio
