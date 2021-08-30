/*

dialog.cpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/

#include <string>
#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <mailio/dialog.hpp>


using std::string;
using std::to_string;
using std::move;
using std::istream;
using std::make_shared;
using std::shared_ptr;
using std::bind;
using std::chrono::milliseconds;
using boost::asio::ip::tcp;
using boost::asio::buffer;
using boost::asio::streambuf;
using boost::asio::io_context;
using boost::asio::deadline_timer;
using boost::asio::ssl::context;
using boost::system::system_error;
using boost::algorithm::trim_if;
using boost::algorithm::is_any_of;


namespace mailio
{


boost::asio::io_context dialog::_ios;


dialog::dialog(const string& hostname, unsigned port, milliseconds timeout) :
    _hostname(hostname), _port(port), _socket(make_shared<tcp::socket>(_ios)), _timer(make_shared<deadline_timer>(_ios)),
    _timeout(timeout), _timer_expired(false), _strmbuf(make_shared<streambuf>())
{
    _istrm = make_shared<istream>(_strmbuf.get());

    try
    {
        if (_timeout.count() == 0)
        {
            tcp::resolver res(_ios);
            boost::asio::connect(*_socket, res.resolve(_hostname, to_string(_port)));
        }
        else
            connect_async();
    }
    catch (system_error&)
    {
        throw dialog_error("Server connecting failed.");
    }
}


dialog::dialog(const dialog& other) : _hostname(move(other._hostname)), _port(other._port), _socket(other._socket), _timer(other._timer),
    _timeout(other._timeout), _timer_expired(other._timer_expired), _strmbuf(other._strmbuf)
{
    _istrm = other._istrm;
}


void dialog::send(const string& line)
{
    if (_timeout.count() == 0)
        send_sync(*_socket, line);
    else
        send_async(*_socket, line);
}


// TODO: perhaps the implementation should be common with `receive_raw()`
string dialog::receive(bool raw)
{
    if (_timeout.count() == 0)
        return receive_sync(*_socket, raw);
    else
        return receive_async(*_socket, raw);
}


template<typename Socket>
void dialog::send_sync(Socket& socket, const string& line)
{
    try
    {
        string l = line + "\r\n";
        write(socket, buffer(l, l.size()));
    }
    catch (system_error&)
    {
        throw dialog_error("Network sending error.");
    }
}


template<typename Socket>
string dialog::receive_sync(Socket& socket, bool raw)
{
    try
    {
        read_until(socket, *_strmbuf, "\n");
        string line;
        getline(*_istrm, line, '\n');
        if (!raw)
            trim_if(line, is_any_of("\r\n"));
        return line;
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
}


void dialog::connect_async()
{
    tcp::resolver res(_ios);

    _timer->expires_at(boost::posix_time::pos_infin);
    _timer->expires_from_now(boost::posix_time::milliseconds(_timeout.count()));
    boost::system::error_code ignored_ec;
    check_timeout(ignored_ec, _socket, _timer, _timer_expired);

    bool has_connected{false}, connect_error{false};
    async_connect(*_socket, res.resolve(_hostname, to_string(_port)),
        [&has_connected, &connect_error](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint&)
        {
            if (!error)
                has_connected = true;
            else
                connect_error = true;
        });
    do
    {
        if (_timer_expired)
            throw dialog_error("Server connecting timed out.");
        if (connect_error)
            throw dialog_error("Server connecting failed.");
        _ios.run_one();
    }
    while (!has_connected);
}


template<typename Socket>
void dialog::send_async(Socket& socket, string line)
{
    _timer->expires_from_now(boost::posix_time::milliseconds(_timeout.count()));
    string l = line + "\r\n";
    bool has_written{false}, send_error{false};
    async_write(socket, buffer(l, l.size()), [&has_written, &send_error](const boost::system::error_code& error, size_t)
        {
            if (!error)
                has_written = true;
            else
                send_error = true;
        });
    do
    {
        if (_timer_expired)
            throw dialog_error("Network sending timed out.");
        if (send_error)
            throw dialog_error("Network sending failed.");
        _ios.run_one();
    }
    while (!has_written);
}


template<typename Socket>
string dialog::receive_async(Socket& socket, bool raw)
{
    _timer->expires_from_now(boost::posix_time::milliseconds(_timeout.count()));
    bool has_read{false}, receive_error{false};
    string line;
    async_read_until(socket, *_strmbuf, "\n", [&has_read, &receive_error, this, &line, raw](const boost::system::error_code& error, size_t)
        {
            if (!error)
            {
                getline(*_istrm, line, '\n');
                if (!raw)
                    trim_if(line, is_any_of("\r\n"));
                has_read = true;
            }
            else
                receive_error = true;
        });
    do
    {
        if (_timer_expired)
            throw dialog_error("Network receiving timed out.");
        if (receive_error)
            throw dialog_error("Network receiving failed.");
        _ios.run_one();
    }
    while (!has_read);

    return line;
}


void dialog::check_timeout(const boost::system::error_code& error, shared_ptr<tcp::socket> socket, shared_ptr<deadline_timer> timer, bool& expired)
{
    if (timer->expires_at() <= deadline_timer::traits_type::now())
    {
        expired = true;
        boost::system::error_code ignored_ec;
        socket->close(ignored_ec);
        timer->expires_at(boost::posix_time::pos_infin);
    }
    timer->async_wait(bind(&dialog::check_timeout, error, socket, timer, expired));
}


dialog_ssl::dialog_ssl(const string& hostname, unsigned port, milliseconds timeout) :
    dialog(hostname, port, timeout), _ssl(false), _context(make_shared<context>(context::sslv23)),
    _ssl_socket(make_shared<boost::asio::ssl::stream<tcp::socket&>>(*_socket, *_context))
{
}


dialog_ssl::dialog_ssl(const dialog& other) : dialog(other), _context(make_shared<context>(context::sslv23)),
    _ssl_socket(make_shared<boost::asio::ssl::stream<tcp::socket&>>(*(dialog::_socket), *_context))
{
    try
    {
        _ssl_socket->set_verify_mode(boost::asio::ssl::verify_none);
        _ssl_socket->handshake(boost::asio::ssl::stream_base::client);
        _ssl = true;
    }
    catch (system_error&)
    {
        // TODO: perhaps the message is confusing
        throw dialog_error("Switching to SSL failed.");
    }
}


void dialog_ssl::send(const string& line)
{
    if (!_ssl)
    {
        dialog::send(line);
        return;
    }

    if (_timeout.count() == 0)
        send_sync(*_ssl_socket, line);
    else
        send_async(*_ssl_socket, line);
}


string dialog_ssl::receive(bool raw)
{
    if (!_ssl)
        return dialog::receive(raw);

    try
    {
        if (_timeout.count() == 0)
            return receive_sync(*_ssl_socket, raw);
        else
            return receive_async(*_ssl_socket, raw);
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
}


} // namespace mailio
