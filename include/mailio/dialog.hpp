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
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>
#include "export.hpp"


namespace mailio
{


/**
Dealing with network in a line oriented fashion.
**/
class MAILIO_EXPORT dialog
{
public:

    /**
    Making a connection to the server.

    @param hostname     Server hostname.
    @param port         Server port.
    @throw dialog_error Server connecting failed.
    **/
    dialog(const std::string& hostname, unsigned port);

    /**
    Moving server parameters and connection.

    @param other Dialog to move from.
    **/
    dialog(dialog&& other);

    /**
    Closing the connection.
    **/
    virtual ~dialog();

    dialog(const dialog& other) = delete;

    void operator=(const dialog&) = delete;

    void operator=(dialog&&) = delete;

    /**
    Sending a line to network.

    @param line         Line to send.
    @throw dialog_error Network sending error.
    **/
    virtual void send(const std::string& line);

    /**
    Receiving a line from network.

    @return             Line read from network.
    @throw dialog_error Network receiving error.
    **/
    virtual std::string receive();

    /**
    Receiving a line from network without removing CRLF characters.

    @return             Line read from network.
    @throw dialog_error Network receiving error.
    **/
    virtual std::string receive_raw();

protected:

    /**
    Server hostname.
    **/
    const std::string _hostname;

    /**
    Server port.
    **/
    const unsigned int _port;

    /**
    Asio input/output service.
    **/
    std::unique_ptr<boost::asio::io_service> _ios;

    /**
    Socket connection.
    **/
    boost::asio::ip::tcp::socket _socket;

    /**
    Stream buffer associated to the socket.
    **/
    std::unique_ptr<boost::asio::streambuf> _strmbuf;

    /**
    Input stream associated to the buffer.
    **/
    std::unique_ptr<std::istream> _istrm;
};


/**
Secure version of `dialog` class.
**/
class dialog_ssl : public dialog
{
public:

    /**
    Making a connection to the server.

    @param hostname Server hostname.
    @param port     Server port.
    @throw *        `dialog::dialog(const std::string&, unsigned)`.
    **/
    dialog_ssl(const std::string& hostname, unsigned port);

    /**
    Calling the parent constructor and members copy constructor.
    **/
    dialog_ssl(const dialog_ssl&) = default;

    /**
    Moving the server parameters and connection.

    @param other        Dialog to move from.
    @throw dialog_error Switching to ssl failed.
    @throw *            `dialog::dialog(dialog&&)`.
    **/
    dialog_ssl(dialog&& other);

    /**
    Default destructor.
    **/
    virtual ~dialog_ssl() = default;

    dialog_ssl(dialog_ssl&&) = delete;

    void operator=(const dialog_ssl&) = delete;

    void operator=(dialog_ssl&&) = delete;

    /**
    Sending an encrypted or unecrypted line, depending of SSL flag.

    @param line         Line to send.
    @throw dialog_error Network sending error.
    @throw *            `dialog::send(const std::string&)`.
    **/
    void send(const std::string& line);

    /**
    Receiving an encrypted or unecrypted line, depending of SSL state.

    @return             Line read from network
    @throw dialog_error Network receiving error.
    @throw *            `dialog::receive()`.
    **/
    std::string receive();

    /**
    Receiving an encrypted or unecrypted line, depending of SSL flag, without removing CRLF characters.

    @return             Line read from network
    @throw dialog_error Network receiving error.
    @throw *            `dialog::receive_raw()`.
    **/
    virtual std::string receive_raw();

protected:

    /**
    Flag if SSL is chosen or not.
    **/
    bool _ssl;

    /**
    SSL context (when used).
    **/
    boost::asio::ssl::context _context;

    /**
    SSL socket (when used).
    **/
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> _ssl_socket;
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
