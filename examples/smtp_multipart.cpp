/*

smtps_multipart.cpp
-------------------

Connects to an SMTP server via START_TLS and sends a multipart message.


Copyright (C) 2023, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <iostream>
#include <fstream>
#include <sstream>
#include <mailio/message.hpp>
#include <mailio/smtp.hpp>


using mailio::message;
using mailio::mail_address;
using mailio::mime;
using mailio::smtp;
using mailio::smtp_error;
using mailio::dialog_error;
using std::cout;
using std::endl;
using std::ifstream;
using std::ostringstream;


int main()
{
    try
    {
        // create mail message
        message msg;
        msg.from(mail_address("mailio library", "mailio@mailserver.com"));// set the correct sender name and address
        msg.add_recipient(mail_address("mailio library", "mailio@mailserver.com"));// set the correct recipent name and address
        msg.subject("smtps multipart message");
        msg.boundary("012456789@mailio.dev");
        msg.content_type(message::media_type_t::MULTIPART, "related");

        mime title;
        title.content_type(message::media_type_t::TEXT, "html", "utf-8");
        title.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_8);
        title.content("<html><head></head><body><h1>Здраво, Свете!</h1></body></html>");

        ifstream ifs("aleph0.png");
        ostringstream ofs;
        ofs << ifs.rdbuf();

        mime img;
        img.content_type(message::media_type_t::IMAGE, "png");
        img.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
        img.content_disposition(mime::content_disposition_t::INLINE);
        img.content(ofs.str());
        img.name("a0.png");

        msg.add_part(title);
        msg.add_part(img);

        // connect to server over start tls
        smtp conn("smtp.mailserver.com", 587);
        conn.start_tls(true);// no need for this since it is the default setting
        // modify username/password to use real credentials
        conn.authenticate("mailio@mailserver.com", "mailiopass", smtp::auth_method_t::LOGIN);
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
