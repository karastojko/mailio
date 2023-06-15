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


boost::asio::io_context dialog::ios_;


dialog::dialog(const string& hostname, unsigned port, milliseconds timeout) : std::enable_shared_from_this<dialog>(),
    hostname_(hostname), port_(port), socket_(make_shared<tcp::socket>(ios_)), timer_(make_shared<deadline_timer>(ios_)),
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
    catch (system_error&)
    {
        throw dialog_error("Server connecting failed.");
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
        read_until(socket, *strmbuf_, "\n");
        string line;
        getline(*istrm_, line, '\n');
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
    tcp::resolver res(ios_);

    timer_->expires_at(boost::posix_time::pos_infin);
    timer_->expires_from_now(boost::posix_time::milliseconds(timeout_.count()));
    boost::system::error_code ignored_ec;
    check_timeout();

    bool has_connected{false}, connect_error{false};
    async_connect(*socket_, res.resolve(hostname_, to_string(port_)),
        [&has_connected, &connect_error](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint&)
        {
            if (!error)
                has_connected = true;
            else
                connect_error = true;
        });
    do
    {
        if (timer_expired_)
            throw dialog_error("Server connecting timed out.");
        if (connect_error)
            throw dialog_error("Server connecting failed.");
        ios_.run_one();
    }
    while (!has_connected);
}


template<typename Socket>
void dialog::send_async(Socket& socket, string line)
{
    timer_->expires_from_now(boost::posix_time::milliseconds(timeout_.count()));
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
        if (timer_expired_)
            throw dialog_error("Network sending timed out.");
        if (send_error)
            throw dialog_error("Network sending failed.");
        ios_.run_one();
    }
    while (!has_written);
}


template<typename Socket>
string dialog::receive_async(Socket& socket, bool raw)
{
    timer_->expires_from_now(boost::posix_time::milliseconds(timeout_.count()));
    bool has_read{false}, receive_error{false};
    string line;
    async_read_until(socket, *strmbuf_, "\n", [&has_read, &receive_error, this, &line, raw](const boost::system::error_code& error, size_t)
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
        });
    do
    {
        if (timer_expired_)
            throw dialog_error("Network receiving timed out.");
        if (receive_error)
            throw dialog_error("Network receiving failed.");
        ios_.run_one();
    }
    while (!has_read);

    return line;
}


void dialog::check_timeout()
{
    if (timer_->expires_at() <= deadline_timer::traits_type::now())
    {
        boost::system::error_code ignored_ec;
        socket_->close(ignored_ec);
        timer_->expires_at(boost::posix_time::pos_infin);
        timer_expired_ = true;
    }
    timer_->async_wait(bind(&dialog::check_timeout, shared_from_this()));
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
    catch (system_error&)
    {
        // TODO: perhaps the message is confusing
        throw dialog_error("Switching to SSL failed.");
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
    catch (system_error&)
    {
        throw dialog_error("Network receiving error.");
    }
}


} // namespace mailio
