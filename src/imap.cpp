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


string imap::to_astring(const string& text)
{
    return codec::surround_string(codec::escape_string(text, "\"\\"));
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

            case BODY:
                imap_string = "BODY " + QUOTED_STRING_SEPARATOR + std::get<string>(value) + QUOTED_STRING_SEPARATOR;
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

            case NEW:
                imap_string = "NEW";
                break;

            case RECENT:
                imap_string = "RECENT";
                break;

            case SEEN:
                imap_string = "SEEN";
                break;

            case UNSEEN:
                imap_string = "UNSEEN";
                break;

            default:
                break;
        }
    }
    catch (std::bad_variant_access& exc)
    {
        throw imap_error("Invaid search condition.", exc.what());
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
    dlg_(make_shared<dialog>(hostname, port, timeout)), tag_(0), optional_part_state_(false), atom_state_(atom_state_t::NONE),
    parenthesis_list_counter_(0), literal_state_(string_literal_state_t::NONE), literal_bytes_read_(0), eols_no_(2)
{
    dlg_->connect();
}


imap::~imap()
{
    try
    {
        dlg_->send(format("LOGOUT"));
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
        cmd = format("EXAMINE " + to_astring(mailbox));
    else
        cmd = format("SELECT " + to_astring(mailbox));
    dlg_->send(cmd);

    mailbox_stat_t stat;
    bool exists_found = false;
    bool recent_found = false;
    bool has_more = true;

    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = dlg_->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);
            parse_response(parsed_line.response);

            if (parsed_line.tag == UNTAGGED_RESPONSE)
            {
                const auto result = parsed_line.result;
                if (result.has_value() && result.value() == tag_result_response_t::OK)
                {
                    if (optional_part_.size() != 2)
                        continue;

                    auto key = optional_part_.front();
                    optional_part_.pop_front();
                    if (key->token_type == response_token_t::token_type_t::ATOM)
                    {
                        auto value = optional_part_.front();
                        if (iequals(key->atom, "UNSEEN"))
                        {
                            if (value->token_type != response_token_t::token_type_t::ATOM)
                                throw imap_error("Number expected for unseen.", "Line=`" + line + "`.");
                            stat.messages_first_unseen = stoul(value->atom);
                        }
                        else if (iequals(key->atom, "UIDNEXT"))
                        {
                            if (value->token_type != response_token_t::token_type_t::ATOM)
                                throw imap_error("Number expected for uidnext.", "Line=`" + line + "`.");
                            stat.uid_next = stoul(value->atom);
                        }
                        else if (iequals(key->atom, "UIDVALIDITY"))
                        {
                            if (value->token_type != response_token_t::token_type_t::ATOM)
                                throw imap_error("Number expected for uidvalidity.", "Line=`" + line + "`.");
                            stat.uid_validity = stoul(value->atom);
                        }
                    }
                }
                else
                {
                    if (mandatory_part_.size() == 2 && mandatory_part_.front()->token_type == response_token_t::token_type_t::ATOM)
                    {
                        auto value = mandatory_part_.front();
                        mandatory_part_.pop_front();
                        auto key = mandatory_part_.front();
                        mandatory_part_.pop_front();
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
            else if (parsed_line.tag == to_string(tag_))
            {
                if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
                    throw imap_error("Select or examine mailbox failure.", "Response=`" + parsed_line.response + "`.");

                has_more = false;
            }
            else
                throw imap_error("Parsing failure.", "Line=`" + line + "`.");
        }
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Integer expected.", exc.what());
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Integer expected.", exc.what());
    }

    // The EXISTS and RECENT are required, the others may be missing in earlier protocol versions.
    if (!exists_found || !recent_found)
        throw imap_error("No number of existing or recent messages.", "");

    reset_response_parser();
    return stat;
}


void imap::fetch(const string& mailbox, unsigned long message_no, message& msg, bool header_only)
{
    select(mailbox);
    fetch(message_no, msg, false, header_only);
}


void imap::fetch(const string& mailbox, unsigned long message_no, bool is_uid, message& msg, bool header_only)
{
    select(mailbox);
    fetch(message_no, msg, is_uid, header_only);
}


void imap::fetch(unsigned long message_no, message& msg, bool is_uid, bool header_only)
{
    list<messages_range_t> messages_range;
    messages_range.push_back(imap::messages_range_t(message_no, message_no));
    map<unsigned long, message> found_messages;
    fetch(messages_range, found_messages, is_uid, header_only, msg.line_policy());
    if (!found_messages.empty())
        msg = std::move(found_messages.begin()->second);
}


/*
Although the RFC mandates MIME messages to have lines ending with CRLF, some email clients put LF only. For that reason, the method `dialog::receive(true)`
is being used for fetching a literal. That way, the EOL counting is properly performed. This is probably the case for the strict mode policy.

According to the RFC 3501 section 6.4.5, the untagged response of the fetch command is not mandatory. Thus, some servers return just the tagged response if
no message is found.

The fetch untagged responses provide messages. In the last tagged response the status is obtained. Then, string literals can be parsed to validate the MIME
format.
*/
void imap::fetch(const list<messages_range_t>& messages_range, map<unsigned long, message>& found_messages, bool is_uids, bool header_only,
    codec::line_len_policy_t line_policy)
{
    if (messages_range.empty())
        throw imap_error("Empty messages range.", "");

    const string RFC822_TOKEN = string("RFC822") + (header_only ? ".HEADER" : "");
    const string message_ids = messages_range_list_to_string(messages_range);

    string cmd;
    if (is_uids)
        cmd.append("UID ");
    cmd.append("FETCH " + message_ids + TOKEN_SEPARATOR_STR + RFC822_TOKEN);
    dlg_->send(format(cmd));

    // Stores messages as strings indexed by the messages number.
    map<unsigned long, string> msg_str;
    // Flag whether the response line is the last one i.e. the tagged response.
    bool has_more = true;
    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = dlg_->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);

            // The untagged response collects all messages.
            if (parsed_line.tag == UNTAGGED_RESPONSE)
            {
                parse_response(parsed_line.response);

                if (mandatory_part_.front()->token_type != response_token_t::token_type_t::ATOM)
                    throw imap_error("Number expected when fetching a message.", "Response=`" + parsed_line.response + ".`");

                unsigned long sequence_no = stoul(mandatory_part_.front()->atom);
                mandatory_part_.pop_front();
                if (sequence_no == 0)
                    throw imap_error("Zero sequence number when fetching a message.", "");

                if (!iequals(mandatory_part_.front()->atom, "FETCH"))
                    throw imap_error("Expecting the fetch atom.", "Response=`" + parsed_line.response + ".`");

                unsigned long uid_no = 0;
                shared_ptr<response_token_t> literal_token = nullptr;
                for (auto part : mandatory_part_)
                    if (part->token_type == response_token_t::token_type_t::LIST)
                        for (auto token = part->parenthesized_list.begin(); token != part->parenthesized_list.end(); token++)
                            if ((*token)->token_type == response_token_t::token_type_t::ATOM)
                            {
                                if (iequals((*token)->atom, "UID"))
                                {
                                    token++;
                                    if (token == part->parenthesized_list.end())
                                        throw imap_error("No uid number when fetching a message.", "");
                                    uid_no = stoul((*token)->atom);
                                }
                                else if (iequals((*token)->atom, RFC822_TOKEN))
                                {
                                    token++;
                                    if (token == part->parenthesized_list.end() || (*token)->token_type != response_token_t::token_type_t::LITERAL)
                                        throw imap_error("No literal when fetching a message.", "");
                                    literal_token = *token;
                                    break;
                                }
                            }

                if (literal_token != nullptr)
                {
                    // Loop to read string literal.
                    while (literal_state_ == string_literal_state_t::READING)
                    {
                        string line = dlg_->receive(true);
                        if (!line.empty())
                            trim_eol(line);
                        parse_response(line);
                    }
                    // Closing parenthesis not yet read.
                    if (literal_state_ == string_literal_state_t::DONE && parenthesis_list_counter_ > 0)
                    {
                        string line = dlg_->receive(true);
                        if (!line.empty())
                            trim_eol(line);
                        // There could be a parenthesized list after the string literal. Read it and do nothing.
                        parse_response(line);
                    }
                    msg_str.emplace(is_uids ? uid_no : sequence_no, move(literal_token->literal));

                    // If no UID was found, but we asked for them, it's an error.
                    if (is_uids && uid_no == 0)
                    {
                        throw imap_error("No UID when fetching a message.", "");
                    }
                }
            }
            // The tagged response determines status and parses provided messages in `msg_str`.
            else if (parsed_line.tag == to_string(tag_))
            {
                if (parsed_line.result.value() == tag_result_response_t::OK)
                {
                    has_more = false;
                    for (const auto& ms : msg_str)
                    {
                        message msg;
                        msg.line_policy(line_policy);
                        msg.parse(ms.second);
                        found_messages.emplace(ms.first, move(msg));
                    }
                }
                else
                    throw imap_error("Fetching message failure.", "Response=`" + parsed_line.response + "`.");
            }
            else
                throw imap_error("Invalid tag when fetching a message.", "Parsed tag=`" + parsed_line.tag + "`.");
        }
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
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
    msg.format(msg_str, message_format_options_t{true, false});

    string cmd = "APPEND " + to_astring(folder_name);
    cmd.append(" {" + to_string(msg_str.size()) + "}");
    dlg_->send(format(cmd));
    string line = dlg_->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.result == tag_result_response_t::BAD || parsed_line.tag != CONTINUE_RESPONSE)
        throw imap_error("Message appending failure.", "Response=`" + parsed_line.response + "`.");

    dlg_->send(msg_str);
    bool has_more = true;
    while (has_more)
    {
        line = dlg_->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);
        if (parsed_line.tag == to_string(tag_))
        {
            if (parsed_line.result != tag_result_response_t::OK)
                throw imap_error("Message appending failure.", "Line=`" + line + "`.");
            has_more = false;
        }
        else if (parsed_line.tag != UNTAGGED_RESPONSE)
            throw imap_error("Expecting the untagged response.", "Tag=`" + parsed_line.tag + "`.");
    }
}


