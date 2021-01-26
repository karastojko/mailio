/*

mailboxes.cpp
-------------

Copyright (C) 2017, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include "mailio/mailboxes.hpp"


using std::string;
using std::vector;


namespace mailio
{


mail_address::mail_address(const string& mail_name, const string& mail_address) : name(mail_name), address(mail_address)
{
}


bool mail_address::empty() const
{
    return name.empty() && address.empty();
}


void mail_address::clear()
{
    name.clear();
    address.clear();
}


mail_group::mail_group(const string& group_name, const vector<mail_address>& group_mails) : name(group_name), members(group_mails)
{
}


void mail_group::add(const vector<mail_address>& mails)
{
    members.insert(members.end(), mails.begin(), mails.end());
}


void mail_group::add(const mail_address& mail)
{
    members.push_back(mail);
}


void mail_group::clear()
{
    name.clear();
    members.clear();
}


mailboxes::mailboxes(vector<mail_address> address_list, vector<mail_group> group_list) : addresses(address_list), groups(group_list)
{
}


bool mailboxes::empty() const
{
    return addresses.empty() && groups.empty();
}


void mailboxes::clear()
{
    addresses.clear();
    groups.clear();
}

} // namespace mailio
