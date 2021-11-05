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

const string imap::UNTAGGED_RESPONSE{"*"};
const string imap::CONTINUE_RESPONSE{"+"};
const string imap::RANGE_SEPARATOR{":"};
const string imap::RANGE_ALL{"*"};
const string imap::LIST_SEPARATOR{","};
const string imap::TOKEN_SEPARATOR_STR{" "};
const string imap::QUOTED_STRING_SEPARATOR{"\""};


string imap::messages_range_to_string(imap::messages_range_t id_pair)
{
    return to_string(id_pair.first) + (id_pair.second.has_value() ? RANGE_SEPARATOR + to_string(id_pair.second.value()) : RANGE_SEPARATOR + RANGE_ALL);
}


string imap::messages_range_list_to_string(list<messages_range_t> ranges)
{
    return boost::join(ranges | boost::adaptors::transformed(static_cast<string(*)(messages_range_t)>(messages_range_to_string)), LIST_SEPARATOR);
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

            case SID_LIST:
            {
                imap_string = messages_range_list_to_string(std::get<list<messages_range_t>>(value));
                break;
            }

            case UID_LIST:
            {
                imap_string = "UID " + messages_range_list_to_string(std::get<list<messages_range_t>>(value));
                break;
            }

            case SUBJECT:
                imap_string = "SUBJECT " + QUOTED_STRING_SEPARATOR + std::get<string>(value) + QUOTED_STRING_SEPARATOR;
                break;

            case FROM:
                imap_string = "FROM " + QUOTED_STRING_SEPARATOR + std::get<string>(value) + QUOTED_STRING_SEPARATOR;
                break;

            case TO:
                imap_string = "TO " + QUOTED_STRING_SEPARATOR + std::get<string>(value) + QUOTED_STRING_SEPARATOR;
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
            result_s = "NO";
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
    _dlg(make_shared<dialog>(hostname, port, timeout)), _tag(0), _optional_part_state(false), _atom_state(atom_state_t::NONE),
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


string imap::authenticate(const string& username, const string& password, auth_method_t method)
{
    string greeting = connect();
    if (method == auth_method_t::LOGIN)
        auth_login(username, password);
    return greeting;
}


auto imap::select(const list<string>& folder_name, bool /*read_only*/) -> mailbox_stat_t
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    return select(folder_name_s);
}


auto imap::select(const string& mailbox, bool read_only) -> mailbox_stat_t
{
    string cmd;
    if (read_only)
        cmd = format("EXAMINE " + QUOTED_STRING_SEPARATOR + mailbox + QUOTED_STRING_SEPARATOR);
    else
        cmd = format("SELECT " + QUOTED_STRING_SEPARATOR + mailbox + QUOTED_STRING_SEPARATOR);
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

            if (parsed_line.tag == UNTAGGED_RESPONSE)
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
    catch (const invalid_argument&)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range&)
    {
        throw imap_error("Parsing failure.");
    }

    // The EXISTS and RECENT are required, the others may be missing in earlier protocol versions.
    if (!exists_found || !recent_found)
        throw imap_error("Parsing failure.");

    reset_response_parser();
    return stat;
}


void imap::fetch(const string& mailbox, unsigned long message_no, message& msg, bool header_only)
{
    select(mailbox);
    fetch(message_no, msg, false, header_only);
}


void imap::fetch(unsigned long message_no, message& msg, bool is_uid, bool header_only)
{
    list<messages_range_t> messages_range;
    messages_range.push_back(imap::messages_range_t(message_no, message_no));
    map<unsigned long, message> found_messages;
    fetch(messages_range, found_messages, is_uid, header_only, msg.line_policy());
    msg = std::move(found_messages.begin()->second);
}


