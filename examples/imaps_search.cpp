/*

imaps_search.cpp
----------------

Connects to IMAP server and searches for messages which satisfy the criteria.


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
using mailio::imaps;
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
        imaps conn("imap-mail.outlook.com", 993);
        // modify username/password to use real credentials
        conn.authenticate("mailio@outlook.com", "mailiopass", imaps::auth_method_t::LOGIN);
        conn.select(list<string>({"Inbox"}));
        list<unsigned long> messages;
        list<imaps::search_condition_t> conds;
        conds.push_back(imaps::search_condition_t(imaps::search_condition_t::BEFORE_DATE, boost::gregorian::date(2018, 6, 22)));
        conds.push_back(imaps::search_condition_t(imaps::search_condition_t::SUBJECT, "mailio"));
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
