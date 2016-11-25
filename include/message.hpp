/*

message.hpp
-----------

Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/

 
#pragma once

#include <string>
#include <vector>
#include <utility>
#include <stdexcept>
#include <memory>
#include <tuple>
#include <istream>
#include <boost/date_time.hpp>
#include "mime.hpp"


namespace mailio
{


/**
Represents mail message and applies parsing/formatting algorithms.
**/
class message : public mime
{
public:

    /**
    Sender info as name and address.
    **/
    struct mail_info
    {
        /**
        Name part of the email information.
        **/
        std::string name;

        /**
        Address part of the email information.
        **/
        std::string address;

        /**
        Defaut constructor.
        **/
        mail_info() = default;

        /**
        Sets mail information.

        @param the_name    Name to set.
        @param the_address Address to set.
        **/
        mail_info(const std::string& the_name, const std::string& the_address) : name(the_name), address(the_address)
        {
        }

        /**
        Assigns mail information.

        @param other Mail to assign.
        @return      This object.
        **/
        mail_info& operator=(const mail_info& other)
        {
            if (this != &other)
            {
                name = other.name;
                address = other.address;
            }
            return *this;
        }

        /**
        Checks if mail information is empty, i.e. name and address are empty.

        @return True if empty, false if not.
        **/
        bool empty() const
        {
            return name.empty() && address.empty();
        }

        /**
        Clear all information.
        **/
        void clear()
        {
            name.clear();
            address.clear();
        }
    };

public:

    /**
    Calls parent destructor, initializes date and time to local time in utc time zone, other members are set to default.
    **/
    message();

    /**
    Copy constructor is default.
    **/
    message(const message&) = default;

    /**
    Move constructor is default.
    
    @todo Default implementation is probably a bug, but not manifested yet.
    **/
    message(message&&) = default;

    /**
    Destructor is default.
    **/
    ~message() = default;

    /**
    Assignment operator is default.
    **/
    message& operator=(const message&) = default;

    /**
    Move assignment operator is default.
    
    @todo Default implementation is probably a bug, but not manifested yet.
    **/
    message& operator=(message&&) = default;

    /**
    Formats message to a string.

    If a line contains leading dot, then it can be escaped as required by mail protocols.

    @param message_str   Resulting message as string.
    @param dot_escape    Flag if the leading dot should be escaped.
    @throw *             `format_header`, `format_content`, `mime::format`.
    **/
    void format(std::string& message_str, bool dot_escape = false) const;

    /**
    Checks if mail is empty.

    @return True if empty, false if not.
    **/
    bool empty() const;

    /**
    Sets sender to given name and address.

    @param name    Sender name.
    @param address Sender address.
    **/
    void sender(const std::string& name, const std::string& address);

    /**
    Gets sender name and address.

    @return Sender name and address info.
    **/
    mail_info sender() const;

    /**
    Gets sender name and address as formatted string.

    @return  Sender name and address as formatted string.
    @throw * 'format_address'.
    **/
    std::string sender_to_string() const;

    /**
    Sets reply to given name and address.

    @param name    Reply name.
    @param address Reply address.
    **/
    void reply_address(const std::string& name, const std::string& address);

    /**
    Gets reply name and address.

    @return Reply name and address.
    **/
    mail_info reply_address() const;

    /**
    Gets reply name and address as string.

    @return  Reply name and address as string.
    @throw * 'format_address'.
    **/
    std::string reply_address_to_string() const;

    /**
    Adds recipent name and address.

    @param name    Reply name.
    @param address Reply address.
    **/
    void add_recipient(const std::string& name, const std::string& address);

    /**
    Gets recipients names and addresses.

    @return Vector of recipients names and addresses.
    **/
    std::vector<mail_info> recipients() const;

    /**
    Gets recipients names and addresses as string.

    @return  Recipients names and addresses as string.
    @throw * 'format_address'.
    **/
    std::string recipients_to_string() const;

    /**
    Adds cc recipent name and address.

    @param name    Cc recipent name.
    @param address Cc recipent address.
    **/
    void add_cc_recipient(const std::string& name, const std::string& address);

    /**
    Gets cc recipients names and addresses.

    @return Vector of cc recipients names and addresses.
    **/
    std::vector<mail_info> cc_recipients() const;

    /**
    Gets cc recipients names and addresses as string.

    @return  Cc recipients names and addresses as string.
    @throw * 'format_address'.
    **/
    std::string cc_recipients_to_string() const;

    /**
    Adds bcc recipent name and address.

    @param name    Bcc recipent name.
    @param address Bcc recipent address.
    **/
    void add_bcc_recipient(const std::string& name, const std::string& address);

