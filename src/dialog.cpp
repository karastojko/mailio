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
using boost::asio::steady_timer;
using boost::asio::ssl::context;
using boost::system::system_error;
using boost::system::error_code;
using boost::algorithm::trim_if;
using boost::algorithm::is_any_of;


namespace mailio
{


boost::asio::io_context dialog::ios_;


dialog::dialog(const string& hostname, unsigned port, milliseconds timeout) : std::enable_shared_from_this<dialog>(),
    hostname_(hostname), port_(port), socket_(make_shared<tcp::socket>(ios_)), timer_(make_shared<steady_timer>(ios_)),
    timeout_(timeout), timer_expired_(false), strmbuf_(make_shared<streambuf>()), istrm_(make_shared<istream>(strmbuf_.get()))
{
}


dialog::dialog(const dialog& other) : std::enable_shared_from_this<dialog>(),
    hostname_(move(other.hostname_)), port_(other.port_), socket_(other.socket_), timer_(other.timer_),
    timeout_(other.timeout_), timer_expired_(other.timer_expired_), strmbuf_(other.strmbuf_), istrm_(other.istrm_)
{
}


void dialog::connect()
{
    try
    {
        if (timeout_.count() == 0)
        {
            tcp::resolver res(ios_);
            boost::asio::connect(*socket_, res.resolve(hostname_, to_string(port_)));
        }
        else
            connect_async();
    }
    catch (const system_error& exc)
    {
        throw dialog_error("Server connecting failed.", exc.code().message());
    }
}


void dialog::send(const string& line)
{
    if (timeout_.count() == 0)
        send_sync(*socket_, line);
    else
        send_async(*socket_, line);
}


// TODO: perhaps the implementation should be common with `receive_raw()`
string dialog::receive(bool raw)
{
    if (timeout_.count() == 0)
        return receive_sync(*socket_, raw);
    else
        return receive_async(*socket_, raw);
}


template<typename Socket>
void dialog::send_sync(Socket& socket, const string& line)
{
    try
    {
        string l = line + "\r\n";
        write(socket, buffer(l, l.size()));
    }
    catch (const system_error& exc)
    {
        throw dialog_error("Network sending error.", exc.code().message());
    }
}


template<typename Socket>
string dialog::receive_sync(Socket& socket, bool raw)
{
    try
    {
        read_until(socket, *strmbuf_, "\n");
        string line;
        getline(*istrm_, line, '\n');
        if (!raw)
            trim_if(line, is_any_of("\r\n"));
        return line;
    }
    catch (const system_error& exc)
    {
        throw dialog_error("Network receiving error.", exc.code().message());
    }
}


void dialog::connect_async()
{
    tcp::resolver res(ios_);
    check_timeout();

    bool has_connected{false}, connect_error{false};
    error_code errc;
    async_connect(*socket_, res.resolve(hostname_, to_string(port_)),
        [&has_connected, &connect_error, &errc](const error_code& error, const boost::asio::ip::tcp::endpoint&)
        {
            if (!error)
                has_connected = true;
            else
                connect_error = true;
            errc = error;
        });
    wait_async(has_connected, connect_error, "Network connecting timed out.", "Network connecting failed.", errc);
}


template<typename Socket>
void dialog::send_async(Socket& socket, string line)
{
    check_timeout();
    string l = line + "\r\n";
    bool has_written{false}, send_error{false};
    error_code errc;
    async_write(socket, buffer(l, l.size()),
        [&has_written, &send_error, &errc](const error_code& error, size_t)
        {
            if (!error)
                has_written = true;
            else
                send_error = true;
            errc = error;
        });
    wait_async(has_written, send_error, "Network sending timed out.", "Network sending failed.", errc);
}


template<typename Socket>
string dialog::receive_async(Socket& socket, bool raw)
{
    check_timeout();
    bool has_read{false}, receive_error{false};
    string line;
    error_code errc;
    async_read_until(socket, *strmbuf_, "\n",
        [&has_read, &receive_error, this, &line, &errc, raw](const error_code& error, size_t)
        {
            if (!error)
            {
                getline(*istrm_, line, '\n');
                if (!raw)
                    trim_if(line, is_any_of("\r\n"));
                has_read = true;
            }
            else
                receive_error = true;
            errc = error;
        });
    wait_async(has_read, receive_error, "Network receiving timed out.", "Network receiving failed.", errc);
    return line;
}


void dialog::wait_async(const bool& has_op, const bool& op_error, const char* expired_msg, const char* op_msg, const error_code& error)
{
    do
    {
        if (timer_expired_)
            throw dialog_error(expired_msg, error.message());
        if (op_error)
            throw dialog_error(op_msg, error.message());
        ios_.run_one();
    }
    while (!has_op);
}


void dialog::check_timeout()
{
    // Expiring automatically cancels the timer, per documentation.
    timer_->expires_after(timeout_);
    timer_expired_ = false;
    timer_->async_wait(bind(&dialog::timeout_handler, shared_from_this(), std::placeholders::_1));
}


void dialog::timeout_handler(const error_code& error)
{
    if (!error)
        timer_expired_ = true;
}


dialog_ssl::dialog_ssl(const string& hostname, unsigned port, milliseconds timeout, const ssl_options_t& options) :
    dialog(hostname, port, timeout), ssl_(false), context_(make_shared<context>(options.method)),
    ssl_socket_(make_shared<boost::asio::ssl::stream<tcp::socket&>>(*socket_, *context_))
{
}


dialog_ssl::dialog_ssl(const dialog& other, const ssl_options_t& options) : dialog(other), context_(make_shared<context>(options.method)),
    ssl_socket_(make_shared<boost::asio::ssl::stream<tcp::socket&>>(*socket_, *context_))
{
    try
    {
        ssl_socket_->set_verify_mode(options.verify_mode);
        ssl_socket_->handshake(boost::asio::ssl::stream_base::client);
        ssl_ = true;
    }
    catch (const system_error& exc)
    {
        // TODO: perhaps the message is confusing
        throw dialog_error("Switching to SSL failed.", exc.code().message());
    }
}


void dialog_ssl::send(const string& line)
{
    if (!ssl_)
    {
        dialog::send(line);
        return;
    }

    if (timeout_.count() == 0)
        send_sync(*ssl_socket_, line);
    else
        send_async(*ssl_socket_, line);
}


string dialog_ssl::receive(bool raw)
{
    if (!ssl_)
        return dialog::receive(raw);

    try
    {
        if (timeout_.count() == 0)
            return receive_sync(*ssl_socket_, raw);
        else
            return receive_async(*ssl_socket_, raw);
    }
    catch (const system_error& exc)
    {
        throw dialog_error("Network receiving error.", exc.code().message());
    }
}


string dialog_error::details() const
{
    return details_;
}

} // namespace mailio
