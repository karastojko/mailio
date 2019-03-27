/*

pop3s_attachment.cpp
--------------------

Fetches attachments of a message on POP3 server.

For this sample to be executed properly, use the message sent by `smtps_attachment.cpp`.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <iostream>
#include <string>
#include <fstream>
#include <mailio/message.hpp>
#include <mailio/pop3.hpp>


using mailio::message;
using mailio::codec;
using mailio::pop3s;
using mailio::pop3_error;
using mailio::dialog_error;
using std::cout;
using std::endl;
using std::string;
using std::ofstream;


int main()
{
    try
    {
        // mail message to store the fetched one
        message msg;
        // set the line policy to mandatory, so longer lines could be parsed
        msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::MANDATORY);

        // use a server with SSL connectivity
        pop3s conn("pop3.mailserver.com", 995);
        // modify to use real account
        conn.authenticate("mailio@mailserver.com", "mailiopass", pop3s::auth_method_t::LOGIN);
        // fetch the first message from mailbox
        conn.fetch(1, msg);
        
        ofstream ofs1("alepho.png", std::ios::binary);
        string att1;
        msg.attachment(1, ofs1, att1);
        ofstream ofs2("infiniti.png", std::ios::binary);
        string att2;
        msg.attachment(2, ofs2, att2);
        cout << "Received message with subject `" << msg.subject() << "` and attached files `" <<
            att1 << "` and `" << att2 << "` saved as `alepho.png` and `infiniti.png`." << endl;
    }
    catch (pop3_error& exc)
    {
        cout << exc.what() << endl;
    }
    catch (dialog_error& exc)
    {
        cout << exc.what() << endl;
    }

    return EXIT_SUCCESS;
}
