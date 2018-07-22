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


dialog::dialog(const string& hostname, unsigned port, unsigned long timeout) : _hostname(hostname), _port(port), _ios(new io_service()), _socket(*_ios), _timer(*_ios),
    _strmbuf(new streambuf()), _istrm(new istream(_strmbuf.get())), _timeout(timeout), _timer_expired(false)
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
        {
            bool rc = connect_async();
            if (!rc)
                throw dialog_error("Server asynchronous connecting failed.");
        }
    }
    catch (system_error&)
    {
        throw dialog_error("Server connecting failed.");
    }    
}


dialog::dialog(dialog&& other) : _hostname(move(other._hostname)), _port(other._port), _socket(move(other._socket)), _timer(move(other._timer)),
    _timeout(other._timeout), _timer_expired(other._timer_expired)
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
    {
        string l = line + "\r\n";
        bool rc = send_async(l);
        if (!rc)
            throw dialog_error("Network sending error.");
    }
}


// TODO: perhaps the implementation should be common with `receive_raw()`
string dialog::receive()
{
    if (_timeout == 0)
        return receive_sync(_socket, false);
    else
    {
        string line;
        bool rc = receive_async(line);
        if (!rc)
            throw dialog_error("Network receiving error.");
        return line;
    }
}


string dialog::receive_raw()
{
    if (_timeout == 0)
        return receive_sync(_socket, true);
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
        if (raw)
            trim_if(line, is_any_of("\r\n"));
        return line;
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
}


bool dialog::connect_async()
{
    tcp::resolver res(*_ios);
    tcp::resolver::query q(_hostname, to_string(_port));
    tcp::resolver::iterator it = res.resolve(q);
    tcp::endpoint end_point = *it;

    _timer.expires_at(boost::posix_time::pos_infin);
    _timer.expires_from_now(boost::posix_time::milliseconds(_timeout));
    boost::system::error_code error;
    check_deadline(error);

    bool has_connected = false;
    _socket.async_connect(end_point, [&has_connected](const boost::system::error_code& error)
    {
        if (!error)
            has_connected = true;
    });
    do
    {
        if (_timer_expired)
            return false;
        _ios->run_one();
    }
    while (!has_connected);
    return true;
}


bool dialog::send_async(string line)
{
    _timer.expires_from_now(boost::posix_time::milliseconds(_timeout));
    bool has_written = false;
    async_write(_socket, buffer(line, line.size()), [&has_written, &line](const boost::system::error_code& error, size_t /*bytes_no*/)
    {
        if (!error)
            has_written = true;
    });
    do
    {
        if (_timer_expired)
            return false;
        _ios->run_one();
    }
    while (!has_written);
    return true;
}


bool dialog::receive_async(string& line)
{
    _timer.expires_from_now(boost::posix_time::milliseconds(_timeout));
    bool has_read = false;
    async_read_until(_socket, *_strmbuf, "\n", [&has_read, this, &line](const boost::system::error_code& error, size_t /*bytes_no*/)
    {
        if (!error)
        {
            getline(*_istrm, line, '\n');
            trim_if(line, is_any_of("\r\n"));
            has_read = true;
        }

    });
    do
    {
        if (_timer_expired)
            return false;
        _ios->run_one();
    }
    while (!has_read);
    return true;
}


void dialog::check_deadline(const boost::system::error_code& error)
{
    if (_timer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        _timer_expired = true;
        std::cout << "Deadline has passed." << std::endl;
        boost::system::error_code ignored_ec;
        _socket.close(ignored_ec);
        _timer.expires_at(boost::posix_time::pos_infin);
    }
    _timer.async_wait(std::bind(&dialog::check_deadline, this, error));
}


dialog_ssl::dialog_ssl(const string& hostname, unsigned port) : dialog(hostname, port), _ssl(false), _context(context::sslv23), _ssl_socket(_socket, _context)
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

    try
    {
        if (_timeout == 0)
            send_sync(_ssl_socket, line);
    }
    catch (system_error&)
    {
        throw dialog_error("Network sending error.");
    }
}


string dialog_ssl::receive()
{
    if (!_ssl)
        return dialog::receive();

    try
    {
        if (_timeout == 0)
            return receive_sync(_ssl_socket, false);
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
}


string dialog_ssl::receive_raw()
{
    if (!_ssl)
        return dialog::receive_raw();

    try
    {
        if (_timeout == 0)
            return receive_sync(_ssl_socket, true);
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
}


} // namespace mailio
