/*

dialog.cpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/

#include <string>
#include <algorithm>
#include <future>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <mailio/dialog.hpp>


using std::string;
using std::to_string;
using std::move;
using std::istream;
using std::future;
using std::future_status;
using boost::asio::ip::tcp;
using boost::asio::buffer;
using boost::asio::streambuf;
using boost::asio::io_service;
using boost::asio::ssl::context;
using boost::system::system_error;
using boost::algorithm::trim_if;
using boost::algorithm::is_any_of;


namespace mailio
{


dialog::dialog(const string& hostname, unsigned port, unsigned long timeout) :
    _hostname(hostname), _port(port), _ios(new io_service()), _socket(*_ios), _timer(*_ios), _strmbuf(new streambuf()), _istrm(new istream(_strmbuf.get())),
    _timeout(timeout), _timer_expired(false)
{
    try
    {
        if (_timeout == 0)
        {
            tcp::resolver res(*_ios);
            tcp::resolver::query q(_hostname, to_string(_port));
            tcp::resolver::iterator it = res.resolve(q);
            tcp::endpoint end_point = *it;
            _socket.connect(end_point);
        }
        else
            connect_async();
    }
    catch (system_error&)
    {
        throw dialog_error("Server connecting failed.");
    }    
}


dialog::dialog(dialog&& other) :
    _hostname(move(other._hostname)), _port(other._port), _socket(move(other._socket)), _timer(move(other._timer)), _timeout(other._timeout),
    _timer_expired(other._timer_expired)
{
    _ios.reset(other._ios.release());
    _strmbuf.reset(other._strmbuf.release());
    _istrm.reset(other._istrm.release());
}


dialog::~dialog()
{
    try
    {
        _socket.close();
    }
    catch (...)
    {
    }
}


void dialog::send(const string& line)
{
    if (_timeout == 0)
        send_sync(_socket, line);
    else
        send_async(_socket, line);
}


// TODO: perhaps the implementation should be common with `receive_raw()`
string dialog::receive(bool raw)
{
    if (_timeout == 0)
        return receive_sync(_socket, raw);
    else
        return receive_async(_socket, raw);
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
    tcp::resolver res(*_ios);
    tcp::resolver::query q(_hostname, to_string(_port));
    tcp::resolver::iterator it = res.resolve(q);
    tcp::endpoint end_point = *it;

    _timer.expires_at(boost::posix_time::pos_infin);
    _timer.expires_from_now(boost::posix_time::milliseconds(_timeout));
    boost::system::error_code error;
    check_timeout(error);

    bool has_connected = false;
    _socket.async_connect(end_point, [&has_connected](const boost::system::error_code& error)
    {
        if (!error)
            has_connected = true;
        else
            throw dialog_error("Server connecting failed.");
    });
    do
    {
        if (_timer_expired)
            throw dialog_error("Server connecting timed out.");
        _ios->run_one();
    }
    while (!has_connected);
}


template<typename Socket>
void dialog::send_async(Socket& socket, string line)
{
    _timer.expires_from_now(boost::posix_time::milliseconds(_timeout));
    bool has_written = false;
    string l = line + "\r\n";
    async_write(socket, buffer(l, l.size()), [&has_written](const boost::system::error_code& error, size_t /*bytes_no*/)
    {
        if (!error)
            has_written = true;
        else
            throw dialog_error("Network sending failed.");
    });
    do
    {
        if (_timer_expired)
            throw dialog_error("Network sending timed out.");
        _ios->run_one();
    }
    while (!has_written);
}


template<typename Socket>
string dialog::receive_async(Socket& socket, bool raw)
{
    _timer.expires_from_now(boost::posix_time::milliseconds(_timeout));
    bool has_read = false;
    string line;
    async_read_until(socket, *_strmbuf, "\n", [&has_read, this, &line, raw](const boost::system::error_code& error, size_t /*bytes_no*/)
    {
        if (!error)
        {
            getline(*_istrm, line, '\n');
            if (!raw)
                trim_if(line, is_any_of("\r\n"));
            has_read = true;
        }
        else
            throw dialog_error("Network receiving failed.");
    });
    do
    {
        if (_timer_expired)
            throw dialog_error("Network receiving timed out.");
        _ios->run_one();
    }
    while (!has_read);
    return line;
}


void dialog::check_timeout(const boost::system::error_code& error)
{
    if (_timer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        _timer_expired = true;
        boost::system::error_code ignored_ec;
        _socket.close(ignored_ec);
        _timer.expires_at(boost::posix_time::pos_infin);
    }
    _timer.async_wait(std::bind(&dialog::check_timeout, this, error));
}


dialog_ssl::dialog_ssl(const string& hostname, unsigned port, unsigned long timeout) :
    dialog(hostname, port, timeout), _ssl(false), _context(context::sslv23), _ssl_socket(_socket, _context)
{
}


dialog_ssl::dialog_ssl(dialog&& dlg) : dialog(move(dlg)), _context(context::sslv23), _ssl_socket(dialog::_socket, _context)
{
    try
    {
        _ssl_socket.set_verify_mode(boost::asio::ssl::verify_none);
        _ssl_socket.handshake(boost::asio::ssl::stream_base::client);
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

    if (_timeout == 0)
        send_sync(_ssl_socket, line);
    else
        send_async(_ssl_socket, line);
}


string dialog_ssl::receive(bool raw)
{
    if (!_ssl)
        return dialog::receive(raw);

    try
    {
        if (_timeout == 0)
            return receive_sync(_ssl_socket, raw);
        else
            return receive_async(_ssl_socket, raw);
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
}


} // namespace mailio
