/*

imap_remove_msg.cpp
-------------------
  
Connects to IMAP server and removes a message in mailbox.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <iostream>
#include <mailio/imap.hpp>


using mailio::imap;
using mailio::imap_error;
using mailio::dialog_error;
using std::cout;
using std::endl;


int main()
{
    try
    {
        // use a server with plain (non-SSL) connectivity
        imap conn("imap.mailserver.com", 143);
        // modify to use real account
        conn.authenticate("mailio@mailserver.com", "mailiopass", imap::auth_method_t::LOGIN);
        // remove first message from mailbox
        conn.remove("inbox", 1);
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
