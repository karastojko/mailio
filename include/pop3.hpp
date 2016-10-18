/*

pop3.hpp
--------

Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/ 


#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <istream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>
#include "dialog.hpp"
#include "message.hpp"


namespace mailio
{


/**
Implements pop3 client.
**/
class pop3
{
public:

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {LOGIN};

    /**
    Messages indexed by their order number containing sizes in octets.
    **/
    typedef std::map<unsigned, unsigned long> message_list_t;

    /**
    Mailbox statistics structure.
    **/
    struct mailbox_stat_t
    {
        /**
        Number of messages in the mailbox.
        **/
        unsigned int messages_no;
        
        /**
        Size of the mailbox.
        **/
        unsigned long mailbox_size;
        
        /**
        Sets number of messages and mailbox size to zero.
        **/
        mailbox_stat_t() : messages_no(0), mailbox_size(0)
        {
        }
    };

    /**
    Makes connection to a server.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @throw *        `dialog::dialog`, `dialog::receive`.
    **/
    pop3(const std::string& hostname, unsigned port);

    /**
    Sends quit command and closes the connection.
    **/    
    virtual ~pop3();

    pop3(const pop3&) = delete;

    pop3(pop3&&) = delete;

    void operator=(const pop3&) = delete;

    void operator=(pop3&&) = delete;

    /**
    Performs authentication with the given credentials.
    
    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `connect`, `auth_login`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

    /**
    Lists the size in octets of a message or all messages in a mailbox.
    
    @param message_no Number of the message to list. If zero, then all messages are listed.
    @return           Message list.
    @throw pop3_error Listing message failed.
    @throw pop3_error Listing all messages failed.
    @throw pop3_error Bad server response.
    @throw *          `parse_status`, `dialog::send`, `dialog::receive`.
    @todo             This method is perhaps useless and should be removed.
    **/
    message_list_t list(unsigned message_no = 0);

    /**
    Gets mailbox statistics.
    
    @return           Number of messages and mailbox size in octets.
    @throw pop3_error Reading statistics failed.
    @throw pop3_error Bad server response.
    @throw *          `parse_status`, `dialog::send`, `dialog::receive`.
    **/
    mailbox_stat_t statistics();

    /**
    Fetches a message.
    
    @param message_no Message number to fetch.
    @throw pop3_error Fetching message failed.
    @throw *          `parse_status`, `dialog::send`, `dialog::receive`.
    **/
    void fetch(unsigned long message_no, message& msg);

    /**
    Removes a message in the mailbox.
    
    @param message_no Message number to remove.
    @throw pop3_error Removing message failed.
    @throw *          `parse_status`, `dialog::send`, `dialog::receive`.
    **/
    void remove(unsigned long message_no);

protected:

    /**
    Initializes connection to the server.

    @throw pop3_error Connection refused by server.
    @throw *          `parse_status`, `dialog::receive`.
    **/
    void connect();

    /**
    Performs the authentication.

    @param username    Username to authenticate.
    @param password    Password to authenticate.
    @throw pop3_error  Username refused by server.
    @throw pop3_error  Password refused by server.
    @throw *           `parse_status`, `dialog::send`, `dialog::receive`.
    **/
    void auth_login(const std::string& username, const std::string& password);

    /**
    Parses a response line for the status.
    
    @param line       Response line to parse.
    @return           Tuple with the status and rest of the line.
    @throw pop3_error Bad server response.
    @throw pop3_error Bad response status.
    **/
    std::tuple<std::string, std::string> parse_status(const std::string& line);

    /**
    Dialog to use for send/receive operations.
    **/
    std::unique_ptr<dialog> _dlg;
};


/**
Secure version of pop3 client.
**/
class pop3s : public pop3
{
public:

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {LOGIN, START_TLS};

    /**
    Makes connection to the server.
    
    Calls parent constructor to do all the work.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @throw *        `pop3::pop3`.
    **/
    pop3s(const std::string& hostname, unsigned port);

    /**
    Sends quit command and closes the connection.

    Calls parent destructor to do all the work.
    **/
    ~pop3s() = default;

    pop3s(const pop3s&) = delete;

    pop3s(pop3s&&) = delete;

    void operator=(const pop3s&) = delete;

    void operator=(pop3s&&) = delete;

    /**
    Performs authentication with the given credentials.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `start_tls`, `pop3::auth_login`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

protected:

    /**
    Switches to tls layer.
    
    @throw pop3_error Start tls refused by server.
    @throw *          `parse_status`, `dialog::send`, `dialog::receive`, `dialog_ssl::dialog_ssl`.
    **/
    void start_tls();

    /**
    Replaces tcp socket with ssl socket.

    @throw *  dialog_ssl::dialog_ssl().
    **/
    void switch_to_ssl();
};


/**
Error thrown by POP3 client.
**/
class pop3_error : public std::runtime_error
{
public:

    /**
    Calls parent constructor.

    @param msg  Error message.
    **/
    explicit pop3_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calls parent constructor.

    @param msg  Error message.
    **/
    explicit pop3_error(const char* msg) : std::runtime_error(msg)
    {
    }
};

} // namespace mailio