auto imap::statistics(const string& mailbox, unsigned int info) -> mailbox_stat_t
{
    // It doesn't like search terms it doesn't recognize.
    // Some older protocol versions or some servers may not support them.
    // So unseen uidnext and uidvalidity are optional.
    string cmd = "STATUS " + to_astring(mailbox) + " (messages recent";
    if (info & mailbox_stat_t::UNSEEN)
        cmd += " unseen";
    if (info & mailbox_stat_t::UID_NEXT)
        cmd += " uidnext";
    if (info & mailbox_stat_t::UID_VALIDITY)
        cmd += " uidvalidity";
    cmd += ")";

    dlg_->send(format(cmd));
    mailbox_stat_t stat;

    bool has_more = true;
    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = dlg_->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);

            if (parsed_line.tag == UNTAGGED_RESPONSE)
            {
                parse_response(parsed_line.response);
                if (!iequals(mandatory_part_.front()->atom, "STATUS"))
                    throw imap_error("Expecting the status atom.", "Line=`" + line + "`.");
                mandatory_part_.pop_front();

                bool mess_found = false, recent_found = false;
                for (auto it = mandatory_part_.begin(); it != mandatory_part_.end(); it++)
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
                    throw imap_error("No messages or recent messages found.", "");
            }
            else if (parsed_line.tag == to_string(tag_))
            {
                if (parsed_line.result.value() == tag_result_response_t::OK)
                    has_more = false;
                else
                    throw imap_error("Getting statistics failure.", "Line=`" + line + "`.");
            }
            else
                throw imap_error("Parsing failure.", "Tag=`" + parsed_line.tag + "`.");
        }
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
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
    dlg_->send(format(cmd));

    bool has_more = true;
    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = dlg_->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);

            if (parsed_line.tag == UNTAGGED_RESPONSE)
            {
                parse_response(parsed_line.response);

                if (mandatory_part_.empty())
                    throw imap_error("No mandatory part.", "Response=`" + parsed_line.response + "`.");
                auto msg_no_token = mandatory_part_.front();
                mandatory_part_.pop_front();

                if (mandatory_part_.empty())
                    throw imap_error("No mandatory part.", "Response=`" + parsed_line.response + "`.");
                auto fetch_token = mandatory_part_.front();
                if (!iequals(fetch_token->atom, "FETCH"))
                    throw imap_error("Parsing failure.", "Tag=`" + fetch_token->atom + "`.");
                mandatory_part_.pop_front();

                // Check the list with flags.

                if (mandatory_part_.empty())
                    throw imap_error("No mandatory part.", "Response=`" + parsed_line.response + "`.");
                auto flags_token_list = mandatory_part_.front();
                if (flags_token_list->token_type != response_token_t::token_type_t::LIST)
                    throw imap_error("Expecting the list.", "Line=`" + line + "`.");

                std::shared_ptr<response_token_t> uid_token = nullptr;
                auto uid_token_it = flags_token_list->parenthesized_list.begin();
                do
                    if (iequals((*uid_token_it)->atom, "UID"))
                    {
                        uid_token_it++;
                        if (uid_token_it == flags_token_list->parenthesized_list.end())
                            throw imap_error("No UID.", "");
                        uid_token = *uid_token_it;
                        break;
                    }
                    else
                        uid_token_it++;
                while (uid_token_it != flags_token_list->parenthesized_list.end());

                if (is_uid)
                {
                    if (uid_token == nullptr)
                        throw imap_error("No UID.", "");
                    msg_no_token = uid_token;
                }

                if (msg_no_token->token_type != response_token_t::token_type_t::ATOM || stoul(msg_no_token->atom) != message_no)
                    throw imap_error("Deleting message failure.", "");

                continue;
            }
            else if (parsed_line.tag == to_string(tag_))
            {
                if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
                    throw imap_error("Deleting message failure.", "");
                else
                {
                    reset_response_parser();
                    dlg_->send(format("CLOSE"));
                    string line = dlg_->receive();
                    tag_result_response_t parsed_line = parse_tag_result(line);

                    if (!iequals(parsed_line.tag, to_string(tag_)))
                        throw imap_error("Incorrect tag.", "Tag=`" + parsed_line.tag + "`.");
                    if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
                        throw imap_error("Deleting message failure.", "");
                }
                has_more = false;
            }
            else
                throw imap_error("Incorrect tag.", "Tag=`" + parsed_line.tag + "`.");
        }
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
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
    dlg_->send(format("CREATE " + to_astring(folder_name)));

    string line = dlg_->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag != to_string(tag_))
        throw imap_error("Incorrect tag.", "Tag=`" + parsed_line.tag + "`.");
    if (parsed_line.result.value() == tag_result_response_t::NO)
        return false;
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Creating folder failure.", "Response=`" + parsed_line.response + "`.");
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
    dlg_->send(format("LIST " + QUOTED_STRING_SEPARATOR + QUOTED_STRING_SEPARATOR + TOKEN_SEPARATOR_STR + to_astring(folder_name + "*")));
    mailbox_folder_t mailboxes;

    bool has_more = true;
    try
    {
        while (has_more)
        {
            string line = dlg_->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);
            parse_response(parsed_line.response);
            if (parsed_line.tag == UNTAGGED_RESPONSE)
            {
                auto token = mandatory_part_.front();
                mandatory_part_.pop_front();
                if (!iequals(token->atom, "LIST"))
                    throw imap_error("Expecting the list atom.", "Atom=`" + token->atom + "`.");

                if (mandatory_part_.size() < 3)
                    throw imap_error("Listing folders failure.", "");
                auto found_folder = mandatory_part_.begin();
                found_folder++; found_folder++;
                if (found_folder != mandatory_part_.end() && (*found_folder)->token_type == response_token_t::token_type_t::ATOM)
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
                    throw imap_error("Parsing failure.", "Line=`" + line + "`.");
            }
            else if (parsed_line.tag == to_string(tag_))
            {
                has_more = false;
            }
            reset_response_parser();
        }
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
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
    dlg_->send(format("DELETE " + to_astring(folder_name)));

    string line = dlg_->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag != to_string(tag_))
        throw imap_error("Incorrect tag.", "Tag=`" + parsed_line.tag + "`.");
    if (parsed_line.result.value() == tag_result_response_t::NO)
        return false;
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Deleting folder failure.", "Line=`" + line + "`.");
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
    dlg_->send(format("RENAME " + to_astring(old_name) + TOKEN_SEPARATOR_STR + to_astring(new_name)));

    string line = dlg_->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag != to_string(tag_))
        throw imap_error("Incorrect tag.", "Tag=`" + parsed_line.tag + "`.");
    if (parsed_line.result.value() == tag_result_response_t::NO)
        return false;
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Renaming folder failure.", "Line=`" + line + "`.");
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
    string line = dlg_->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);

    if (parsed_line.tag != UNTAGGED_RESPONSE)
        throw imap_error("Incorrect tag.", "Tag=`" + parsed_line.tag + "`.");
    if (!parsed_line.result.has_value() || parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Connection to server failure.", "Line=`" + line + "`.");
    return parsed_line.response;
}


