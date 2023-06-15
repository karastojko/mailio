/*

dialog.hpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
#include <stdexcept>
#include <chrono>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "export.hpp"


namespace mailio
{


/**
Dealing with network in a line oriented fashion.
**/
class dialog : public std::enable_shared_from_this<dialog>
{
public:

    /**
    Making a connection to the server.

    @param hostname     Server hostname.
    @param port         Server port.
    @param timeout      Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw dialog_error Server connecting failed.
    @throw *             `connect_async()`.
    **/
    dialog(const std::string& hostname, unsigned port, std::chrono::milliseconds timeout);

    /**
    Copy constructor.

    @param other Object to copy.
    **/
    dialog(const dialog& other);

    /**
    Closing the connection.
    **/
    virtual ~dialog() = default;

    dialog(dialog&&) = delete;

    void operator=(const dialog&) = delete;

    void operator=(dialog&&) = delete;

    virtual void connect();

    /**
    Sending a line to network synchronously or asynchronously, depending of the timeout value.

    @param line Line to send.
    @throw *    `send_sync<Socket>(Socket&, const std::string&)`, `send_async<Socket>(Socket&, const std::string&)`.
    **/
    virtual void send(const std::string& line);

    /**
    Receiving a line from network.

    @param raw Flag if the receiving is raw (no CRLF is truncated) or not.
    @return    Line read from network.
    @throw *   `receive_sync<Socket>(Socket&, bool)`, `receive_async<Socket>(Socket&, bool)`.
    **/
    virtual std::string receive(bool raw = false);

protected:

    /**
    Sending a line to network in synchronous manner.

    @param socket       Socket to use for I/O.
    @param line         Line to send.
    @throw dialog_error Network sending error.
    **/
    template<typename Socket>
    void send_sync(Socket& socket, const std::string& line);

    /**
    Receiving a line from network in synchronous manner.

    @param socket       Socket to use for I/O.
    @param raw          Flag if the receiving is raw (no CRLF is truncated) or not.
    @return line        Line received.
    @throw dialog_error Network sending error.
    **/
    template<typename Socket>
    std::string receive_sync(Socket& socket, bool raw);

    /**
    Connecting to the host within the given timeout period.

    @throw dialog_error Server connecting failed.
    @throw dialog_error Server connecting timed out.
    **/
    void connect_async();

    /**
    Sending a line over network within the given timeout period.

    @param socket       Socket to use for I/O.
    @param line         Line to send.
    @throw dialog_error Network sending failed.
    @throw dialog_error Network sending timed out.
    **/
    template<typename Socket>
    void send_async(Socket& socket, std::string line);

    /**
    Receiving a line over network within the given timeout period.

    @param socket       Socket to use for I/O.
    @param raw          Flag if the receiving is raw (no CRLF is truncated) or not.
    @return line        Line received.
    @throw dialog_error Network receiving failed.
    @throw dialog_error Network receiving timed out.
    **/
    template<typename Socket>
    std::string receive_async(Socket& socket, bool raw);

    /**
    Checking if the timeout is reached.
    **/
    void check_timeout();

    /**
    Server hostname.
    **/
    const std::string hostname_;

    /**
    Server port.
    **/
    const unsigned int port_;

    /**
    Asio input/output service.
    **/
    static boost::asio::io_context ios_;

    /**
    Socket connection.
    **/
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;

    /**
    Timer to check the timeout.
    **/
    std::shared_ptr<boost::asio::deadline_timer> timer_;

    /**
    Timeout on I/O operations in milliseconds.
    **/
    std::chrono::milliseconds timeout_;

    /**
    Flag to show whether the timeout has expired.
    **/
    bool timer_expired_;

    /**
    Stream buffer associated to the socket.
    **/
    std::shared_ptr<boost::asio::streambuf> strmbuf_;

    /**
    Input stream associated to the buffer.
    **/
    std::shared_ptr<std::istream> istrm_;
};


/**
Secure version of `dialog` class.
**/
class dialog_ssl : public dialog
{
public:

    /**
    SSL options to set on a socket.
    **/
    struct ssl_options_t
    {
        /**
        Methods as supported by Asio.
        **/
        boost::asio::ssl::context::method method;

        /**
        Peer verification bitmask supported by Asio.
        **/
        boost::asio::ssl::verify_mode verify_mode;
    };

    /**
    Making a connection to the server.

    @param hostname Server hostname.
    @param port     Server port.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @param options  SSL options to set.
    @throw *        `dialog::dialog(const std::string&, unsigned)`.
    **/
    dialog_ssl(const std::string& hostname, unsigned port, std::chrono::milliseconds timeout, const ssl_options_t& options);

    /**
    Calling the parent constructor, initializing the SSL socket.

    @param other   Plain connection to use for the SSL.
    @param options SSL options to set.
    **/
    dialog_ssl(const dialog& other, const ssl_options_t& options);

    /**
    Default copy constructor.
    **/
    dialog_ssl(const dialog_ssl&) = default;

    /**
    Default destructor.
    **/
    virtual ~dialog_ssl() = default;

    dialog_ssl(dialog_ssl&&) = delete;

    void operator=(const dialog_ssl&) = delete;

    void operator=(dialog_ssl&&) = delete;

    /**
    Sending an encrypted or unecrypted line, depending of SSL flag.

    @param line Line to send.
    @throw *    `dialog::send(const std::string&)`, `send_sync<Socket>(Socket&, const std::string&)`, `send_async<Socket>(Socket&, const std::string&)`.
    **/
    void send(const std::string& line);

    /**
    Receiving an encrypted or unecrypted line, depending of SSL state.

    @param raw Flag if the receiving is raw (no CRLF is truncated) or not.
    @return    Line read from network
    @throw *   `dialog::receive()`, `receive_sync<Socket>(Socket&, bool)`, `receive_async<Socket>(Socket&, bool)`.
    **/
    std::string receive(bool raw = false);

protected:

    /**
    Flag if SSL is chosen or not.
    **/
    bool ssl_;

    /**
    SSL context (when used).
    **/
    std::shared_ptr<boost::asio::ssl::context> context_;

    /**
    SSL socket (when used).
    **/
    std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> ssl_socket_;
};


/**
Error thrown by `dialog` client.
**/
class dialog_error : public std::runtime_error
{
public:

    /**
    Calling the parent constructor.

    @param msg Error message.
    **/
    explicit dialog_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calling the parent constructor.

    @param msg Error message.
    **/
    explicit dialog_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


} // namespace mailio
