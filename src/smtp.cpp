/*

smtp.cpp
---------------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <tuple>
#include <algorithm>
#include <boost/asio/ip/host_name.hpp>
#include <base64.hpp>
#include <smtp.hpp>


using std::cout;
using std::endl;
using std::ostream;
using std::istream;
using std::vector;
using std::string;
using std::to_string;
using std::tuple;
using std::stoi;
using std::move;
using std::runtime_error;
using std::out_of_range;
using std::invalid_argument;
using boost::asio::ip::host_name;
using boost::system::system_error;



namespace mailio
{


smtp::smtp(const string& hostname, unsigned port) : _dlg(new dialog(hostname, port))
{
    _src_host = read_hostname();
}


smtp::~smtp()
{
    try
    {
        _dlg->send("QUIT");
    }
    catch (...)
    {
    }
}


void smtp::authenticate(const string& username, const string& password, auth_method_t method)
{
    connect();
    if (method == auth_method_t::NONE)
    {
        ehlo();
    }
    else if (method == auth_method_t::LOGIN)
    {
        ehlo();
        auth_login(username, password);
    }
}


void smtp::submit(const message& msg)
{
    _dlg->send("MAIL FROM: <" + msg.sender().address + ">");
    string line = _dlg->receive();
    tuple<int, bool, string> tokens = parse_line(line);
    if (std::get<1>(tokens) && !positive_completion(std::get<0>(tokens)))
        throw smtp_error("Mail sender rejection.");

    for (const auto& rcpt : msg.recipients().addresses)
    {
        _dlg->send("RCPT TO: <" + rcpt.address + ">");
        line = _dlg->receive();
        tokens = parse_line(line);
        if (!positive_completion(std::get<0>(tokens)))
            throw smtp_error("Mail recipient rejection.");
    }

    for (const auto& rcpt : msg.recipients().groups)
    {
        _dlg->send("RCPT TO: <" + rcpt.name + ">");
        line = _dlg->receive();
        tokens = parse_line(line);
        if (!positive_completion(std::get<0>(tokens)))
            throw smtp_error("Mail group recipient rejection.");
    }

    for (const auto& rcpt : msg.cc_recipients().addresses)
    {
        _dlg->send("RCPT TO: <" + rcpt.address + ">");
        line = _dlg->receive();
        tokens = parse_line(line);
        if (!positive_completion(std::get<0>(tokens)))
            throw smtp_error("Mail cc recipient rejection.");
    }

    for (const auto& rcpt : msg.cc_recipients().groups)
    {
        _dlg->send("RCPT TO: <" + rcpt.name + ">");
        line = _dlg->receive();
        tokens = parse_line(line);
        if (!positive_completion(std::get<0>(tokens)))
            throw smtp_error("Mail group cc recipient rejection.");
    }

    for (const auto& rcpt : msg.bcc_recipients().addresses)
    {
        _dlg->send("RCPT TO: <" + rcpt.address + ">");
        line = _dlg->receive();
        tokens = parse_line(line);
        if (!positive_completion(std::get<0>(tokens)))
            throw smtp_error("Mail bcc recipient rejection.");
    }

    for (const auto& rcpt : msg.bcc_recipients().groups)
    {
        _dlg->send("RCPT TO: <" + rcpt.name + ">");
        line = _dlg->receive();
        tokens = parse_line(line);
        if (!positive_completion(std::get<0>(tokens)))
            throw smtp_error("Mail group bcc recipient rejection.");
    }

    _dlg->send("DATA");
    line = _dlg->receive();
    tokens = parse_line(line);
    if (!positive_intermediate(std::get<0>(tokens)))
        throw smtp_error("Mail message rejection.");

    string msg_str;
    msg.format(msg_str, true);
    _dlg->send(msg_str + "\r\n.");
    line = _dlg->receive();
    tokens = parse_line(line);
    if (!positive_completion(std::get<0>(tokens)))
        throw smtp_error("Mail message rejection.");
}


void smtp::source_hostname(const string& src_host)
{
    _src_host = src_host;
}


string smtp::source_hostname() const
{
    return _src_host;
}


void smtp::connect()
{
    string line = _dlg->receive();
    tuple<int, bool, string> tokens = parse_line(line);
    if (std::get<0>(tokens) != 220)
        throw smtp_error("Connection rejection.");
}


void smtp::auth_login(const string& username, const string& password)
{
    _dlg->send("AUTH LOGIN");
    string line = _dlg->receive();
    tuple<int, bool, string> tokens = parse_line(line);
    if (std::get<1>(tokens) && !positive_intermediate(std::get<0>(tokens)))
        throw smtp_error("Authentication rejection.");

    // TODO: use static encode from base64
    base64 b64;
    _dlg->send(b64.encode(username)[0]);
    line = _dlg->receive();
    tokens = parse_line(line);
    if (std::get<1>(tokens) && !positive_intermediate(std::get<0>(tokens)))
        throw smtp_error("Username rejection.");

    _dlg->send(b64.encode(password)[0]);
    line = _dlg->receive();
    tokens = parse_line(line);
    if (std::get<1>(tokens) && !positive_completion(std::get<0>(tokens)))
        throw smtp_error("Password rejection.");
}


void smtp::ehlo()
{
    _dlg->send("EHLO " + _src_host);
    string line = _dlg->receive();
    tuple<int, bool, string> tokens = parse_line(line);    
    while (!std::get<1>(tokens))
    {
        line = _dlg->receive();
        tokens = parse_line(line);
    }

    if (!positive_completion(std::get<0>(tokens)))
    {
        _dlg->send("HELO " + _src_host);
        
        line = _dlg->receive();
        tokens = parse_line(line);
        while (!std::get<1>(tokens))
        {
            line = _dlg->receive();
            tokens = parse_line(line);
        }
        if (!positive_completion(std::get<0>(tokens)))
            throw smtp_error("Initial message rejection.");
    }
}


string smtp::read_hostname()
{
    try
    {
        return host_name();
    }
    catch (system_error&)
    {
        throw smtp_error("Reading hostname failure.");
    }
}


tuple<int, bool, string> smtp::parse_line(const string& line)
{
    try
    {
        return make_tuple(stoi(line.substr(0, 3)), (line.at(3) == '-' ? false : true), line.substr(4));
    }
    catch (out_of_range&)
    {
        throw smtp_error("Parsing server failure.");
    }
    catch (invalid_argument&)
    {
        throw smtp_error("Parsing server failure.");
    }
}


inline bool smtp::positive_completion(int status)
{
    return status / 100 == smtp_status_t::POSITIVE_COMPLETION;
}


inline bool smtp::positive_intermediate(int status)
{
    return status / 100 == smtp_status_t::POSITIVE_INTERMEDIATE;
}


inline bool smtp::transient_negative(int status)
{
    return status / 100 == smtp_status_t::TRANSIENT_NEGATIVE;
}


inline bool smtp::permanent_negative(int status)
{
    return status / 100 == smtp_status_t::PERMANENT_NEGATIVE;
}


smtps::smtps(const string& hostname, unsigned port) : smtp(hostname, port)
{
}


void smtps::authenticate(const string& username, const string& password, auth_method_t method)
{
    if (method == auth_method_t::NONE)
    {
        switch_to_ssl();
        connect();
        ehlo();
    }
    else if (method == auth_method_t::LOGIN)
    {
        switch_to_ssl();
        connect();
        ehlo();
        auth_login(username, password);
    }
    else if (method == auth_method_t::START_TLS)
    {
        connect();
        ehlo();
        start_tls();
        auth_login(username, password);
    }
}


void smtps::start_tls()
{
    _dlg->send("STARTTLS");
    string line = _dlg->receive();
    tuple<int, bool, string> tokens = parse_line(line);
    if (std::get<1>(tokens) && std::get<0>(tokens) != 220)
        throw smtp_error("Start tls refused by server.");

    switch_to_ssl();
    ehlo();
}


void smtps::switch_to_ssl()
{
    dialog* d = _dlg.get();
    _dlg.reset(new dialog_ssl(move(*d)));
}


} // namespace mailio
