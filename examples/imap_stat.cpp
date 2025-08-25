/*

imaps_stat.cpp
--------------
  
Connects to an IMAP server over START TLS and gets the number of messages in mailbox.


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
        // connect to server
        imap conn("imap.zoho.com", 143);
        // modify to use an existing zoho account
        conn.authenticate("mailio@zoho.com", "mailiopass", imap::auth_method_t::LOGIN);
        // query inbox statistics
        imap::mailbox_stat_t stat = conn.statistics("inbox");
        cout << "Number of messages in mailbox: " << stat.messages_no << endl;
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