// Fetching literal is the only place where line is ended with LF only, instead of CRLF. Thus, `receive(true)` and counting EOLs is performed.
void imap::fetch(const list<messages_range_t>& messages_range, map<unsigned long, message>& found_messages, bool is_uids, bool header_only,
    codec::line_len_policy_t line_policy)
{
    const string RFC822_TOKEN = string("RFC822") + (header_only ? ".HEADER" : "");
    const string message_ids = messages_range_list_to_string(messages_range);

    string cmd;
    if (is_uids)
        cmd.append("UID ");
    cmd.append("FETCH " + message_ids + TOKEN_SEPARATOR_STR + RFC822_TOKEN);
    _dlg->send(format(cmd));

    // Stores messages as string literals for parsing after the OK response.
    map<unsigned long, string> msg_str;
    bool has_more = true;
    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = _dlg->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);

            if (parsed_line.tag == UNTAGGED_RESPONSE)
            {
                parse_response(parsed_line.response);

                if (_mandatory_part.front()->token_type != response_token_t::token_type_t::ATOM)
                    throw imap_error("Fetching message failure.");
                unsigned long msg_no = stoul(_mandatory_part.front()->atom);
                _mandatory_part.pop_front();
                if (msg_no == 0)
                    throw imap_error("Fetching message failure.");

                if (!iequals(_mandatory_part.front()->atom, "FETCH"))
                    throw imap_error("Fetching message failure.");

                unsigned long uid = 0;
                shared_ptr<response_token_t> literal_token = nullptr;
                for (auto part : _mandatory_part)
                    if (part->token_type == response_token_t::token_type_t::LIST)
                        for (auto token = part->parenthesized_list.begin(); token != part->parenthesized_list.end(); token++)
                            if ((*token)->token_type == response_token_t::token_type_t::ATOM)
                                if (iequals((*token)->atom, "UID"))
                                {
                                    token++;
                                    if (token == part->parenthesized_list.end())
                                        throw imap_error("Parsing failure.");
                                    uid = stoul((*token)->atom);
                                }
                                else if (iequals((*token)->atom, RFC822_TOKEN))
                                {
                                    token++;
                                    if (token == part->parenthesized_list.end() || (*token)->token_type != response_token_t::token_type_t::LITERAL)
                                        throw imap_error("Parsing failure.");
                                    literal_token = *token;
                                    break;
                                }

                if (literal_token != nullptr)
                {
                    // Loop to read string literal.
                    while (_literal_state == string_literal_state_t::READING)
                    {
                        string line = _dlg->receive(true);
                        if (!line.empty())
                            trim_eol(line);
                        parse_response(line);
                    }
                    // Closing parenthesis not yet read.
                    if (_literal_state == string_literal_state_t::DONE && _parenthesis_list_counter > 0)
                    {
                        string line = _dlg->receive(true);
                        if (!line.empty())
                            trim_eol(line);
                        parse_response(line);
                    }
                    msg_str.emplace(is_uids ? uid : msg_no, move(literal_token->literal));

                    // If no UID was found, but we asked for them, it's an error.
                    if (is_uids && uid == 0)
                    {
                        throw imap_error("Parsing failure.");
                    }
                }
                else
                    throw imap_error("Parsing failure.");
            }
            else if (parsed_line.tag == to_string(_tag))
            {
                if (parsed_line.result.value() == tag_result_response_t::OK)
                {
                    has_more = false;
                    for (const auto& ms : msg_str)
                    {
                        message msg;
                        msg.line_policy(line_policy, line_policy);
                        msg.parse(ms.second);
                        found_messages.emplace(ms.first, move(msg));
                    }
                }
                else
                    throw imap_error("Fetching message failure.");
            }
            else
                throw imap_error("Parsing failure.");
        }
    }
    catch (const invalid_argument&)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range&)
    {
        throw imap_error("Parsing failure.");
    }

    reset_response_parser();
}


void imap::append(const list<string>& folder_name, const message& msg)
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    append(folder_name_s, msg);
}


