/*

mailboxes.hpp
-------------

Copyright (C) 2017, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once
#pragma warning(disable:4251)

#include <string>
#include <vector>

#include "export.hpp"

namespace mailio
{


/**
Mail as name and address.
**/
struct MAILIO_EXPORT mail_address
{
    /**
    Name part of the mail.
    **/
    std::string name;

    /**
    Address part of the mail.
    **/
    std::string address;

    /**
    Default constructor.
    **/
    mail_address() = default;

    /**
    Setting a mail name and address.

    @param mail_name    Name to set.
    @param mail_address Address to set.
    **/
    mail_address(const std::string& mail_name, const std::string& mail_address);

    /**
    Checking if a mail is empty, i.e. name and address are empty.

    @return True if empty, false if not.
    **/
    bool empty() const;

    /**
    Clearing name and address.
    **/
    void clear();
};


/**
Mail group with the name and members.
**/
struct MAILIO_EXPORT mail_group
{
    /**
    Mail group name.
    **/
    std::string name;

    /**
    Mail group members.
    **/
    std::vector<mail_address> members;

    /**
    Default constructor.
    **/
    mail_group() = default;

    /**
    Setting a group name and members.

    @param group_name  Name of a group.
    @param group_mails Members of a group.
    **/
    mail_group(const std::string& group_name, const std::vector<mail_address>& group_mails);

    /**
    Default destructor.
    **/
    ~mail_group() = default;

    /**
    Adding a list of mails to members.

    @param mails Mail list to add.
    **/
    void add(const std::vector<mail_address>& mails);

    /**
    Adding a mail to members.

    @param mail Mail to add.
    **/
    void add(const mail_address& mail);

    /**
    Clearing the group name and members.
    **/
    void clear();
};


/**
List of mail addresses and groups.
**/
struct MAILIO_EXPORT mailboxes
{
    /**
    Mail addresses.
    **/
    std::vector<mail_address> addresses;

    /**
    Mail groups.
    **/
    std::vector<mail_group> groups;

    /**
    Default constructor.
    **/
    mailboxes() = default;

    /**
    Default copy constructor.
    **/
    mailboxes(const mailboxes&) = default;

    /**
    Default move constructor.
    **/
    mailboxes(mailboxes&&) = default;

    /**
    Creating a mailbox with the given addresses and groups.

    @param address_list Mail addresses to set.
    @param group_list   Mail groups to set.
    **/
    mailboxes(std::vector<mail_address> address_list, std::vector<mail_group> group_list);

    /**
    Default destructor.
    **/
    ~mailboxes() = default;

    /**
    Default copy assignment operator.
    **/
    mailboxes& operator=(const mailboxes&) = default;

    /**
    Default move assignment operator.
    **/
    mailboxes& operator=(mailboxes&&) = default;

    /**
    Checking if the mailbox is empty.

    @return True if empty, false if not.
    **/
    bool empty() const;

    /**
    Clearing the list of addresses.
    **/
    void clear();
};


} // namespace mailio
