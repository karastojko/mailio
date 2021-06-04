/*

message.hpp
-----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
#include <vector>
#include <list>
#include <utility>
#include <stdexcept>
#include <memory>
#include <tuple>
#include <istream>
#include <boost/date_time.hpp>
#include "q_codec.hpp"
#include "mime.hpp"
#include "mailboxes.hpp"
#include "export.hpp"


namespace mailio
{


/**
Mail message and applied parsing/formatting algorithms.
**/
class MAILIO_EXPORT message : public mime
{
public:

    /**
    Enclosing character for the mail address begin.
    **/
    static const char ADDRESS_BEGIN_CHAR = '<';

    /**
    String representation of the mail address start enclosing character.
    **/
    static const std::string ADDRESS_BEGIN_STR;

    /**
    Enclosing character for the mail address end.
    **/
    static const char ADDRESS_END_CHAR = '>';

    /**
    String representation of the mail address end enclosing character.
    **/
    static const std::string ADDRESS_END_STR;

    /**
    Character to separate mail addresses in a list.
    **/
    static const char ADDRESS_SEPARATOR = ',';

    /**
    Mail group name separator from the list of addresses.
    **/
    static const char MAILGROUP_NAME_SEPARATOR = ':';

    /**
    Separator of several mail groups.
    **/
    static const char MAILGROUP_SEPARATOR = ';';

    /**
    Calling parent destructor, initializing date and time to local time in utc time zone, other members set to default.
    **/
    message();

    /**
    Default copy constructor.
    **/
    message(const message&) = default;

    /**
    Default move constructor.

    @todo Default implementation is probably a bug, but not manifested yet.
    **/
    message(message&&) = default;

    /**
    Default destructor.
    **/
    ~message() = default;

    /**
    Default assignment operator.
    **/
    message& operator=(const message&) = default;

    /**
    Default move assignment operator.

    @todo Default implementation is probably a bug, but not manifested yet.
    **/
    message& operator=(message&&) = default;

    /**
    Formatting the message to a string.

    If a line contains leading dot, then it can be escaped as required by mail protocols.

    @param message_str Resulting message as string.
    @param dot_escape  Flag if the leading dot should be escaped.
    @throw *           `format_header()`, `format_content(bool)`, `mime::format(string&, bool)`.
    **/
    void format(std::string& message_str, bool dot_escape = false) const;

    /**
    Parsing a message from a string.

    Essentially, the method calls the same one from `mime` and checks for errors.

    @param message_str   String to parse.
    @param dot_escape    Flag if the leading dot should be escaped.
    @throw message_error No author address.
    @throw *             `mime::parse(const string&, bool)`.
    **/
    void parse(const std::string& message_str, bool dot_escape = false);

    /**
    Checking if the mail is empty.

    @return True if empty, false if not.
    **/
    bool empty() const;

    /**
    Setting the author to a given address.

    The given address is set as the only one, others are deleted.

    @param mail Mail address to set.
    **/
    void from(const mail_address& mail);

    /**
    Getting the author address.

    @return Author mail address.
    **/
    mailboxes from() const;

    /**
    Adding an addrress to the author field.

    @param mail Mail address to set.
    **/
    void add_from(const mail_address& mail);

    /**
    Formatting the author as string.

    @return  Author name and address as formatted string.
    @throw * `format_address(const string&, const string&)`.
    **/
    std::string from_to_string() const;

    /**
    Setting the sender to the given address.

    @param mail Mail address to set.
    **/
    void sender(const mail_address& mail);

    /**
    Getting the sender address.

    @return Sender mail address.
    **/
    mail_address sender() const;

    /**
    Formatting the sender as string.

    @return  Sender name and address as formatted string.
    @throw * `format_address(const string&, const string&)`.
    **/
    std::string sender_to_string() const;

    /**
    Setting the reply address.

    @param mail Reply mail address.
    **/
    void reply_address(const mail_address& mail);

    /**
    Getting the reply address.

    @return Reply mail address.
    **/
    mail_address reply_address() const;

    /**
    Formatting the reply name and address as string.

    @return  Reply name and address as string.
    @throw * `format_address(const string&, const string&)`.
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
    Getting the recipients.

    @return List of recipients.
    **/
    mailboxes recipients() const;

    /**
    Getting the recipients names and addresses as string.

    @return  Recipients names and addresses as string.
    @throw * `format_mailbox`.
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
    Getting the CC recipients names and addresses.

    @return List of CC recipients.
    **/
    mailboxes cc_recipients() const;

    /**
    Getting the CC recipients names and addresses as string.

    @return  CC recipients names and addresses as string.
    @throw * `format_mailbox`.
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
    void add_bcc_recipient(const mail_group& group);

    /**
    Getting the BCC recipients names and addresses.

    @return List of BCC recipients.
    **/
    mailboxes bcc_recipients() const;

    /**
    Getting the BCC recipients names and addresses as string.

    @return  BCC recipients names and addresses as string.
    @throw * `format_mailbox`.
    **/
    std::string bcc_recipients_to_string() const;

