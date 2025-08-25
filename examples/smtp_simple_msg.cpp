/*

smtps_simple_msg.cpp
--------------------

Connects to an SMTP server via START_TLS and sends a simple message.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <iostream>
#include <mailio/message.hpp>
#include <mailio/smtp.hpp>


using mailio::message;
using mailio::mail_address;
using mailio::smtp;
using mailio::smtp_error;
using mailio::dialog_error;
using std::cout;
using std::endl;


int main()
{
    try
    {
        // create mail message
        message msg;
        msg.from(mail_address("mailio library", "mailio@gmail.com"));// set the correct sender name and address
        msg.add_recipient(mail_address("mailio library", "mailio@gmail.com"));// set the correct recipent name and address
        msg.subject("smtps simple message");
        msg.content("Hello, World!");

        // connect to server
        smtp conn("smtp.gmail.com", 587);
        // modify username/password to use real credentials
        conn.authenticate("mailio@gmail.com", "mailiopass", smtp::auth_method_t::LOGIN);
        conn.submit(msg);
    }
    catch (smtp_error& exc)
    {
        cout << exc.what() << endl;
    }
    catch (dialog_error& exc)
    {
        cout << exc.what() << endl;
    }

    return EXIT_SUCCESS;
}