void imap::auth_login(const string& username, const string& password)
{
    auto user_esc = to_astring(username);
    auto pass_esc = to_astring(password);
    auto cmd = format("LOGIN " + user_esc + TOKEN_SEPARATOR_STR + pass_esc);
    dlg_->send(cmd);

    bool has_more = true;
    while (has_more)
    {
        string line = dlg_->receive();
        tag_result_response_t parsed_line = parse_tag_result(line);

        if (parsed_line.tag == UNTAGGED_RESPONSE)
            continue;
        if (parsed_line.tag != to_string(tag_))
            throw imap_error("Incorrect tag.", "Tag=`" + parsed_line.tag + "`.");
        if (parsed_line.result.value() != tag_result_response_t::OK)
            throw imap_error("Authentication failure.", "line=`" + line + "`.");

        has_more = false;
    }
}


void imap::search(const string& conditions, list<unsigned long>& results, bool want_uids)
{
    string cmd;
    if (want_uids)
        cmd.append("UID ");
    cmd.append("SEARCH " + conditions);
    dlg_->send(format(cmd));

    bool has_more = true;
    try
    {
        while (has_more)
        {
            reset_response_parser();
            string line = dlg_->receive();
            tag_result_response_t parsed_line = parse_tag_result(line);
            if (parsed_line.tag == UNTAGGED_RESPONSE)
            {
                parse_response(parsed_line.response);

                auto search_token = mandatory_part_.front();
                // ignore other responses, although not sure whether this is by the rfc or not
                if (search_token->token_type == response_token_t::token_type_t::ATOM && !iequals(search_token->atom, "SEARCH"))
                    continue;
                mandatory_part_.pop_front();

                for (auto it = mandatory_part_.begin(); it != mandatory_part_.end(); it++)
                    if ((*it)->token_type == response_token_t::token_type_t::ATOM)
                    {
                        const unsigned long idx = stoul((*it)->atom);
                        if (idx == 0)
                            throw imap_error("Incorrect message id.", "Line=`" + line + "`.");
                        results.push_back(idx);
                    }
            }
            else if (parsed_line.tag == to_string(tag_))
            {
                if (parsed_line.result.value() != tag_result_response_t::OK)
                    throw imap_error("Search mailbox failure.", "Line=`" + line + "`.");

                has_more = false;
            }
            else
            {
                throw imap_error("Incorrect tag parsed.", "Tag=`" + parsed_line.tag + "`.");
            }
        }
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
    }
    reset_response_parser();
}


