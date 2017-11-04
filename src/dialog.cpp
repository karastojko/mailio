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
#include <dialog.hpp>


using std::string;
using std::to_string;
using std::move;
using std::istream;
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


dialog::dialog(const string& hostname, unsigned port) : _hostname(hostname), _port(port), _ios(new io_service()),  _socket(*_ios), _strmbuf(new streambuf()), _istrm(new istream(_strmbuf.get()))
{
    try
    {
        tcp::resolver res(*_ios);
        tcp::resolver::query q(_hostname, to_string(_port));
        tcp::resolver::iterator it = res.resolve(q);
        tcp::endpoint end_point = *it;
        _socket.connect(end_point);
    }
    catch (system_error&)
    {
        throw dialog_error("Server connecting failed.");
    }    
}


dialog::dialog(dialog&& other) : _hostname(move(other._hostname)), _port(other._port), _socket(move(other._socket))
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
    try
    {
        string l = line + "\r\n";
        write(_socket, buffer(l, l.size()));
    }
    catch (system_error&)
    {
        throw dialog_error("Network sending error.");
    }
}


// TODO: perhaps the implementation should be common with `receive_raw()`
string dialog::receive()
{
    try
    {
        read_until(_socket, *_strmbuf, "\n");
        string line;
        getline(*_istrm, line, '\n');
        trim_if(line, is_any_of("\r\n"));
        return line;
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
}


string dialog::receive_raw()
{
    try
    {
        read_until(_socket, *_strmbuf, "\n");
        string line;
        getline(*_istrm, line, '\n');
        return line;
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
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
        string l = line + "\r\n";
        write(_ssl_socket, buffer(l, l.size()));
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
        read_until(_ssl_socket, *_strmbuf, "\n");
        string line;
        getline(*_istrm, line, '\n');
        trim_if(line, is_any_of("\r\n"));
        return line;
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
        read_until(_ssl_socket, *_strmbuf, "\n");
        string line;
        getline(*_istrm, line, '\n');
        return line;
    }
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }

}


} // namespace mailio