    /**
    Gets bcc recipients names and addresses.

    @return Vector of bcc recipients names and addresses.
    **/
    std::vector<mail_info> bcc_recipients() const;

    /**
    Gets bcc recipients names and addresses as string.

    @return  Bcc recipients names and addresses as string.
    @throw * 'format_address'.
    **/
    std::string bcc_recipients_to_string() const;

    /**
    Sets subject.

    @param mail_subject Subject to set.
    */
    void subject(const std::string& mail_subject);

    /**
    Gets subject.

    @return Subject value.
    **/
    std::string subject() const;

    /**
    Gets date, time and zone.

    @return Date, time and zone.
    **/
    boost::local_time::local_date_time date_time();

    /**
    Sets date, time and zone.

    @param the_date_time Date, time and zone to set.
    **/
    void date_time(const boost::local_time::local_date_time& mail_dt);

    /**
    Attaches file with the given media type.

    @param att_strm Stream to read the attachment.
    @param att_name Attachment name to set.
    @param type     Attachment media type to set.
    @param subtype  Attachment media subtype to set.
    @throw *        `mime::content_type`, `mime::content_transfer_encoding`, `mime::content_disposition`, `mime::name`, `mime::content`,
                    `stringstream::stringstream`, `istream::rdbuf`.
    **/
    void attach(const std::istream& att_strm, const std::string& att_name, media_type_t type, const std::string& subtype);

    /**
    Gets number of attachments.
    
    @return Number of attachments.
    **/
    std::size_t attachments_size() const;
    
    /**
    Gets attachment at the given index.
    
    @param index         Index of the attachment.
    @param att_strm      Stream to write the attachment.
    @param att_name      Name of the attachment.
    @throw message_error No attachment at the given index.
    **/
    void attachment(std::size_t index, std::ostream& att_strm, std::string& att_name);
    
protected:

    /**
    Alphanumerics plus some special character allowed in the address.
    **/
    static const std::string ATEXT;

    /**
    Printable ascii not including brackets and backslash.
    **/
    static const std::string DTEXT;

    /**
    From header name.
    **/
    static const std::string FROM_HEADER;

    /**
    Reply To header name.
    **/
    static const std::string REPLY_TO_HEADER;

    /**
    To header name.
    **/
    static const std::string TO_HEADER;

    /**
    Cc header name.
    **/
    static const std::string CC_HEADER;

    /**
    Bcc header name.
    **/
    static const std::string BCC_HEADER;

    /**
    Subject header name.
    **/
    static const std::string SUBJECT_HEADER;

    /**
    Date header name.
    **/
    static const std::string DATE_HEADER;

    /**
    Mime version header name.
    **/
    static const std::string MIME_VERSION_HEADER;

    /**
    Formats header to a string.

    @return Header as string.
    @throw  Non multipart message with boundary.
    **/
    virtual std::string format_header() const;

    /**
    Parses a header line for a specific header.

    @param header_line   Header line to be parsed.
    @throw message_error Bad sender name or address.
    @throw *             `mime::parse_header_line`, `mime::parse_header_name_value`, `parse_address_list`, `parse_date`.
    **/
    virtual void parse_header_line(const std::string& header_line);

    /**
    Formats sender name and address.

    @param name          Sender name.
    @param address       Sender mail address.
    @return              Sender name and address formatted.
    @throw message_error Bad name or address.
    **/
    std::string format_address(const std::string& name, const std::string& address) const;

    /**
    Parses a string into vector of names and addresses.

    @param address       String to parse.
    @return              Vector of names and addresses.
    @throw message_error Bad name or address.
    **/
    static std::vector<mail_info> parse_address_list(const std::string& address_list);

    /**
    Parses a string into date and time.
    
    @param date_str       Date string to parse.
    @return               Date and time translated to local time zone.
    @throw message_error  Bad date format.
    **/
    static boost::local_time::local_date_time parse_date(const std::string& date_str);

    /**
    Sender name and address.
    **/
    mail_info _sender;

    /**
    Reply name and address.
    **/
    mail_info _reply_address;

    /**
    Vector of recipient addresses and names.
    **/
    std::vector<mail_info> _recipients;

    /**
    Vector of cc recipient addresses and names.
    **/
    std::vector<mail_info> _cc_recipients;

    /**
    Vector of bcc recipient addresses and names.
    **/
    std::vector<mail_info> _bcc_recipients;

    /**
    Message subject.
    **/
    std::string _subject;

    /**
    Message date and time with time zone.
    **/
    std::shared_ptr<boost::local_time::local_date_time> _date_time;
};


class message_error : public std::runtime_error
{
public:

    /**
    Calls parent constructor.

    @param msg Error message.
    **/
    explicit message_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calls parent constructor.

    @param msg Error message.
    **/
    explicit message_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


} // namespace mailio