string imap::folder_delimiter()
{
    try
    {
        if (folder_delimiter_.empty())
        {
            dlg_->send(format("LIST " + QUOTED_STRING_SEPARATOR + QUOTED_STRING_SEPARATOR + TOKEN_SEPARATOR_STR + QUOTED_STRING_SEPARATOR + QUOTED_STRING_SEPARATOR));
            bool has_more = true;
            while (has_more)
            {
                string line = dlg_->receive();
                tag_result_response_t parsed_line = parse_tag_result(line);
                if (parsed_line.tag == UNTAGGED_RESPONSE && folder_delimiter_.empty())
                {
                    parse_response(parsed_line.response);
                    if (!iequals(mandatory_part_.front()->atom, "LIST"))
                        throw imap_error("Incorrect atom parsed.", "Line=`" + line + "`.");
                    mandatory_part_.pop_front();

                    if (mandatory_part_.size() < 3)
                        throw imap_error("Determining folder delimiter failure.", "");
                    auto it = mandatory_part_.begin();
                    if ((*(++it))->token_type != response_token_t::token_type_t::ATOM)
                        throw imap_error("Incorrect atom parsed.", "");
                    folder_delimiter_ = trim_copy_if((*it)->atom, [](char c ){ return c == QUOTED_STRING_SEPARATOR_CHAR; });
                    reset_response_parser();
                }
                else if (parsed_line.tag == to_string(tag_))
                {
                    if (parsed_line.result.value() != tag_result_response_t::OK)
                        throw imap_error("Determining folder delimiter failure.", "Line=`" + line + "`.");

                    has_more = false;
                }
            }
        }
        return folder_delimiter_;
    }
    catch (const invalid_argument& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
    }
    catch (const out_of_range& exc)
    {
        throw imap_error("Parsing failure.", exc.what());
    }
}


