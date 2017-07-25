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
#include "q_codec.hpp"
#include "mime.hpp"
#include "mailboxes.hpp"


namespace mailio
{


/**
Represents mail message and applies parsing/formatting algorithms.
**/
class message : public mime
{
public:

    /**
    Calling parent destructor, initializing date and time to local time in utc time zone, other members are set to default.
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
    Formatting message to a string.

    If a line contains leading dot, then it can be escaped as required by mail protocols.

    @param message_str   Resulting message as string.
    @param dot_escape    Flag if the leading dot should be escaped.
    @throw *             `format_header`, `format_content`, `mime::format`.
    **/
    void format(std::string& message_str, bool dot_escape = false) const;

    /**
    Checking if mail is empty.

    @return True if empty, false if not.
    **/
    bool empty() const;

    /**
    Setting sender to the given address.

    @param mail Mail address to set.
    **/
    void sender(const mail_address& mail);

    /**
    Getting sender.

    @return Sender mail address.
    **/
    mail_address sender() const;

    /**
    Getting sender as formatted string.

    @return  Sender name and address as formatted string.
    @throw * 'format_address'.
    **/
    std::string sender_to_string() const;

    /**
    Setting reply address.

    @param mail Reply mail address.
    **/
    void reply_address(const mail_address& mail);

    /**
    Getting reply address.

    @return Reply mail address.
    **/
    mail_address reply_address() const;

    /**
    Getting reply name and address as string.

    @return  Reply name and address as string.
    @throw * 'format_address'.
    **/
    std::string reply_address_to_string() const;

    /**
    Adding a recipent name and address.

    @param mail Address to add.
    **/
    void add_recipient(const mail_address& mail);

    /**
    Adding a recipient group.

    @param group Group to add.
    **/
    void add_recipient(const mail_group& group);

    /**
    Gets recipients.

    @return List of recipients.
    **/
    mailboxes recipients() const;

    /**
    Gets recipients names and addresses as string.

    @return  Recipients names and addresses as string.
    @throw * 'format_mailbox'.
    **/
    std::string recipients_to_string() const;

    /**
    Adding a CC recipent name and address.

    @param mail Mail address to set.
    **/
    void add_cc_recipient(const mail_address& mail);

    /**
    Adding a CC recipient group.

    @param group Group to add.
    **/
    void add_cc_recipient(const mail_group& group);

    /**
    Getting CC recipients names and addresses.

    @return List of CC recipients.
    **/
    mailboxes cc_recipients() const;

    /**
    Getting CC recipients names and addresses as string.

    @return  CC recipients names and addresses as string.
    @throw * 'format_mailbox'.
    **/
    std::string cc_recipients_to_string() const;

    /**
    Adding a BCC recipent name and address.

    @param mail Mail address to set.
    **/
    void add_bcc_recipient(const mail_address& mail);

    /**
    Adding a BCC recipient group.

    @param group Group to add.
    **/
    void add_bcc_recipient(const mail_group& mail);

    /**
    Getting BCC recipients names and addresses.

    @return List of BCC recipients.
    **/
    mailboxes bcc_recipients() const;

    /**
    Getting BCC recipients names and addresses as string.

    @return  BCC recipients names and addresses as string.
    @throw * 'format_mailbox'.
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

    /**
    Codec used for header fields.
    **/
    typedef q_codec::codec_method_t header_codec_t;

    /**
    Sets headers codec.

    @param hdr_codec Codec to set.
    **/
    void header_codec(header_codec_t hdr_codec);

    /**
    Gets headers codec.

    @return Codec set.
    **/
    header_codec_t header_codec() const;
    
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
    @throw message_error Bad sender.
    @throw *             `mime::parse_header_line`, `mime::parse_header_name_value`, `parse_address_list`, `parse_date`.
    **/
    virtual void parse_header_line(const std::string& header_line);

    /**
    Formatting mailbox list to string.

    @param mailbox_list  Mailbox to format.
    @return              Mailbox as string.
    @throw message_error Bad address or group.
    **/
    std::string format_mailbox(const mailboxes& mailbox_list) const;

    /**
    Formatting sender name and address.

    @param name          Sender name.
    @param address       Sender mail address.
    @return              Sender name and address formatted.
    @throw message_error Bad address or group.
    **/
    std::string format_address(const std::string& name, const std::string& address) const;

    /**
    Parsing a string into vector of names and addresses.

    @param address       String to parse.
    @return              Vector of names and addresses.
    @throw message_error Bad address or group.
    **/
    static mailboxes parse_address_list(const std::string& address_list);

    /**
    Parses a string into date and time.
    
    @param date_str       Date string to parse.
    @return               Date and time translated to local time zone.
    @throw message_error  Bad date format.
    **/
    static boost::local_time::local_date_time parse_date(const std::string& date_str);

    /**
    Parses message subject.

    @param subject Subject to parse.
    @return Parsed subject.
    @throw * `q_codec::decode`.
    **/
    std::string parse_subject(const std::string& subject);

    /**
    Sender name and address.
    **/
    mail_address _sender;

    /**
    Reply address.
    **/
    mail_address _reply_address;

    /**
    List of recipients.
    **/
    mailboxes _recipients;

    /**
    List of CC recipients.
    **/
    mailboxes _cc_recipients;

    /**
    List of BCC recipients.
    **/
    mailboxes _bcc_recipients;

    /**
    Message subject.
    **/
    std::string _subject;

    /**
    Message date and time with time zone.
    **/
    std::shared_ptr<boost::local_time::local_date_time> _date_time;

    /**
    Codec used for headers.
    **/
    header_codec_t _header_codec;
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
