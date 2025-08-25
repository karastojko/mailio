/*

imaps_folders.cpp
-----------------

Connects to an IMAP server over SSL and lists recursively all folders.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <algorithm>
#include <iostream>
#include <string>
#include <mailio/imap.hpp>


using mailio::imap;
using mailio::imap_error;
using mailio::dialog_error;
using std::for_each;
using std::cout;
using std::endl;
using std::string;


void print_folders(unsigned tabs, const imap::mailbox_folder_t& folder)
{
    string indent(tabs, '\t');
    for (auto& f : folder.folders)
    {
        cout << indent << f.first << endl;
        if (!f.second.folders.empty())
            print_folders(++tabs, f.second);
    }
}


int main()
{
    try
    {
        imap conn("imap.mailserver.com", 993);
        conn.start_tls(false);
        // modify username/password to use real credentials
        conn.authenticate("mailio@mailserver.com", "mailiopass", imap::auth_method_t::LOGIN);
        imap::mailbox_folder_t fld = conn.list_folders("");
        print_folders(0, fld);
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