auto imap::parse_tag_result(const string& line) const -> tag_result_response_t
{
    string::size_type tag_pos = line.find(TOKEN_SEPARATOR_STR);
    if (tag_pos == string::npos)
        throw imap_error("Parsing failure.", "");
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

    if (literal_state_ == string_literal_state_t::READING)
    {
        token_list = optional_part_state_ ? find_last_token_list(optional_part_) : find_last_token_list(mandatory_part_);
        if (token_list->back()->token_type == response_token_t::token_type_t::LITERAL && literal_bytes_read_ > token_list->back()->literal.size())
            throw imap_error("Parser failure.", "");
        unsigned long literal_size = stoul(token_list->back()->literal_size);
        if (literal_bytes_read_ + response.size() < literal_size)
        {
            token_list->back()->literal += response + codec::END_OF_LINE;
            literal_bytes_read_ += response.size() + eols_no_;
            if (literal_bytes_read_ == literal_size)
                literal_state_ = string_literal_state_t::DONE;
            return;
        }
        else
        {
            string::size_type resp_len = response.size();
            token_list->back()->literal += response.substr(0, literal_size - literal_bytes_read_);
            literal_bytes_read_ += literal_size - literal_bytes_read_;
            literal_state_ = string_literal_state_t::DONE;
            parse_response(response.substr(resp_len - (literal_size - literal_bytes_read_) - 1));
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
                if (atom_state_ == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (optional_part_state_)
                        throw imap_error("Parser failure.", "");

                    optional_part_state_ = true;
                }
            }
            break;

            case OPTIONAL_END:
            {
                if (atom_state_ == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (!optional_part_state_)
                        throw imap_error("Parser failure.", "");

                    optional_part_state_ = false;
                    atom_state_ = atom_state_t::NONE;
                }
            }
            break;

            case LIST_BEGIN:
            {
                if (atom_state_ == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    cur_token = make_shared<response_token_t>();
                    cur_token->token_type = response_token_t::token_type_t::LIST;
                    token_list = optional_part_state_ ? find_last_token_list(optional_part_) : find_last_token_list(mandatory_part_);
                    token_list->push_back(cur_token);
                    parenthesis_list_counter_++;
                    atom_state_ = atom_state_t::NONE;
                }
            }
            break;

            case LIST_END:
            {
                if (atom_state_ == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (parenthesis_list_counter_ == 0)
                        throw imap_error("Parser failure.", "");

                    parenthesis_list_counter_--;
                    atom_state_ = atom_state_t::NONE;
                }
            }
            break;

            case STRING_LITERAL_BEGIN:
            {
                if (atom_state_ == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (literal_state_ == string_literal_state_t::SIZE)
                        throw imap_error("Parser failure.", "");

                    cur_token = make_shared<response_token_t>();
                    cur_token->token_type = response_token_t::token_type_t::LITERAL;
                    token_list = optional_part_state_ ? find_last_token_list(optional_part_) : find_last_token_list(mandatory_part_);
                    token_list->push_back(cur_token);
                    literal_state_ = string_literal_state_t::SIZE;
                    atom_state_ = atom_state_t::NONE;
                }
            }
            break;

            case STRING_LITERAL_END:
            {
                if (atom_state_ == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (literal_state_ == string_literal_state_t::NONE)
                        throw imap_error("Parser failure.", "");

                    literal_state_ = string_literal_state_t::WAITING;
                }
            }
            break;

            case TOKEN_SEPARATOR_CHAR:
            {
                if (atom_state_ == atom_state_t::QUOTED)
                    cur_token->atom +=ch;
                else
                {
                    if (cur_token != nullptr)
                    {
                        trim(cur_token->atom);
                        atom_state_ = atom_state_t::NONE;
                    }
                }
            }
            break;

            case QUOTED_ATOM:
            {
                if (atom_state_ == atom_state_t::NONE)
                {
                    cur_token = make_shared<response_token_t>();
                    cur_token->token_type = response_token_t::token_type_t::ATOM;
                    token_list = optional_part_state_ ? find_last_token_list(optional_part_) : find_last_token_list(mandatory_part_);
                    token_list->push_back(cur_token);
                    atom_state_ = atom_state_t::QUOTED;
                }
                else if (atom_state_ == atom_state_t::QUOTED)
                {
                    // The backslash and a double quote within an atom is the double quote only.
                    if (token_list->back()->atom.back() != codec::BACKSLASH_CHAR)
                        atom_state_ = atom_state_t::NONE;
                    else
                        token_list->back()->atom.back() = ch;
                }
            }
            break;

            default:
            {
                // Double backslash in an atom is translated to the single backslash.
                if (ch == codec::BACKSLASH_CHAR && atom_state_ == atom_state_t::QUOTED && token_list->back()->atom.back() == codec::BACKSLASH_CHAR)
                    break;

                if (literal_state_ == string_literal_state_t::SIZE)
                {
                    if (!isdigit(ch))
                        throw imap_error("Parser failure.", "");

                    cur_token->literal_size += ch;
                }
                else if (literal_state_ == string_literal_state_t::WAITING)
                {
                    // no characters allowed after the right brace, crlf is required
                    throw imap_error("Parser failure.", "");
                }
                else
                {
                    if (atom_state_ == atom_state_t::NONE)
                    {
                        cur_token = make_shared<response_token_t>();
                        cur_token->token_type = response_token_t::token_type_t::ATOM;
                        token_list = optional_part_state_ ? find_last_token_list(optional_part_) : find_last_token_list(mandatory_part_);
                        token_list->push_back(cur_token);
                        atom_state_ = atom_state_t::PLAIN;
                    }
                    cur_token->atom += ch;
                }
            }
        }
    }

    if (literal_state_ == string_literal_state_t::WAITING)
        literal_state_ = string_literal_state_t::READING;
}

