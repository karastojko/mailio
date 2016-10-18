/*

imap_stat.cpp
-------------
  
Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <imap.hpp>
#include <iostream>


using mailio::imaps;
using std::cout;
using std::endl;

int main()
{
    // connect to server
    imaps conn("imap.zoho.com", 993);
    conn.authenticate("mailio@zoho.com", "mailiopass", imaps::auth_method_t::LOGIN);// modify to use existing zoho account
    // query inbox statistics
    imaps::mailbox_stat_t stat = conn.statistics("inbox");
    cout << "Number of messages in mailbox: " << stat.messages_no << endl;

    return EXIT_SUCCESS;
}