void imap::append(const string& folder_name, const message& msg)
{
    string msg_str;
    msg.format(msg_str, true);

    string cmd = "APPEND " + folder_name;
    cmd.append(" {" + to_string(msg_str.size()) + "}");
    _dlg->send(format(cmd));
    string line = _dlg->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.result == tag_result_response_t::BAD || parsed_line.tag != CONTINUE_RESPONSE)
        throw imap_error("Message appending failure.");

    _dlg->send(msg_str);
    bool has_more = true;
    while (has_more)
    {
        line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);
        if (parsed_line.tag == to_string(_tag))
        {
            if (parsed_line.result != tag_result_response_t::OK)
                throw imap_error("Message appending failure.");
            has_more = false;
        }
        else if (parsed_line.tag != UNTAGGED_RESPONSE)
            throw imap_error("Message appending failure.");
    }
}


auto imap::statistics(const string& mailbox, unsigned int info) -> mailbox_stat_t
{
    // It doesn't like search terms it doesn't recognize.
    // Some older protocol versions or some servers may not support them.
    // So unseen uidnext and uidvalidity are optional.
    string cmd = "STATUS " + QUOTED_STRING_SEPARATOR + mailbox + QUOTED_STRING_SEPARATOR + " (messages recent";
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
    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = _dlg->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);

            if (parsed_line.tag == UNTAGGED_RESPONSE)
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
    }
    catch (const invalid_argument&)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range&)
    {
        throw imap_error("Parsing failure.");
    }

    reset_response_parser();
    return stat;
}


auto imap::statistics(const list<string>& folder_name, unsigned int info) -> mailbox_stat_t
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    return statistics(folder_name_s, info);
}


void imap::remove(const string& mailbox, unsigned long message_no, bool is_uid)
{
    select(mailbox);
    remove(message_no, is_uid);
}


void imap::remove(const list<string>& mailbox, unsigned long message_no, bool is_uid)
{
    string delim = folder_delimiter();
    string mailbox_s = folder_tree_to_string(mailbox, delim);
    remove(mailbox_s, message_no, is_uid);
}


void imap::remove(unsigned long message_no, bool is_uid)
{
    string cmd;
    if (is_uid)
        cmd.append("UID ");
    cmd.append("STORE " + to_string(message_no) + " +FLAGS (\\Deleted)");
    _dlg->send(format(cmd));

    bool has_more = true;
    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = _dlg->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);

            if (parsed_line.tag == UNTAGGED_RESPONSE)
            {
                parse_response(parsed_line.response);

                if (_mandatory_part.empty())
                    throw imap_error("Parsing failure.");
                auto msg_no_token = _mandatory_part.front();
                _mandatory_part.pop_front();

                if (_mandatory_part.empty())
                    throw imap_error("Parsing failure.");
                auto fetch_token = _mandatory_part.front();
                if (!iequals(fetch_token->atom, "FETCH"))
                    throw imap_error("Parsing failure.");
                _mandatory_part.pop_front();

                // Check the list with flags.

                if (_mandatory_part.empty())
                    throw imap_error("Parsing failure.");
                auto flags_token_list = _mandatory_part.front();
                if (flags_token_list->token_type != response_token_t::token_type_t::LIST)
                    throw imap_error("Parsing failure.");

                std::shared_ptr<response_token_t> uid_token = nullptr;
                auto uid_token_it = flags_token_list->parenthesized_list.begin();
                do
                    if (iequals((*uid_token_it)->atom, "UID"))
                    {
                        uid_token_it++;
                        if (uid_token_it == flags_token_list->parenthesized_list.end())
                            throw imap_error("Parsing failure.");
                        uid_token = *uid_token_it;
                        break;
                    }
                    else
                        uid_token_it++;
                while (uid_token_it != flags_token_list->parenthesized_list.end());

                if (is_uid)
                {
                    if (uid_token == nullptr)
                        throw imap_error("Parsing failure.");
                    msg_no_token = uid_token;
                }

                if (msg_no_token->token_type != response_token_t::token_type_t::ATOM || stoul(msg_no_token->atom) != message_no)
                    throw imap_error("Deleting message failure.");

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
    catch (const invalid_argument&)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range&)
    {
        throw imap_error("Parsing failure.");
    }
}