void imap::reset_response_parser()
{
    optional_part_.clear();
    mandatory_part_.clear();
    optional_part_state_ = false;
    atom_state_ = atom_state_t::NONE;
    parenthesis_list_counter_ = 0;
    literal_state_ = string_literal_state_t::NONE;
    literal_bytes_read_ = 0;
    eols_no_ = 2;
}


string imap::format(const string& command)
{
    return to_string(++tag_) + TOKEN_SEPARATOR_STR + command;
}


void imap::trim_eol(string& line)
{
    if (line.length() >= 1 && line[line.length() - 1] == codec::END_OF_LINE[0])
    {
        eols_no_ = 2;
        line.pop_back();
    }
    else
        eols_no_ = 1;
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
    while (!list_ptr->empty() && list_ptr->back()->token_type == response_token_t::token_type_t::LIST && depth <= parenthesis_list_counter_)
    {
        list_ptr = &(list_ptr->back()->parenthesized_list);
        depth++;
    }
    return list_ptr;
}


imaps::imaps(const string& hostname, unsigned port, milliseconds timeout) : imap(hostname, port, timeout)
{
    ssl_options_ =
        {
            boost::asio::ssl::context::sslv23,
            boost::asio::ssl::verify_none
        };
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


void imaps::ssl_options(const dialog_ssl::ssl_options_t& options)
{
    ssl_options_ = options;
}


void imaps::start_tls()
{
    dlg_->send(format("STARTTLS"));
    string line = dlg_->receive();
    tag_result_response_t parsed_line = parse_tag_result(line);
    if (parsed_line.tag == UNTAGGED_RESPONSE)
        throw imap_error("Bad server response.", "");
    if (parsed_line.result.value() != tag_result_response_t::OK)
        throw imap_error("Start TLS refused by server.", "");

    switch_to_ssl();
}


void imaps::switch_to_ssl()
{
    dlg_ = std::make_shared<dialog_ssl>(*dlg_, ssl_options_);
}


imap_error::imap_error(const string& msg, const string& details) : dialog_error(msg, details)
{
}


imap_error::imap_error(const char* msg, const string& details) : dialog_error(msg, details)
{
}


} // namespace mailio
