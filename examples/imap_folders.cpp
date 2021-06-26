/*

imaps_folders.cpp
-----------------

Connects to IMAP server and lists recursively all folders.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <algorithm>
#include <iostream>
#include <string>
#include <mailio/imap.hpp>


using mailio::imaps;
using mailio::imap_error;
using mailio::dialog_error;
using std::for_each;
using std::cout;
using std::endl;
using std::string;


void print_folders(unsigned tabs, const imaps::mailbox_folder_t& folder)
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
        imaps conn("imap.mailserver.com", 993);
        // modify username/password to use real credentials
        conn.authenticate("mailio@mailserver.com", "mailiopass", imaps::auth_method_t::LOGIN);
        imaps::mailbox_folder_t fld = conn.list_folders("");
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