    /**
    Setting the disposition notification mail address.

    @param mail Mail address to set.
    **/
    void disposition_notification(const mail_address& mail);

    /**
    Getting the disposition notification mail address.

    @return Dispostion notification mail address.
    **/
    mail_address disposition_notification() const;

    /**
    Getting the disposition notification mail address as string.

    @return  Disposition notification mail address as string.
    @throw * `format_address(const string&, const string&)`.
    **/
    std::string disposition_notification_to_string() const;

    /**
    Setting the message ID.

    @param id            The message ID in the format `string1@string2`.
    @throw message_error Invalid message ID.
    **/
    void message_id(std::string id);

    /**
    Getting the message ID.

    @return Message ID.
    **/
    std::string message_id() const;

    /**
    Adding the in-reply-to ID.

    @param in-reply ID of the in-reply-to header.
    **/
    void add_in_reply_to(const std::string& in_reply);

    /**
    Getting the in-reply-to ID.

    @return List of in-reply-to IDs.
    **/
    std::vector<std::string> in_reply_to() const;

    /**
    Adding the reference ID to the list.

    @param reference_id Reference ID.
    **/
    void add_references(const std::string& reference_id);

    /**
    Getting the references list of IDs.

    @return List of references IDs.
    **/
    std::vector<std::string> references() const;

    /**
    Setting the subject.

    @param mail_subject Subject to set.
    */
    void subject(const std::string& mail_subject);

    /**
    Getting the subject.

    @return Subject value.
    **/
    std::string subject() const;

    /**
    Getting the date, time and zone.

    @return Date, time and zone.
    **/
    boost::local_time::local_date_time date_time() const;

    /**
    Setting the date, time and zone.

    @param the_date_time Date, time and zone to set.
    **/
    void date_time(const boost::local_time::local_date_time& mail_dt);

    /**
    Attaching a file with the given media type.

    @param att_strm Stream to read the attachment.
    @param att_name Attachment name to set.
    @param type     Attachment media type to set.
    @param subtype  Attachment media subtype to set.
    @throw *        `mime::content_type(const content_type_t&)`, `mime::content_transfer_encoding(content_transfer_encoding_t)`,
                    `mime::content_disposition(content_disposition_t)`.
    **/
    [[deprecated]]
    void attach(const std::istream& att_strm, const std::string& att_name, media_type_t type, const std::string& subtype);

    /**
    Attaching a list of streams.

    If the content is set, attaching a file moves the content to the first MIME part. Thus, the content and the attached files are MIME parts, as described in
    RFC 2046 section 5.1. The consequence is that the content remains empty afterwards.

    @param attachments Files to attach. Each tuple consists of a stream, attachment name and content type.
    @throw *           `mime::content_type(const content_type_t&)`, `mime::content_transfer_encoding(content_transfer_encoding_t)`,
                       `mime::content_disposition(content_disposition_t)`.
    **/
    void attach(const std::list<std::tuple<std::istream&, std::string, content_type_t>>& attachments);

    /**
    Getting the number of attachments.

    @return Number of attachments.
    **/
    std::size_t attachments_size() const;

    /**
    Getting the attachment at the given index.

    @param index         Index of the attachment.
    @param att_strm      Stream to write the attachment.
    @param att_name      Name of the attachment.
    @throw message_error Bad attachment index.
    **/
    void attachment(std::size_t index, std::ostream& att_strm, std::string& att_name) const;

    /**
    Adding another header.

    Adding a header defined by other methods leads to the undefined behaviour.

    @param name  Header name.
    @param value Header value.
    @todo        Disallowing standard headers defined elsewhere?
    **/
    void add_header(const std::string& name, const std::string& value);

    /**
    Removing another header.

    Removing a header defined by other methods leads to the undefined behaviour.

    @param name Header to remove.
    **/
    void remove_header(const std::string& name);

    /**
    Returning the other headers.

    @return Message headers.
    **/
    std::multimap<std::string, std::string> headers() const;

protected:

    /**
    Printable ASCII characters without the alphanumerics, double quote, comma, colon, semicolon, angle and square brackets and monkey.
    **/
    static const std::string ATEXT;

    /**
    Printable ASCII characters without the alphanumerics, brackets and backslash.
    **/
    static const std::string DTEXT;

    /**
    `From` header name.
    **/
    static const std::string FROM_HEADER;

    /**
    `Sender` header name.
    **/
    static const std::string SENDER_HEADER;

    /**
    `Reply-To` header name.
    **/
    static const std::string REPLY_TO_HEADER;

    /**
    `To` header name.
    **/
    static const std::string TO_HEADER;

    /**
    `Cc` header name.
    **/
    static const std::string CC_HEADER;

    /**
    `Bcc` header name.
    **/
    static const std::string BCC_HEADER;

    /**
    `Message-ID` header name.
    **/
    static const std::string MESSAGE_ID_HEADER;

    /**
    `In-Reply-To` header name.
    **/
    static const std::string IN_REPLY_TO_HEADER;

    /**
    `References` header name.
    **/
    static const std::string REFERENCES_HEADER;

    /**
    Subject header name.
    **/
    static const std::string SUBJECT_HEADER;

