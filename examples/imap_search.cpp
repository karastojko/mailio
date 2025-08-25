/*

imaps_search.cpp
----------------

Connects to an IMAP server over START TLS and searches for the messages which satisfy the criteria.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <mailio/imap.hpp>


using mailio::message;
using mailio::codec;
using mailio::imap;
using mailio::imap_error;
using mailio::dialog_error;
using std::cout;
using std::endl;
using std::for_each;
using std::list;
using std::string;


int main()
{
    try
    {
        imap conn("imap-mail.outlook.com", 143);
        conn.start_tls(true);// // no need for this since it is the default setting
        // modify username/password to use real credentials
        conn.authenticate("mailio@outlook.com", "mailiopass", imap::auth_method_t::LOGIN);
        conn.select(list<string>({"Inbox"}));
        list<unsigned long> messages;
        list<imap::search_condition_t> conds;
        conds.push_back(imap::search_condition_t(imap::search_condition_t::BEFORE_DATE, boost::gregorian::date(2018, 6, 22)));
        conds.push_back(imap::search_condition_t(imap::search_condition_t::SUBJECT, "mailio"));
        conn.search(conds, messages, true);
        for_each(messages.begin(), messages.end(), [](unsigned int msg_uid){ cout << msg_uid << endl; });
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
