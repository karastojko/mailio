/*

imaps_stat.cpp
--------------
  
Connects to an IMAP server over SSL and gets the first message from the inbox.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <iostream>
#include <mailio/imap.hpp>


using mailio::message;
using mailio::codec;
using mailio::imap;
using mailio::imap_error;
using mailio::dialog_error;
using std::cout;
using std::endl;


int main()
{
    try
    {
        imap conn("imap.zoho.com", 993);
        conn.start_tls(false);
        conn.authenticate("mailio@zoho.com", "mailiopass", imap::auth_method_t::LOGIN);
        message msg;
        msg.line_policy(codec::line_len_policy_t::MANDATORY);
        conn.fetch("inbox", 1, false, msg);
        cout << "msg.content()=" << msg.content() << endl;
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