    /**
    Date header name.
    **/
    static const std::string DATE_HEADER;


    /**
    Disposition notification header name.
    **/
    static const std::string DISPOSITION_NOTIFICATION_HEADER;

    /**
    Mime version header name.
    **/
    static const std::string MIME_VERSION_HEADER;

    /**
    Formatting the header to a string.

    @return              Header as string.
    @throw message_error No boundary for multipart message.
    @throw message_error No author.
    @throw message_error No sender for multiple authors.
    @throw *             `mime::format_header()`.
    **/
    virtual std::string format_header() const;

    /**
    Parsing a header line for a specific header.

    @param header_line   Header line to be parsed.
    @throw message_error Line policy overflow in a header.
    @throw message_error Empty author header.
    @throw *             `mime::parse_header_line(const string&)`, `mime::parse_header_name_value(const string&, string&, string&)`,
                         `parse_address_list(const string&)`, `parse_subject(const string&)`, `parse_date(const string&)`.
    **/
    virtual void parse_header_line(const std::string& header_line);

    /**
    Formatting a list of addresses to string.

    Multiple addresses are put into separate lines.

    @param mailbox_list  Mailbox to format.
    @return              Mailbox as string.
    @throw message_error Formatting failure of address list, bad group name.
    @throw *             `format_address(const string&, const string&)`.
    **/
    std::string format_address_list(const mailboxes& mailbox_list) const;

    /**
    Formatting a name and an address.

    If UTF-8 is detected in the name, the given header codec is used.

    @param name          Mail name.
    @param address       Mail address.
    @return              The mail name and address formatted.
    @throw message_error Formatting failure of name, line policy overflow.
    @throw message_error Formatting failure of name.
    @throw message_error Formatting failure of address.
    **/
    std::string format_address(const std::string& name, const std::string& address) const;

    /**
    Parsing a string into vector of names and addresses.

    @param address_list  String to parse.
    @return              Vector of names and addresses.
    @throw message_error Parsing failure of address or group at.
    @throw message_error Parsing failure of group at.
    @throw message_error Parsing failure of name or address at.
    @throw message_error Parsing failure of address at.
    @throw message_error Parsing failure of name at.
    @throw message_error Parsing failure of comment at.
    **/
    mailboxes parse_address_list(const std::string& address_list) const;

    /**
    Parsing a string into date and time.

    @param date_str      Date string to parse.
    @return              Date and time translated to local time zone.
    @throw message_error Parsing failure of date.
    **/
    boost::local_time::local_date_time parse_date(const std::string& date_str) const;

    /**
    Formatting the vector of IDs.

    @param ids Vector of IDs.
    @return    String of IDs in the angle brackets.
    **/
    static std::string format_many_ids(const std::vector<std::string>& ids);

    /**
    Formatting the ID.

    @param id ID to format.
    @return   ID within the angle brackets.
    **/
    static std::string format_many_ids(const std::string& id);

    /**
    Parsing a string of IDs into a vector.

    @param ids           String of IDs within the angle brackets.
    @return              Vector of IDs.
    @throw message_error Parsing failure of the message ID.
    **/
    std::vector<std::string> parse_many_ids(const std::string& ids) const;

    /**
    Formatting the subject which can be ASCII or UTF-8.

    @return Formatted subject.
    **/
    std::string format_subject() const;

    /**
    Parsing a subject which can be ASCII or UTF-8.

    The result is string either ASCII or UTF-8 encoded. If another encoding is used like ISO-8859-X, then the result is undefined.

    @param subject       Subject to parse.
    @return              Parsed subject.
    @throw message_error Parsing failure of Q encoding.
    @throw *             `q_codec::decode(const string&)`.
    **/
    std::string parse_subject(const std::string& subject) const;

    /**
    Parsing a name part of a mail ASCII or UTF-8 encoded.

    The result is string ASCII or UTF-8 encoded. If another encoding is used, then it should be decoded by the method caller.

    @param address_name  Name part of mail.
    @return              Parsed name part of the address.
    @todo                Not tested with charsets different than ASCII and UTF-8.
    **/
    std::string parse_address_name(const std::string& address_name) const;

    /**
    From name and address.
    **/
    mailboxes _from;

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
    Disposition notification address.
    **/
    mail_address _disposition_notification;

    /**
    Message ID.
    **/
    std::string _message_id;

    /**
    In reply to list of IDs.
    **/
    std::vector<std::string> _in_reply_to;

    /**
    References list of IDs.
    **/
    std::vector<std::string> _references;

    /**
    Message subject.
    **/
    std::string _subject;

    /**
    Message date and time with time zone.
    **/
    boost::local_time::local_date_time _date_time;

    /**
    Other headers not included into the known ones.
    **/
    std::multimap<std::string, std::string> _headers;
};


/**
Exception reported by `message` class.
**/
class message_error : public std::runtime_error
{
public:

    /**
    Calling parent constructor.

    @param msg Error message.
    **/
    explicit message_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calling parent constructor.

    @param msg Error message.
    **/
    explicit message_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


} // namespace mailio
