/*

dialog.hpp
----------

Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/ 


#pragma once

#include <string>
#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>


namespace mailio
{


/**
Deals with network in a line oriented fashion.
**/
class dialog
{
public:

    /**
    Makes connection to the server.

    @param hostname     Server hostname.
    @param port         Server port.
    @throw dialog_error Server connecting failed.
    **/
    dialog(const std::string& hostname, unsigned port);

    /**
    Moves server parameters and connection.

    @param other Dialog to move from.
    **/
    dialog(dialog&& other);

    /**
    Closes the connection.
    **/
    virtual ~dialog();

    dialog(const dialog& other) = delete;

    void operator=(const dialog&) = delete;

    void operator=(dialog&&) = delete;

    /**
    Sends a line to network.

    @param line         Line to send.
    @throw dialog_error Network sending error.
    **/
    virtual void send(const std::string& line);

    /**
    Receives a line from network.

    @return             Line read from network.
    @throw dialog_error Network reading error.
    **/
    virtual std::string receive();

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
    Stream buffer associated to socket.
    **/
    std::unique_ptr<boost::asio::streambuf> _strmbuf;

    /**
    Input stream associated to buffer.
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
    Makes connection to the server.

    @param hostname Server hostname.
    @param port     Server port.
    @throw *        `dialog::dialog`.
    **/
    dialog_ssl(const std::string& hostname, unsigned port);

    /**
    Calls parent constructor and members copy constructor.
    **/
    dialog_ssl(const dialog_ssl&) = default;

    /**
    Moves server parameters and connection.

    @param other        Dialog to move from.
    @throw dialog_error Switching to ssl failed.
    @throw *            `dialog::dialog`.
    **/
    dialog_ssl(dialog&& other);

    /**
    Calls parent destructor.
    **/
    virtual ~dialog_ssl() = default;

    dialog_ssl(dialog_ssl&&) = delete;

    void operator=(const dialog_ssl&) = delete;

    void operator=(dialog_ssl&&) = delete;

    /**
    Sends encrypted or non-crypted line, depending of SSL flag.

    @param line         Line to send.
    @throw dialog_error Network sending error.
    @throw *            `dialog::send`.
    **/
    void send(const std::string& line);

    /**
    Receives encrypted or non-crypted line, depending of ssl state.

    @return             Line read from network
    @throw dialog_error Network receiving error.
    @throw *            `dialog::receive`.
    **/
    std::string receive();

protected:

    /**
    Flag if ssl is chosen or not.
    **/
    bool _ssl;

    /**
    Ssl context (when used).
    **/
    boost::asio::ssl::context _context;

    /**
    Ssl socket (when used).
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
    Calls parent constructor.

    @param msg Error message.
    **/
    explicit dialog_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calls parent constructor.

    @param msg Error message.
    **/
    explicit dialog_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


} // namespace mailio
