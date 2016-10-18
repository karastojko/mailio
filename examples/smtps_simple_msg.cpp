/*

smtps_simple_msg.cpp
--------------------
  
Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <message.hpp>
#include <smtp.hpp>


using mailio::message;
using mailio::smtps;


int main()
{
    // create mail message
    message msg;
    msg.sender("mailio library", "mailio@gmail.com");// set the correct sender name and address
    msg.add_recipient("mailio library", "mailio@gmail.com");// set the correct recipent name and address
    msg.subject("smtps simple message");
    msg.content("Hello, World!");

    // connect to server
    smtps conn("smtp.gmail.com", 587);
    conn.authenticate("mailio@gmail.com", "mailiopass", smtps::auth_method_t::START_TLS);// modify to use existing gmail account
    conn.submit(msg);

    return EXIT_SUCCESS;
}
