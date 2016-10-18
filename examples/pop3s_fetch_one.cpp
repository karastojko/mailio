/*

pop3s_fetch_one.cpp
-------------------
  
Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <message.hpp>
#include <pop3.hpp>
#include <iostream>


using mailio::message;
using mailio::codec;
using mailio::pop3s;
using std::cout;
using std::endl;

int main()
{
    // mail message to store the fetched one
    // set the line policy to mandatory, so longer lines could be parsed
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY);

    // connect to server
    pop3s conn("pop.mail.yahoo.com", 995);
    conn.authenticate("mailio@yahoo.com", "mailiopass", pop3s::auth_method_t::LOGIN);// modify to use existing yahoo account
    // fetch the first message from mailbox
    conn.fetch(1, msg);
    cout << msg.subject() << endl;

    return EXIT_SUCCESS;
}
