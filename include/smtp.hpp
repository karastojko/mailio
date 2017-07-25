/*

smtp.hpp
--------

Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
#include <memory>
#include <tuple>
#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>
#include "message.hpp"
#include "dialog.hpp"


namespace mailio
{


/**
Implements smtp client.
**/
class smtp
{
public:

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {NONE, LOGIN};

    /**
    Makes connection to the server.

    @param hostname   Hostname of the server.
    @param port       Port of the server.
    @throw smtp_error Empty source hostname not allowed.
    @throw *          `dialog::dialog`, `read_hostname`.
    **/
    smtp(const std::string& hostname, unsigned port);

    /**
    Sends quit command and closes the connection.
    **/
    virtual ~smtp();

    smtp(const smtp&) = delete;
    
    smtp(smtp&&) = delete;

    void operator=(const smtp&) = delete;

    void operator=(smtp&&) = delete;

    /**
    Performs authentication with the given credentials.

    The method should be called only once on an existing object - it is not possible to authenticate again within the same connection.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `connect`, `ehlo`, `auth_login`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

    /**
    Submits the message.
    
    @param msg        Mail message to send.
    @throw smtp_error Mail refused by server.
    @throw smtp_error Mail sender refused by server.
    @throw smtp_error Mail recipient refused by server.
    @throw *          `parse_line`, `dialog::send`, `dialog::receive`.
    **/
    void submit(const message& msg);

    /**
    Sets the source hostname.

    @param src_host Source hostname to set.
    **/
    void source_hostname(const std::string& src_host);

    /**
    Gets source hostname.

    @return Source hostname.
    **/
    std::string source_hostname() const;

protected:

    /**
    SMTP response status.
    **/
    enum smtp_status_t {POSITIVE_COMPLETION = 2, POSITIVE_INTERMEDIATE = 3, TRANSIENT_NEGATIVE = 4, PERMANENT_NEGATIVE = 5};

    /**
    Initializes connection to the server.

    @throw smtp_error Connection refused by server.
    @throw *          `parse_line`, `dialog::receive`.
    **/
    void connect();

    /**
    Performs authentication with the login method.

    @param username   Username to authenticate.
    @param password   Password to authenticate.
    @throw smtp_error Authentication refused by server.
    @throw smtp_error Username refused by server.
    @throw smtp_error Password refused by server.
    @throw *          `parse_line`, `dialog::send`, `dialog::receive`.
    **/
    void auth_login(const std::string& username, const std::string& password);

    /**
    Issues `EHLO` and/or `HELO` commands.
    
    @throw smtp_error Initiation message refused.
    @throw *          `parse_line`, `dialog::send`, `dialog::receive`.
    **/
    void ehlo();

    /**
    Reads the source hostname.

    @return           Source hostname.
    @throw smtp_error Reading hostname failed.
    **/
    std::string read_hostname();

    /**
    Parses response line into three tokens.

    @param response   Response line to parse.
    @return           Tuple with a status number, flag if the line is the last one and status message.
    @throw smtp_error Bad server response.
    **/
    static std::tuple<int, bool, std::string> parse_line(const std::string& response);

    /**
    Check if the status is 2XX.

    @param status Status to check.
    @return       True if does, false if not.
    **/
    static bool positive_completion(int status);

    /**
    Check if the status is 3XX.

    @param status Status to check.
    @return       True if does, false if not.
    **/
    static bool positive_intermediate(int status);

    /**
    Check if the status is 4XX.

    @param status Status to check.
    @return       True if does, false if not.
    **/
    static bool transient_negative(int status);

    /**
    Check if the status is 5XX.

    @param status Status to check.
    @return       True if does, false if not.
    **/
    static bool permanent_negative(int status);

    /**
    Name of the host which client is connecting from.
    **/
    std::string _src_host;

    /**
    Dialog to use for send/receive operations.
    **/
    std::unique_ptr<dialog> _dlg;
};


/**
Secure version of smtp client.
**/
class smtps : public smtp
{
public:

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {NONE, LOGIN, START_TLS};

    /**
    Makes connection to the server.
    
    Calls parent constructor to do all the work.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @throw *        `smtp::smtp`.
    **/
    smtps(const std::string& hostname, unsigned port);

    /**
    Sends quit command and closes the connection.
    
    Calls parent destructor to do all the work.
    **/
    ~smtps() = default;

    smtps(const smtps&) = delete;

    smtps(smtps&&) = delete;

    void operator=(const smtps&) = delete;

    void operator=(smtps&&) = delete;

    /**
    Performs authentication with the given credentials.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `start_tls`, `switch_to_ssl`, `ehlo`, `auth_login`, `connect`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

protected:

    /**
    Switches to tls layer.
    
    @throw smtp_error Start tls refused by server.
    @throw *          `parse_line`, `ehlo`, `dialog::send`, `dialog::receive`, `switch_to_ssl`.
    **/
    void start_tls();

    /**
    Replaces tcp socket with ssl socket.

    @throw * `dialog_ssl::dialog_ssl`.
    **/
    void switch_to_ssl();
};


/**
Error thrown by smtp client.
**/
class smtp_error : public std::runtime_error
{
public:

    /**
    Calls parent constructor.

    @param msg Error message.
    **/
    explicit smtp_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calls parent constructor.

    @param msg Error message.
    **/
    explicit smtp_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


} // namespace mailio
