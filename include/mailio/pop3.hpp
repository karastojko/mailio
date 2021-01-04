/*

pop3.hpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <istream>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>
#include "dialog.hpp"
#include "message.hpp"
#include "export.hpp"


namespace mailio
{


/**
POP3 client implementation.
**/
class MAILIO_EXPORT pop3
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
    Message order numbers and their corresponding unique IDs (a UIDL response from server).
    **/
    typedef std::map<unsigned, std::string> uidl_list_t;

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
        Setting the number of messages and mailbox size to zero.
        **/
        mailbox_stat_t() : messages_no(0), mailbox_size(0)
        {
        }
    };

    /**
    Making connection to a server.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `dialog::dialog(const string&, unsigned)`.
    **/
    pop3(const std::string& hostname, unsigned port, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
    Sending the quit command and closing the connection.
    **/
    virtual ~pop3();

    pop3(const pop3&) = delete;

    pop3(pop3&&) = delete;

    void operator=(const pop3&) = delete;

    void operator=(pop3&&) = delete;

    /**
    Authentication with the given credentials.

    The method should be called only once on an existing object - it is not possible to authenticate again within the same connection.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @return         The server greeting message.
    @throw *        `connect()`, `auth_login(const string&, const string&)`.
    **/
    std::string authenticate(const std::string& username, const std::string& password, auth_method_t method);

    /**
    Listing the size in octets of a message or all messages in a mailbox.

    @param message_no Number of the message to list. If zero, then all messages are listed.
    @return           Message list.
    @throw pop3_error Listing message failure.
    @throw pop3_error Listing all messages failure.
    @throw pop3_error Parser failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             This method is perhaps useless and should be removed.
    **/
    message_list_t list(unsigned message_no = 0);

    /**
    Getting the unique ID of a message or all messages in a mailbox (UIDL command; might not be supported by server).

    @param message_no Number of the message to get ID for. If zero, then all messages are listed.
    @return           Uidl list.
    @throw pop3_error Listing message failure.
    @throw pop3_error Listing all messages failure.
    @throw pop3_error Parser failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    uidl_list_t uidl(unsigned message_no = 0);

    /**
    Fetching the mailbox statistics.

    @return           Number of messages and mailbox size in octets.
    @throw pop3_error Reading statistics failure.
    @throw pop3_error Parser failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    mailbox_stat_t statistics();

    /**
    Fetching a message.

    The flag for fetching the header only uses a different POP3 command (than for retrieving the full messsage) which is not mandatory by POP3. In case the
    command fails, the method will not report an error but rather the `msg` parameter will be empty.

    @param message_no  Message number to fetch.
    @param msg         Fetched message.
    @param header_only Flag if only the message header should be fetched.
    @throw pop3_error  Fetching message failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void fetch(unsigned long message_no, message& msg, bool header_only = false);

    /**
    Removing a message in the mailbox.

    @param message_no Message number to remove.
    @throw pop3_error Removing message failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void remove(unsigned long message_no);

protected:

    /**
    Character used by POP3 to separate tokens.
    **/
    static const char TOKEN_SEPARATOR_CHAR{' '};

    /**
    Initializing a connection to the server.

    @return           The server greeting message.
    @throw pop3_error Connection to server failure.
    @throw *          `parse_status(const string&)`, `dialog::receive()`.
    **/
    std::string connect();

    /**
    Authentication of a user.

    @param username    Username to authenticate.
    @param password    Password to authenticate.
    @throw pop3_error  Username rejection.
    @throw pop3_error  Password rejection.
    @throw *           `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void auth_login(const std::string& username, const std::string& password);

    /**
    Parsing a response line for the status.

    @param line       Response line to parse.
    @return           Tuple with the status and rest of the line.
    @throw pop3_error Response status unknown.
    **/
    std::tuple<std::string, std::string> parse_status(const std::string& line);

    /**
    Dialog to use for send/receive operations.
    **/
    std::shared_ptr<dialog> _dlg;
};


/**
Secure version of POP3 client.
**/
class MAILIO_EXPORT pop3s : public pop3
{
public:

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {LOGIN, START_TLS};

    /**
    Making a connection to server.

    Parent constructor is called to do all the work.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `pop3::pop3(const string&, unsigned)`.
    **/
    pop3s(const std::string& hostname, unsigned port, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
    Sending the quit command and closing the connection.

    Parent destructor is called to do all the work.
    **/
    ~pop3s() = default;

    pop3s(const pop3s&) = delete;

    pop3s(pop3s&&) = delete;

    void operator=(const pop3s&) = delete;

    void operator=(pop3s&&) = delete;

    /**
    Authenticating with the given credentials.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @return         The server greeting message.
    @throw *        `start_tls()`, `pop3::auth_login(const string&, const string&)`.
    **/
    std::string authenticate(const std::string& username, const std::string& password, auth_method_t method);

protected:

    /**
    Switching to TLS layer.

    @throw pop3_error Start TLS failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`, `switch_to_ssl()`.
    **/
    void start_tls();

    /**
    Replacing a TCP socket with an SSL one.

    @throw * `dialog_ssl::dialog_ssl(dialog&&)`.
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
    Calling the parent constructor.

    @param msg Error message.
    **/
    explicit pop3_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calling the parent constructor.

    @param msg Error message.
    **/
    explicit pop3_error(const char* msg) : std::runtime_error(msg)
    {
    }
};

} // namespace mailio