void imap::search(const list<imap::search_condition_t>& conditions, list<unsigned long>& results, bool want_uids)
{
    string cond_str;
    std::size_t elem = 0;
    for (const auto& c : conditions)
        if (elem++ < conditions.size() - 1)
            cond_str += c.imap_string + TOKEN_SEPARATOR_STR;
        else
            cond_str += c.imap_string;
    search(cond_str, results, want_uids);
}


bool imap::create_folder(const string& folder_name)
{
    _dlg->send(format("CREATE " + QUOTED_STRING_SEPARATOR + folder_name + QUOTED_STRING_SEPARATOR));

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


bool imap::create_folder(const list<string>& folder_name)
{
    string delim = folder_delimiter();
    string folder_str = folder_tree_to_string(folder_name, delim);
    return create_folder(folder_str);
}


auto imap::list_folders(const string& folder_name) -> mailbox_folder_t
{
    string delim = folder_delimiter();
    _dlg->send(format("LIST " + QUOTED_STRING_SEPARATOR + QUOTED_STRING_SEPARATOR + TOKEN_SEPARATOR_STR + QUOTED_STRING_SEPARATOR + folder_name + "*" +
        QUOTED_STRING_SEPARATOR));
    mailbox_folder_t mailboxes;

    bool has_more = true;
    try
    {
        while (has_more)
        {
            string line = _dlg->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);
            parse_response(parsed_line.response);
            if (parsed_line.tag == UNTAGGED_RESPONSE)
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
                    map<string, mailbox_folder_t>* mbox = &mailboxes.folders;
                    for (auto f : folders_hierarchy)
                    {
                        auto fit = find_if(mbox->begin(), mbox->end(), [&f](const std::pair<string, mailbox_folder_t>& mf){ return mf.first == f; });
                        if (fit == mbox->end())
                            mbox->insert(std::make_pair(f, mailbox_folder_t{}));
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
    }
    catch (const invalid_argument&)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range&)
    {
        throw imap_error("Parsing failure.");
    }

    return mailboxes;
}


auto imap::list_folders(const list<string>& folder_name) -> mailbox_folder_t
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    return list_folders(folder_name_s);
}


bool imap::delete_folder(const string& folder_name)
{
    _dlg->send(format("DELETE " + QUOTED_STRING_SEPARATOR + folder_name + QUOTED_STRING_SEPARATOR));

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


bool imap::delete_folder(const list<string>& folder_name)
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    return delete_folder(folder_name_s);
}


bool imap::rename_folder(const string& old_name, const string& new_name)
{
    _dlg->send(format("RENAME " + QUOTED_STRING_SEPARATOR + old_name + QUOTED_STRING_SEPARATOR + TOKEN_SEPARATOR_STR + QUOTED_STRING_SEPARATOR + new_name +
        QUOTED_STRING_SEPARATOR));

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


bool imap::rename_folder(const list<string>& old_name, const list<string>& new_name)
{
    string delim = folder_delimiter();
    string old_name_s = folder_tree_to_string(old_name, delim);
    string new_name_s = folder_tree_to_string(new_name, delim);
    return rename_folder(old_name_s, new_name_s);
}


string imap::connect()
{
    // read greetings message
    string line = _dlg->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);

    if (parsed_line.tag != UNTAGGED_RESPONSE)
        throw imap_error("Parsing failure.");
    if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Connection to server failure.");
    return parsed_line.response;
}


