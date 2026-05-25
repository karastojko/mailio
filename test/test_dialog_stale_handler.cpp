/*
test_dialog_stale_handler.cpp
-----------------------------

Copyright (C) 2026, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.
*/


#define BOOST_TEST_MODULE dialog_test

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <mailio/dialog.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <thread>


using boost::asio::ip::tcp;
using std::chrono::milliseconds;
using namespace std::chrono_literals;


// A minimal TCP server that accepts one connection and optionally sends data
// after a delay.  Used to simulate a slow server that triggers a timeout.
class TestServer
{
public:
    explicit TestServer(unsigned short port)
        : work_(io_.get_executor()), acceptor_(io_, tcp::endpoint(tcp::v4(), port))
    {
    }

    void start()
    {
        thread_ = std::thread([this] {
            auto socket = std::make_shared<tcp::socket>(io_);
            acceptor_.async_accept(*socket, [this, socket](boost::system::error_code ec) {
                if (!ec)
                {
                    socket_ = socket;
                }
            });
            io_.run();
        });
    }

    void send_after(milliseconds delay, const std::string& msg)
    {
        boost::asio::post(io_, [this, delay, msg] {
            auto timer = std::make_shared<boost::asio::steady_timer>(io_, delay);
            timer->async_wait([this, timer, msg](boost::system::error_code ec) {
                if (!ec && socket_ && socket_->is_open())
                {
                    boost::asio::async_write(*socket_, boost::asio::buffer(msg),
                        [](boost::system::error_code, std::size_t) {});
                }
            });
        });
    }

    void stop()
    {
        boost::system::error_code ec;
        if (socket_ && socket_->is_open())
        {
            static_cast<void>(socket_->close(ec));
        }
        io_.stop();
        if (thread_.joinable())
        {
            thread_.join();
        }
    }

private:
    boost::asio::io_context                                                  io_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
    tcp::acceptor                                                            acceptor_;
    std::shared_ptr<tcp::socket>                                             socket_;
    std::thread                                                              thread_;
};


BOOST_AUTO_TEST_SUITE(test_dialog_stale_handler)


BOOST_AUTO_TEST_CASE(timeout_then_copy_then_receive)
{
    const unsigned short PORT = 22125;

    TestServer server(PORT);
    server.start();
    std::this_thread::sleep_for(100ms);

    // Step 1: create a dialog with a short timeout and connect.
    auto dlg1 = std::make_shared<mailio::dialog>("127.0.0.1", PORT, 100ms);
    dlg1->connect();

    // Step 2: receive() will time out because the server hasn't sent data yet.
    // Before the fix, this left a stale async_read_until handler on the shared
    // static io_context, which would crash on the next receive().
    BOOST_CHECK_THROW(dlg1->receive(), mailio::dialog_error);

    // Step 3: copy the dialog.  The copy shares the socket/timer/streambuf.
    auto dlg2 = std::make_shared<mailio::dialog>(*dlg1);
    dlg1.reset();

    // Step 4: server sends data.
    server.send_after(50ms, "220 test\r\n");

    // Step 5: receive on the copy.  Before the fix this would either crash
    // (stale handler writing to freed stack memory) or hang (io_context stuck
    // in stopped state).  After the fix it should succeed.
    std::string line = dlg2->receive();
    BOOST_CHECK_EQUAL(line, "220 test");

    server.stop();
}


BOOST_AUTO_TEST_SUITE_END()
