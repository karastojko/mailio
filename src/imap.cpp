/*

imap.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/
 

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <mailio/imap.hpp>


using std::find_if;
using std::invalid_argument;
using std::list;
using std::make_shared;
using std::make_tuple;
using std::map;
using std::move;
using std::out_of_range;
using std::shared_ptr;
using std::stoul;
using std::string;
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


// REMOVE Tim. Added.
auto imap::select_mailbox(const std::list<std::string>& folder_name) -> mailbox_stat_t
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    return select(folder_name_s);
}

// REMOVE Tim. Added.
auto imap::examine_mailbox(const std::list<std::string>& folder_name) -> mailbox_stat_t
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    return examine(folder_name_s);
}


// REMOVE Tim. Changed.
// // fetching literal is the only place where line is ended with LF only, instead of CRLF; thus, `receive(true)` and counting EOLs is performed
// void imap::fetch(const string& mailbox, unsigned long message_no, message& msg, bool header_only)
// {
//     const string RFC822_TOKEN = string("RFC822") + (header_only ? ".HEADER" : "");
//     select(mailbox);
//     _dlg->send(format("FETCH " + to_string(message_no) + " " + RFC822_TOKEN));
// 
//     bool has_more = true;
//     while (has_more)
//     {
//         string line = _dlg->receive();
//         tuple<string, string, string> tag_result_response = parse_tag_result(line);
//         
//         if (std::get<0>(tag_result_response) == "*")
//         {
//             parse_response(std::get<2>(tag_result_response));
//             shared_ptr<response_token_t> literal_token = nullptr;
//             if (!_mandatory_part.empty())
//                 for (auto part : _mandatory_part)
//                 {
//                     if (part->token_type == response_token_t::token_type_t::LIST)
//                     {
//                         bool rfc_found = false;
//                         for (auto token = part->parenthesized_list.begin(); token != part->parenthesized_list.end(); token++)
//                         {
//                             if ((*token)->token_type == response_token_t::token_type_t::ATOM && iequals((*token)->atom, RFC822_TOKEN))
//                             {
//                                 rfc_found = true;
//                                 continue;
//                             }
//                             
//                             if ((*token)->token_type == response_token_t::token_type_t::LITERAL)
//                                 if (rfc_found)
//                                 {
//                                     literal_token = *token;
//                                     break;
//                                 }
//                                 else
//                                     throw imap_error("Parsing failure.");
//                         }
//                     }
//                 }
// 
//             if (literal_token != nullptr)
//             {
//                 // loop to read string literal
//                 while (_literal_state == string_literal_state_t::READING)
//                 {
//                     string line = _dlg->receive(true);
//                     if (!line.empty())
//                         trim_eol(line);
//                     parse_response(line);
//                 }
//                 // closing parenthesis not yet read
//                 if (_literal_state == string_literal_state_t::DONE && _parenthesis_list_counter > 0)
//                 {
//                     string line = _dlg->receive(true);
//                     if (!line.empty())
//                         trim_eol(line);
//                     parse_response(line);
//                 }
//                 msg.parse(literal_token->literal);
//             }
//             else
//                 throw imap_error("Parsing failure.");
//             reset_response_parser();
//         }
//         else if (std::get<0>(tag_result_response) == to_string(_tag))
//         {
//             if (iequals(std::get<1>(tag_result_response), "OK"))
//                 has_more = false;
//             else
//                 throw imap_error("Fetching message failure.");
//         }
//         else
//             throw imap_error("Parsing failure.");
//     }
//         
//     reset_response_parser();
// }

// fetching literal is the only place where line is ended with LF only, instead of CRLF; thus, `receive(true)` and counting EOLs is performed
void imap::fetch(const string& mailbox, unsigned long message_no, message& msg, bool header_only)
{
    select(mailbox);
    fetch_message(message_no, msg, header_only);
}

// REMOVE Tim. Added. Do not select mailbox every time. Added select_mailbox() method for that.
// fetching literal is the only place where line is ended with LF only, instead of CRLF; thus, `receive(true)` and counting EOLs is performed
void imap::fetch_message(unsigned long message_no, message& msg, bool header_only, bool is_uid)
{
    const string RFC822_TOKEN = string("RFC822") + (header_only ? ".HEADER" : "");
    //_dlg->send(format("FETCH " + to_string(message_no) + " " + RFC822_TOKEN));

    string cmd;
    if(is_uid)
      cmd.append("UID ");
    cmd.append("FETCH " + to_string(message_no) + " " + RFC822_TOKEN);
    _dlg->send(format(cmd));
    
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        
        if (std::get<0>(tag_result_response) == "*")
        {
// REMOVE Tim. Added. Hm, this response has four parts and the third is FETCH,
//              but tag_result_response is a tuple and there are only three items,
//              and all of the libray code wants to parse starting at the third item.
//             So in this case only, we are including the word FETCH in the parse,
//              which the code below deems harmless because the FETCH is just a string atom
//              and the code below just looks for the LIST token.
//             Nonetheless, the word FETCH will be the first atom returned in the mandatory list.
//             We could look for the word there. But then for consistency we should change
//              all the other code to do the same (that is, include the resonse word).
//
//             if (stoul(std::get<1>(tag_result_response)) != message_no)
//                 throw imap_error("Fetching message failure.");
// 
//             if (!iequals(std::get<2>(tag_result_response), "FETCH"))
//                 throw imap_error("Fetching message failure.");

            parse_response(std::get<2>(tag_result_response));
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
            reset_response_parser();
        }
        else if (std::get<0>(tag_result_response) == to_string(_tag))
        {
            if (iequals(std::get<1>(tag_result_response), "OK"))
                has_more = false;
            else
                throw imap_error("Fetching message failure.");
        }
        else
            throw imap_error("Parsing failure.");
    }
        
    reset_response_parser();
}

// REMOVE Tim. Changed.
// auto imap::statistics(const string& mailbox) -> mailbox_stat_t
// {
//     _dlg->send(format("STATUS " + mailbox + " (messages)"));
//     mailbox_stat_t stat;
//     
//     bool has_more = true;
//     while (has_more)
//     {
//         string line = _dlg->receive();
//         tuple<string, string, string> tag_result_response = parse_tag_result(line);
//         
//         if (std::get<0>(tag_result_response) == "*")
//         {
//             parse_response(std::get<2>(tag_result_response));
//             
//             bool stat_found = false;
//             for (auto it = _mandatory_part.begin(); it != _mandatory_part.end(); it++)
//                 if ((*it)->token_type == response_token_t::token_type_t::LIST && (*it)->parenthesized_list.size() >= 2)
//                 {
//                     string msg = (*it)->parenthesized_list.front()->atom;
//                     if (iequals(msg, "MESSAGES"))
//                     {
//                         stat.messages_no = stoul((*it)->parenthesized_list.back()->atom);
//                         stat_found = true;
//                         break;
//                     }
//                 }
//             if (!stat_found)
//                 throw imap_error("Parsing failure.");
//             reset_response_parser();
//         }
//         else if (std::get<0>(tag_result_response) == to_string(_tag))
//         {
//             if (iequals(std::get<1>(tag_result_response), "OK"))
//                 has_more = false;
//             else
//                 throw imap_error("Getting statistics failure.");
//         }
//         else
//             throw imap_error("Parsing failure.");
//     }
//     reset_response_parser();
//     return stat;
// }
auto imap::statistics(const string& mailbox) -> mailbox_stat_t
{
    // TODO: It doesn't like search terms it doens't recognize. "2 BAD ...".
    //       Some older protocol versions or some servers may not support them.
    //       Pass a flag indicating whether we want to try "unseen uidnext uidvalidity".
    _dlg->send(format("STATUS \"" + mailbox + "\" (messages recent unseen uidnext uidvalidity)"));
    mailbox_stat_t stat;
    
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        
        if (std::get<0>(tag_result_response) == "*")
        {
// REMOVE Tim. Added.
//             if (!iequals(std::get<1>(tag_result_response), "STATUS"))
//                 throw imap_error("Getting statistics failure.");
            
            parse_response(std::get<2>(tag_result_response));
            
            bool mess_found = false, recent_found = false;
            for (auto it = _mandatory_part.begin(); it != _mandatory_part.end(); it++)
                if ((*it)->token_type == response_token_t::token_type_t::LIST && (*it)->parenthesized_list.size() >= 2)
                {
                    bool key_found = false;
                    string key;
                    auto mp = *it;
                    for(auto il = mp->parenthesized_list.begin(); il != mp->parenthesized_list.end(); ++il)
                    {
                      string atm = (*il)->atom;
                      if(key_found)
                      {
                        if(iequals(key, "MESSAGES"))
                        {
                          stat.messages_no = stoul(atm);
                          mess_found = true;
                        }
                        else if(iequals(key, "RECENT"))
                        {
                          stat.messages_recent = stoul(atm);
                          recent_found = true;
                        }
                        else if(iequals(key, "UNSEEN"))
                        {
                          stat.messages_unseen = stoul(atm);
                        }
                        else if(iequals(key, "UIDNEXT"))
                        {
                          stat.uidnext = stoul(atm);
                        }
                        else if(iequals(key, "UIDVALIDITY"))
                        {
                          stat.uidvalidity = stoul(atm);
                        }
                        key_found = false;
                      }
                      else
                      {
                        key = atm;
                        key_found = true;
                      }
                    }
                }
            // The MESSAGES and RECENT are required.
            if (!mess_found || !recent_found)
                throw imap_error("Parsing failure.");
            reset_response_parser();
        }
        else if (std::get<0>(tag_result_response) == to_string(_tag))
        {
            if (iequals(std::get<1>(tag_result_response), "OK"))
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

// REMOVE Tim. Changed.
// void imap::remove(const string& mailbox, unsigned long message_no)
// {
//      select(mailbox);
//     _dlg->send(format("STORE " + to_string(message_no) + " +FLAGS (\\Deleted)"));
//     
//     bool has_more = true;
//     while (has_more)
//     {
//         string line = _dlg->receive();
//         tuple<string, string, string> tag_result_response = parse_tag_result(line);
//         
//         if (std::get<0>(tag_result_response) == "*")
//             continue;
//         else if(std::get<0>(tag_result_response) == to_string(_tag))
//         {
//             if (!iequals(std::get<1>(tag_result_response), "OK"))
//                 throw imap_error("Deleting message failure.");
//             else
//             {
//                 _dlg->send(format("CLOSE"));
//                 string line = _dlg->receive();
//                 
//                 tuple<string, string, string> tag_result_response = parse_tag_result(line);
//                 if (!iequals(std::get<0>(tag_result_response), to_string(_tag)))
//                     throw imap_error("Parsing failure.");
//                 if (!iequals(std::get<1>(tag_result_response), "OK"))
//                     throw imap_error("Deleting message failure.");
//             }
//             has_more = false;
//         }
//         else
//             throw imap_error("Parsing failure.");
//     }
// }
void imap::remove(const string& mailbox, unsigned long message_no)
{
    select(mailbox);
    remove_message(message_no);
}

// REMOVE Tim. Added.
void imap::remove_message(unsigned long message_no, bool is_uid)
{
    //_dlg->send(format("STORE " + to_string(message_no) + " +FLAGS (\\Deleted)"));

    string cmd;
    if(is_uid)
      cmd.append("UID ");
    cmd.append("STORE " + to_string(message_no) + " +FLAGS (\\Deleted)");
    _dlg->send(format(cmd));

    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        
        if (std::get<0>(tag_result_response) == "*")
        {
// REMOVE Tim. Added.
//             // Yes, that's FETCH.
//             if (!iequals(std::get<1>(tag_result_response), "FETCH"))
//                 throw imap_error("Deleting message failure.");

            continue;
        }
        else if(std::get<0>(tag_result_response) == to_string(_tag))
        {
            if (!iequals(std::get<1>(tag_result_response), "OK"))
                throw imap_error("Deleting message failure.");
            else
            {
                _dlg->send(format("CLOSE"));
                string line = _dlg->receive();
                
                tuple<string, string, string> tag_result_response = parse_tag_result(line);
                if (!iequals(std::get<0>(tag_result_response), to_string(_tag)))
                    throw imap_error("Parsing failure.");
                if (!iequals(std::get<1>(tag_result_response), "OK"))
                    throw imap_error("Deleting message failure.");
            }
            has_more = false;
        }
        else
            throw imap_error("Parsing failure.");
    }
}

bool imap::create_folder(const list<string>& folder_tree)
{
    string delim = folder_delimiter();
    string folder_str = folder_tree_to_string(folder_tree, delim);
    _dlg->send(format("CREATE \"" + folder_str + "\""));

    string line = _dlg->receive();
    tuple<string, string, string> tag_result_response = parse_tag_result(line);
    if (std::get<0>(tag_result_response) != to_string(_tag))
        throw imap_error("Parsing failure.");
    if (iequals(std::get<1>(tag_result_response), "NO"))
        return false;
    if (!iequals(std::get<1>(tag_result_response), "OK"))
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
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        if (std::get<0>(tag_result_response) == "*")
        {
// REMOVE Tim. Added.
//             if (!iequals(std::get<1>(tag_result_response), "LIST"))
//                 throw imap_error("Listing folders failure.");

            parse_response(std::get<2>(tag_result_response));
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
            reset_response_parser();
        }
        else if(std::get<0>(tag_result_response) == to_string(_tag))
        {
            has_more = false;
        }
    }

    return mailboxes;
}


bool imap::delete_folder(const list<string>& folder_name)
{
    string delim = folder_delimiter();
    string folder_name_s = folder_tree_to_string(folder_name, delim);
    _dlg->send(format("DELETE \"" + folder_name_s + "\""));

    string line = _dlg->receive();
    tuple<string, string, string> tag_result_response = parse_tag_result(line);
    if (std::get<0>(tag_result_response) != to_string(_tag))
        throw imap_error("Parsing failure.");
    if (iequals(std::get<1>(tag_result_response), "NO"))
        return false;
    if (!iequals(std::get<1>(tag_result_response), "OK"))
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
    tuple<string, string, string> tag_result_response = parse_tag_result(line);
    if (std::get<0>(tag_result_response) != to_string(_tag))
        throw imap_error("Parsing failure.");
    if (iequals(std::get<1>(tag_result_response), "NO"))
        return false;
    if (!iequals(std::get<1>(tag_result_response), "OK"))
        throw imap_error("Renaming folder failure.");
    return true;
}


void imap::connect()
{
    // read greetings message
    string line = _dlg->receive();
    tuple<string, string, string> tag_result_response = parse_tag_result(line);
    
    if (std::get<0>(tag_result_response) != "*")
        throw imap_error("Parsing failure.");
    if (!iequals(std::get<1>(tag_result_response), "OK"))
        throw imap_error("Connection to server failure.");
}


void imap::auth_login(const string& username, const string& password)
{
    _dlg->send(format("LOGIN " + username + " " + password));
    
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);

        if (std::get<0>(tag_result_response) == "*")
            continue;
        if (std::get<0>(tag_result_response) != to_string(_tag))
            throw imap_error("Parsing failure.");
        if (!iequals(std::get<1>(tag_result_response), "OK"))
            throw imap_error("Authentication failure.");
    
        has_more = false;
    }
}


// REMOVE Tim. Changed.
// void imap::select(const string& mailbox)
// {
//     _dlg->send(format("SELECT " + mailbox));
// 
//     bool has_more = true;
//     while (has_more)
//     {
//         string line = _dlg->receive();
//         tuple<string, string, string> tag_result_response = parse_tag_result(line);
//         if (std::get<0>(tag_result_response) == "*")
//             continue;
//         else if (std::get<0>(tag_result_response) == to_string(_tag))
//         {
//             if (!iequals(std::get<1>(tag_result_response), "OK"))
//                 throw imap_error("Selecting mailbox failure.");
//  
//             has_more = false;
//         }
//         else
//             throw imap_error("Parsing failure.");
//     }
// }
auto imap::select(const string& mailbox) -> mailbox_stat_t
{
    _dlg->send(format("SELECT \"" + mailbox + "\""));
    return parse_select_or_examine();
}

// REMOVE Tim. Added.
auto imap::examine(const string& mailbox) -> mailbox_stat_t
{
    _dlg->send(format("EXAMINE \"" + mailbox + "\""));
    return parse_select_or_examine();
}

// REMOVE Tim. Added.
void imap::search_messages(const string& keys, std::list<unsigned long>& result_list, bool want_uids)
{
  search(keys, result_list, want_uids);
}

// REMOVE Tim. Added.
void imap::search(const string& keys, std::list<unsigned long>& result_list, bool want_uids)
{
    string cmd;
    if(want_uids)
      cmd.append("UID ");
    cmd.append("SEARCH " + keys);
    _dlg->send(format(cmd));

    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        if (std::get<0>(tag_result_response) == "*")
        {
// REMOVE Tim. Added.
//             if (!iequals(std::get<1>(tag_result_response), "SEARCH"))
//                 throw imap_error("Search mailbox failure.");
            
            parse_response(std::get<2>(tag_result_response));
            
            //unsigned long idxs_found = 0;
            for (auto it = _mandatory_part.begin(); it != _mandatory_part.end(); it++)
                //if ((*it)->token_type == response_token_t::token_type_t::LIST && (*it)->parenthesized_list.size() >= 1)
                if ((*it)->token_type == response_token_t::token_type_t::ATOM)
                {
                    //auto mp = *it;
                    //for(auto il = mp->parenthesized_list.begin(); il != mp->parenthesized_list.end(); ++il)
                    {
                      //const unsigned long idx = stoul((*il)->atom);
                      const unsigned long idx = stoul((*it)->atom);
                      if(idx == 0)
                      {
                        throw imap_error("Parsing failure.");
                      }
                      //++idxs_found;
                      result_list.push_back(idx);
                    }
                }
            reset_response_parser();
        }
        else if (std::get<0>(tag_result_response) == to_string(_tag))
        {
            if (!iequals(std::get<1>(tag_result_response), "OK"))
                throw imap_error("Search mailbox failure.");
 
            has_more = false;
        }
        else
            throw imap_error("Parsing failure.");
    }
}

string imap::folder_delimiter()
{
    string delimiter;
    _dlg->send(format("LIST \"\" \"\""));
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        if (std::get<0>(tag_result_response) == "*" && delimiter.empty())
        {
// REMOVE Tim. Added.
//             if (!iequals(std::get<1>(tag_result_response), "LIST"))
//                 throw imap_error("Determining folder delimiter failure.");

            parse_response(std::get<2>(tag_result_response));
            if (_mandatory_part.size() < 3)
                throw imap_error("Determining folder delimiter failure.");
            auto it = _mandatory_part.begin();
            if ((*(++it))->token_type != response_token_t::token_type_t::ATOM)
                throw imap_error("Determining folder delimiter failure.");
            delimiter = trim_copy_if((*it)->atom, [](char c ){ return c == codec::QUOTE_CHAR; });
            reset_response_parser();
        }
        else if (std::get<0>(tag_result_response) == to_string(_tag))
        {
            if (!iequals(std::get<1>(tag_result_response), "OK"))
                throw imap_error("Determining folder delimiter failure.");

            has_more = false;
        }
    }
    return delimiter;
}


// REMOVE Tim. Added.
auto imap::parse_select_or_examine() -> mailbox_stat_t
{
    mailbox_stat_t stat;
    bool exists_found = false;
    bool recent_found = false;
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        if (std::get<0>(tag_result_response) == "*")
        {
            const string code = std::get<1>(tag_result_response);
            if (iequals(code, "OK"))
            {
                parse_response(std::get<2>(tag_result_response));

                bool key_found = false;
                string key;
                for (auto it = _optional_part.begin(); it != _optional_part.end(); it++)
                    if ((*it)->token_type == response_token_t::token_type_t::ATOM)
                    {
                        string atm = (*it)->atom;
                        if(key_found)
                        {
                          if(iequals(key, "UNSEEN"))
                          {
                            stat.messages_first_unseen = stoul(atm);
                          }
                          else if(iequals(key, "UIDNEXT"))
                          {
                            stat.uidnext = stoul(atm);
                          }
                          else if(iequals(key, "UIDVALIDITY"))
                          {
                            stat.uidvalidity = stoul(atm);
                          }
                          key_found = false;
                        }
                        else
                        {
                          key = atm;
                          key_found = true;
                        }
                    }
                reset_response_parser();
            }
            else if (iequals(code, "FLAGS"))
            {

            }
            else
            {
              const string code2 = std::get<2>(tag_result_response);
              if (iequals(code2, "EXISTS"))
              {
                stat.messages_no = stoul(code);
                exists_found = true;
              }
              else if (iequals(code2, "RECENT"))
              {
                stat.messages_recent = stoul(code);
                recent_found = true;
              }
            }

            continue;
        }
        else if (std::get<0>(tag_result_response) == to_string(_tag))
        {
            if (!iequals(std::get<1>(tag_result_response), "OK"))
                throw imap_error("Select or examine mailbox failure.");

            has_more = false;
        }
        else
            throw imap_error("Parsing failure.");
    }

    // The EXISTS and RECENT are required, the others may be missing in earlier protocol versions.
    if(!exists_found || !recent_found)
        throw imap_error("Parsing failure.");

    return stat;
}





tuple<string, string, string> imap::parse_tag_result(const string& line) const
{
    string::size_type tag_pos = line.find(' ');
    if (tag_pos == string::npos)
        throw imap_error("Parsing failure.");
    
    string::size_type result_pos = line.find(' ', tag_pos + 1);
    if (result_pos == string::npos)
        throw imap_error("Parsing failure.");
    
    return make_tuple(line.substr(0, tag_pos), line.substr(tag_pos + 1, result_pos - tag_pos - 1), line.substr(result_pos + 1));
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
        if (elem++ < folder_tree.size())
            folders += f + delimiter;
        else
            folders += f;
    return folders;
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
    tuple<string, string, string> tag_result_response = parse_tag_result(line);    
    if (std::get<0>(tag_result_response) == "*")
        throw imap_error("Bad server response.");
    if (!iequals(std::get<1>(tag_result_response), "OK"))
        throw imap_error("Start TLS refused by server.");

    switch_to_ssl();
}


void imaps::switch_to_ssl()
{
    dialog* d = _dlg.get();
    _dlg.reset(new dialog_ssl(move(*d)));
}


} // namespace mailio