void imap::auth_login(const string& username, const string& password)
{
    auto cmd = format("LOGIN " + username + TOKEN_SEPARATOR_STR + password);
    _dlg->send(cmd);

    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);

        if (parsed_line.tag == UNTAGGED_RESPONSE)
            continue;
        if (parsed_line.tag != to_string(_tag))
            throw imap_error("Parsing failure.");
        if (parsed_line.result.value() != tag_result_response_t::OK)
            throw imap_error("Authentication failure.");

        has_more = false;
    }
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
            if (parsed_line.tag == UNTAGGED_RESPONSE)
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
    catch (const invalid_argument&)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range&)
    {
        throw imap_error("Parsing failure.");
    }
    reset_response_parser();
}


string imap::folder_delimiter()
{
    try
    {
        static string delimiter;
        if (delimiter.empty())
        {
            _dlg->send(format("LIST " + QUOTED_STRING_SEPARATOR + QUOTED_STRING_SEPARATOR + TOKEN_SEPARATOR_STR + QUOTED_STRING_SEPARATOR + QUOTED_STRING_SEPARATOR));
            bool has_more = true;
            while (has_more)
            {
                string line = _dlg->receive();
                tag_result_response_t parsed_line = parse_tag_result(line);
                if (parsed_line.tag == UNTAGGED_RESPONSE && delimiter.empty())
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
                    delimiter = trim_copy_if((*it)->atom, [](char c ){ return c == QUOTED_STRING_SEPARATOR_CHAR; });
                    reset_response_parser();
                }
                else if (parsed_line.tag == to_string(_tag))
                {
                    if (parsed_line.result.value() != tag_result_response_t::OK)
                        throw imap_error("Determining folder delimiter failure.");

                    has_more = false;
                }
            }
        }
        return delimiter;
    }
    catch (const invalid_argument&)
    {
        throw imap_error("Parsing failure.");
    }
    catch (const out_of_range&)
    {
        throw imap_error("Parsing failure.");
    }
}


auto imap::parse_tag_result(const string& line) const -> tag_result_response_t
{
    string::size_type tag_pos = line.find(TOKEN_SEPARATOR_STR);
    if (tag_pos == string::npos)
        throw imap_error("Parsing failure.");
    string tag = line.substr(0, tag_pos);

    string::size_type result_pos = string::npos;
    result_pos = line.find(TOKEN_SEPARATOR_STR, tag_pos + 1);
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
            token_list->back()->literal += response + codec::END_OF_LINE;
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
            case OPTIONAL_BEGIN:
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

            case OPTIONAL_END:
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

            case LIST_BEGIN:
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

            case LIST_END:
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

            case STRING_LITERAL_BEGIN:
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

            case STRING_LITERAL_END:
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

            case TOKEN_SEPARATOR_CHAR:
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

            case QUOTED_ATOM:
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
    return to_string(++_tag) + TOKEN_SEPARATOR_STR + command;
}


void imap::trim_eol(string& line)
{
    if (line.length() >= 1 && line[line.length() - 1] == codec::END_OF_LINE[0])
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
    std::size_t elem = 0;
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
    ss.exceptions(std::ios_base::failbit);
    boost::gregorian::date_facet* facet = new boost::gregorian::date_facet("%d-%b-%Y");
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


string imaps::authenticate(const string& username, const string& password, auth_method_t method)
{
    string greeting;
    if (method == auth_method_t::LOGIN)
    {
        switch_to_ssl();
        greeting = connect();
        auth_login(username, password);
    }
    else if (method == auth_method_t::START_TLS)
    {
        greeting = connect();
        start_tls();
        auth_login(username, password);
    }
    return greeting;
}


void imaps::start_tls()
{
    _dlg->send(format("STARTTLS"));
    string line = _dlg->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag == UNTAGGED_RESPONSE)
        throw imap_error("Bad server response.");
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Start TLS refused by server.");

    switch_to_ssl();
}


void imaps::switch_to_ssl()
{
    _dlg = std::make_shared<dialog_ssl>(*_dlg);
}


} // namespace mailio
