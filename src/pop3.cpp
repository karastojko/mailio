/*

pop3.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <pop3.hpp>


using std::cout;
using std::endl;
using std::istream;
using std::string;
using std::to_string;
using std::vector;
using std::map;
using std::runtime_error;
using std::out_of_range;
using std::invalid_argument;
using std::stoi;
using std::stol;
using std::pair;
using std::make_pair;
using std::tuple;
using std::make_tuple;
using std::move;
using boost::algorithm::trim;
using boost::iequals;


namespace mailio
{


pop3::pop3(const string& hostname, unsigned port) : _dlg(new dialog(hostname, port))
{
}


pop3::~pop3()
{
    try
    {
        _dlg->send("QUIT");
    }
    catch (...)
    {
    }
}


void pop3::authenticate(const string& username, const string& password, auth_method_t method)
{
    connect();
    if (method == auth_method_t::LOGIN)
    {
        auth_login(username, password);
    }
}


auto pop3::list(unsigned message_no) -> message_list_t
{
    message_list_t results;
    try
    {
        if (message_no > 0)
        {
            _dlg->send("LIST " + to_string(message_no));
            string line = _dlg->receive();
            tuple<string, string> stat_msg = parse_status(line);
            if (iequals(std::get<0>(stat_msg), "-ERR"))
                throw pop3_error("Listing message failure.");

            // parse data
            string::size_type pos = std::get<1>(stat_msg).find(' ');
            if (pos == string::npos)
                throw pop3_error("Parser failure.");
            unsigned msg_id = stoi(std::get<1>(stat_msg).substr(0, pos));
            unsigned long msg_size = stol(std::get<1>(stat_msg).substr(pos + 1));
            results[msg_id] = msg_size;
        }
        else
        {
            _dlg->send("LIST");
            string line = _dlg->receive();
            tuple<string, string> stat_msg = parse_status(line);
            if (iequals(std::get<0>(stat_msg), "-ERR"))
                throw pop3_error("Listing all messages failure.");

            // parse data
            bool end_of_msg = false;
            while (!end_of_msg)
            {
                line = _dlg->receive();
                if (line == ".")
                    end_of_msg = true;
                else
                {
                    string::size_type pos = line.find(' ');
                    if (pos == string::npos)
                        throw pop3_error("Parser failure.");
                    unsigned msg_id = stoi(line.substr(0, pos));
                    unsigned long msg_size = stol(line.substr(pos + 1));
                    results[msg_id] = msg_size;
                }
            }
        }
    }
    catch (out_of_range&)
    {
        throw pop3_error("Parser failure.");
    }
    catch (invalid_argument&)
    {
        throw pop3_error("Parser failure.");
    }

    return results;
}


auto pop3::statistics() -> mailbox_stat_t
{
    _dlg->send("STAT");
    string line = _dlg->receive();
    tuple<string, string> stat_msg = parse_status(line);
    if (iequals(std::get<0>(stat_msg), "-ERR"))
        throw pop3_error("Reading statistics failure.");

    // parse data
    string::size_type pos = std::get<1>(stat_msg).find(' ');
    if (pos == string::npos)
        throw pop3_error("Parser failure.");
    mailbox_stat_t mailbox_stat;
    try
    {
        mailbox_stat.messages_no = stoul(std::get<1>(stat_msg).substr(0, pos));
        mailbox_stat.mailbox_size = stoul(std::get<1>(stat_msg).substr(pos + 1));
    }
    catch (out_of_range&)
    {
        throw pop3_error("Parser failure.");
    }
    catch (invalid_argument&)
    {
        throw pop3_error("Parser failure.");
    }

    return mailbox_stat;
}


void pop3::fetch(unsigned long message_no, message& msg)
{
    _dlg->send("RETR " + to_string(message_no));
    string line = _dlg->receive();
    tuple<string, string> stat_msg = parse_status(line);
    if (iequals(std::get<0>(stat_msg), "-ERR"))
        throw pop3_error("Fetching message failure.");

    // end of message is marked with crlf+dot+crlf sequence
    // empty_line marks the last empty line, so it could be used to detect end of message when dot is reached
    bool empty_line = false;
    while (true)
    {
        line = _dlg->receive();
        // reading line by line ensures that crlf are the last characters read; so, reaching single dot in the line means that it's end of message
        if (line == ".")
        {
            msg.parse_by_line("\r\n");
            break;
        }
        else if (line.empty())
        {
            // ensure that sequence of empty lines are all included in the message; otherwise, mark that an empty line is reached
            if (empty_line)
                msg.parse_by_line("");
            else
                empty_line = true;
        }
        else
        {
            // regular line with the content; if empty line was before this one, ensure that it is included
            if (empty_line)
                msg.parse_by_line("");
            msg.parse_by_line(line, true);
            empty_line = false;
        }
    }
}


void pop3::remove(unsigned long message_no)
{
    _dlg->send("DELE " + to_string(message_no));
    string line = _dlg->receive();
    tuple<string, string> stat_msg = parse_status(line);
    if (iequals(std::get<0>(stat_msg), "-ERR"))
        throw pop3_error("Removing message failure.");
}


void pop3::connect()
{
    string line = _dlg->receive();
    tuple<string, string> stat_msg = parse_status(line);
    if (iequals(std::get<0>(stat_msg), "-ERR"))
        throw pop3_error("Connection to server failure.");
}


void pop3::auth_login(const string& username, const string& password)
{
    {
        _dlg->send("USER " + username);
        string line = _dlg->receive();
        tuple<string, string> stat_msg = parse_status(line);
        if (iequals(std::get<0>(stat_msg), "-ERR"))
            throw pop3_error("Username rejection.");
    }

    {
        _dlg->send("PASS " + password);
        string line = _dlg->receive();
        tuple<string, string> stat_msg = parse_status(line);
        if (iequals(std::get<0>(stat_msg), "-ERR"))
            throw pop3_error("Password rejection.");
    }
}


tuple<string, string> pop3::parse_status(const string& line)
{
    string::size_type pos = line.find(' ');
    string status = line.substr(0, pos);
    if (!iequals(status, "+OK") && !iequals(status, "-ERR"))
        throw pop3_error("Response status unknown.");
    string message;
    if (pos != string::npos)
        message = line.substr(pos + 1);
    return make_tuple(status, message);
}


pop3s::pop3s(const string& hostname, unsigned port) : pop3(hostname, port)
{
}


void pop3s::authenticate(const string& username, const string& password, auth_method_t method)
{
    if (method == auth_method_t::LOGIN)
    {
        switch_to_ssl();
        connect();
        auth_login(username, password);
    }
    if (method == auth_method_t::START_TLS)
    {
        connect();
        start_tls();
        auth_login(username, password);
    }
}


/*
For details see [rfc 2595/4616].
*/
void pop3s::start_tls()
{
    _dlg->send("STLS");
    string response = _dlg->receive();
    tuple<string, string> stat_msg = parse_status(response);
    if (iequals(std::get<0>(stat_msg), "-ERR"))
        throw pop3_error("Start TLS failure.");

    switch_to_ssl();
}


void pop3s::switch_to_ssl()
{
    dialog* d = _dlg.get();
    _dlg.reset(new dialog_ssl(move(*d)));
}


} // namespace mailio
