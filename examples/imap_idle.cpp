/*

imap_idle.cpp
-------------

Connects to an IMAP server over START TLS and enters IDLE mode to receive real-time mailbox notifications.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <chrono>
#include <iostream>
#include <list>
#include <string>
#include <mailio/imap.hpp>


using mailio::imap;
using mailio::imap_error;
using mailio::dialog_error;
using std::cout;
using std::endl;
using std::list;
using std::string;


int main()
{
    try
    {
        imap conn("imap-mail.outlook.com", 143);
        conn.start_tls(true); // no need for this since it is the default setting
        // modify username/password to use real credentials
        conn.authenticate("mailio@outlook.com", "mailiopass", imap::auth_method_t::LOGIN);
        conn.select(list<string>({"Inbox"}));

        // Check if the server supports IDLE
        if (!conn.has_capability("IDLE"))
        {
            cout << "Server does not support IDLE." << endl;
            return EXIT_FAILURE;
        }

        cout << "Entering IDLE mode, waiting for notifications..." << endl;
        cout << "(Send an email to the mailbox to see EXISTS notification)" << endl;
        cout << endl;

        // Enter IDLE mode with 30 second timeout for demonstration purposes.
        // Per RFC 2177, IDLE should be re-issued every 29 minutes.
        // Some servers (e.g., Gmail) may timeout earlier (~10 minutes).
        list<imap::idle_response_t> notifications = conn.idle(std::chrono::seconds(600));

        if (notifications.empty())
        {
            cout << "No notifications received (timeout)." << endl;
        }
        else
        {
            cout << "Received " << notifications.size() << " notification(s):" << endl;
            for (const auto& notification : notifications)
            {
                switch (notification.type)
                {
                    case imap::idle_notification_type_t::EXISTS:
                        cout << "EXISTS: " << notification.message_no << " messages in mailbox" << endl;
                        break;
                    case imap::idle_notification_type_t::EXPUNGE:
                        cout << "EXPUNGE: Message " << notification.message_no << " was deleted" << endl;
                        break;
                    case imap::idle_notification_type_t::FETCH:
                        cout << "FETCH: Message " << notification.message_no << " flags changed" << endl;
                        break;
                }
                cout << "  Raw: " << notification.raw_response << endl;
            }
        }
    }
    catch (imap_error& exc)
    {
        cout << exc.what() << endl;
    }
    catch (dialog_error& exc)
    {
        cout << exc.what() << endl;
    }

    return EXIT_SUCCESS;
}
