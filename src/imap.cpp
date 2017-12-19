/*

imap.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/
 

#include <iostream>
#include <string>
#include <tuple>
#include <algorithm>
#include <memory>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <imap.hpp>


using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::stoul;
using std::vector;
using std::tuple;
using std::make_tuple;
using std::move;
using std::list;
using std::shared_ptr;
using std::make_shared;
using std::out_of_range;
using std::invalid_argument;
using boost::system::system_error;
using boost::iequals;
using boost::regex;
using boost::regex_match;
using boost::smatch;
using boost::trim;
using boost::algorithm::trim_if;
using boost::algorithm::is_any_of;


namespace mailio
{


imap::imap(const string& hostname, unsigned port) : _dlg(new dialog(hostname, port)), _tag(0), _optional_part_state(false), _atom_state(false),
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


// fetching literal is the only place where line is ended with LF only, instead of CRLF; thus, `receive_raw()` and counting EOLs is performed
void imap::fetch(const string& mailbox, unsigned long message_no, message& msg)
{
    select(mailbox);
    _dlg->send(format("FETCH " + to_string(message_no) + " RFC822"));

    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        
        if (std::get<0>(tag_result_response) == "*")
        {
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
                            if ((*token)->token_type == response_token_t::token_type_t::ATOM && iequals((*token)->atom, "RFC822"))
                            {
                                rfc_found = true;
                                continue;
                            }
                            
                            if ((*token)->token_type == response_token_t::token_type_t::LITERAL)
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

            if (literal_token != nullptr)
            {
                // loop to read string literal
                while (_literal_state == string_literal_state_t::READING)
                {
                    string line = _dlg->receive_raw();
                    if (!line.empty())
                        trim_eol(line);
                    parse_response(line);
                }
                // closing parenthesis not yet read
                if (_literal_state == string_literal_state_t::DONE && _parenthesis_list_counter > 0)
                {
                    string line = _dlg->receive_raw();
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


auto imap::statistics(const string& mailbox) -> mailbox_stat_t
{
    _dlg->send(format("STATUS " + mailbox + " (messages)"));
    mailbox_stat_t stat;
    
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        
        if (std::get<0>(tag_result_response) == "*")
        {
            parse_response(std::get<2>(tag_result_response));
            
            bool stat_found = false;
            for (auto it = _mandatory_part.begin(); it != _mandatory_part.end(); it++)
                if ((*it)->token_type == response_token_t::token_type_t::LIST && (*it)->parenthesized_list.size() >= 2)
                {
                    string msg = (*it)->parenthesized_list.front()->atom;
                    if (iequals(msg, "MESSAGES"))
                    {
                        stat.messages_no = stoul((*it)->parenthesized_list.back()->atom);
                        stat_found = true;
                        break;
                    }
                }
            if (!stat_found)
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


void imap::remove(const string& mailbox, unsigned long message_no)
{
    select(mailbox);
    _dlg->send(format("STORE " + to_string(message_no) + " +FLAGS (\\Deleted)"));
    
    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        
        if (std::get<0>(tag_result_response) == "*")
            continue;
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


void imap::select(const string& mailbox)
{
    _dlg->send(format("SELECT " + mailbox));

    bool has_more = true;
    while (has_more)
    {
        string line = _dlg->receive();
        
        tuple<string, string, string> tag_result_response = parse_tag_result(line);
        if (std::get<0>(tag_result_response) == "*")
            continue;
        else if (std::get<0>(tag_result_response) == to_string(_tag))
        {
            if (!iequals(std::get<1>(tag_result_response), "OK"))
                throw imap_error("Selecting mailbox failure.");
 
            has_more = false;
        }
        else
            throw imap_error("Parsing failure.");
    }
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
                if (_optional_part_state)
                    throw imap_error("Parser failure.");
                
                _optional_part_state = true;
            }
            break;
        
            case codec::RIGHT_BRACKET_CHAR:
            {
                if (!_optional_part_state)
                    throw imap_error("Parser failure.");
                
                _optional_part_state = false;
                _atom_state = false;
            }
            break;
            
            case codec::LEFT_PARENTHESIS_CHAR:
            {
                cur_token = make_shared<response_token_t>();
                cur_token->token_type = response_token_t::token_type_t::LIST;
                token_list = _optional_part_state ? find_last_token_list(_optional_part) : find_last_token_list(_mandatory_part);
                token_list->push_back(cur_token);
                _parenthesis_list_counter++;
                _atom_state = false;
            }
            break;
            
            case codec::RIGHT_PARENTHESIS_CHAR:
            {
                if (_parenthesis_list_counter == 0)
                    throw imap_error("Parser failure.");
                _parenthesis_list_counter--;
                
            }
            break;
            
            case codec::LEFT_BRACE_CHAR:
            {
                if (_literal_state == string_literal_state_t::SIZE)
                    throw imap_error("Parser failure.");

                cur_token = make_shared<response_token_t>();
                cur_token->token_type = response_token_t::token_type_t::LITERAL;
                token_list = _optional_part_state ? find_last_token_list(_optional_part) : find_last_token_list(_mandatory_part);
                token_list->push_back(cur_token);
                _literal_state = string_literal_state_t::SIZE;
                _atom_state = false;
                
            }
            break;
            
            case codec::RIGHT_BRACE_CHAR:
            {
                if (_literal_state == string_literal_state_t::NONE)
                    throw imap_error("Parser failure.");

                _literal_state = string_literal_state_t::WAITING;
            }
            break;
            
            case codec::SPACE_CHAR:
            {
                if (_parenthesis_list_counter > 0)
                {
                    _atom_state = false;
                    if (cur_token != nullptr)
                        trim(cur_token->atom);
                }
                else
                    cur_token->atom += ch;
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
                    if (!_atom_state)
                    {
                        cur_token = make_shared<response_token_t>();
                        cur_token->token_type = response_token_t::token_type_t::ATOM;
                        token_list = _optional_part_state ? find_last_token_list(_optional_part) : find_last_token_list(_mandatory_part);
                        token_list->push_back(cur_token);
                        _atom_state = true;
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
    _atom_state = false;
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


imaps::imaps(const string& hostname, unsigned port) : imap(hostname, port)
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
