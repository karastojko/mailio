/*

pop3s_fetch_one.cpp
-------------------

Connects to POP3 server via SSL and fetches first message from mailbox.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <mailio/message.hpp>
#include <mailio/pop3.hpp>
#include <iostream>


using mailio::message;
using mailio::codec;
using mailio::pop3s;
using mailio::pop3_error;
using mailio::dialog_error;
using std::cout;
using std::endl;


int main() {
    try {
        // mail message to store the fetched one
        message msg;
        // set the line policy to mandatory, so longer lines could be parsed
        msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::MANDATORY);

        std::string dovecotServerHost = "mail.wenchao.fit";
        int dovecotServerPop3Port = 110;
        std::string dovecotServerAuthUsername = "test@wenchao.fit";
        std::string dovecotServerAuthPassword = "wc123456";
        // connect to server

        unsigned long mail_cursor = 1;
        unsigned long mail_no = 0;
        while (true) {

            // mail message to store the fetched one
            message msg;
            // set the line policy to mandatory, so longer lines could be parsed
            msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::MANDATORY);

            pop3s conn(dovecotServerHost, dovecotServerPop3Port);
            // modify to use existing yahoo account
            conn.authenticate(dovecotServerAuthUsername, dovecotServerAuthPassword, pop3s::auth_method_t::LOGIN);
//            conn.authenticate(dovecotServerAuthUsername, dovecotServerAuthPassword, pop3s::auth_method_t::LOGIN);
            mailio::pop3::mailbox_stat_t mail_info = conn.statistics();
            mail_no = mail_info.messages_no;
            if(mail_no >= mail_cursor) {
                conn.fetch(mail_cursor, msg);
                cout << "mail no: " << mail_no << endl;
                cout << "current mail index: " << mail_cursor << endl;
                cout << "current mail content: " << msg.content()<< endl;
                cout << endl;
                mail_cursor ++;
            }

        }

        // fetch the first message from mailbox
//        conn.fetch(7, msg);
//        cout << msg.subject() << endl;
    }
    catch (pop3_error &exc) {
        cout << exc.what() << endl;
    }
    catch (dialog_error &exc) {
        cout << exc.what() << endl;
    }

    return EXIT_SUCCESS;
}
