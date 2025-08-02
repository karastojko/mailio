/*

imaps_folders.cpp
-----------------

Connects to IMAP SLL server with SASL authentication and lists recursively all folders.


Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/

#include <algorithm>
#include <iostream>
#include <list>
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
  const string indent(tabs, '\t');
  for (const auto &[fst, snd] : folder.folders)
  {
    cout << indent << fst << endl;
    if (!snd.folders.empty())
      print_folders(++tabs, snd);
  }
}


int main()
{
  try
  {
    imaps conn("imap.gmail.com", 993);
    // modify username/password to use real credentials
    conn.authenticate("XOAUTH2", "accesstoken", imaps::auth_method_t::LOGIN_SASL);
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
