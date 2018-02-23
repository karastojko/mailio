/*

smtps_attachment.cpp
--------------------
 
Connects to SMTP server via SSL and sends a message with attached files.

 
Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <iostream>
#include <fstream>
#include <libmailio/message.hpp>
#include <libmailio/smtp.hpp>


using mailio::message;
using mailio::mail_address;
using mailio::smtps;
using mailio::smtp_error;
using mailio::dialog_error;
using std::cout;
using std::endl;
using std::ifstream;


int main()
{
    try
    {
        // create mail message
        message msg;
        msg.sender(mail_address("mailio library", "mailio@gmail.com"));// set the correct sender name and address
        msg.add_recipient(mail_address("mailio library", "mailio@gmail.com"));// set the correct recipent name and address
        msg.subject("smtps message with attachment");
        ifstream ifs1("aleph0.png");
        msg.attach(ifs1, "aleph0.png", message::media_type_t::IMAGE, "png");
        ifstream ifs2("infinity.png");
        msg.attach(ifs2, "infinity.png", message::media_type_t::IMAGE, "png");
        msg.content("Here are Aleph0 and Infinity pictures.");

        // use a server with plain (non-SSL) connectivity
        smtps conn("smtp.mailserver.com", 465);
        // modify username/password to use real credentials
        conn.authenticate("mailio@mailserver.com", "mailiopass", smtps::auth_method_t::LOGIN);
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
