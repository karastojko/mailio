/*

message.cpp
-----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <vector>
#include <list>
#include <map>
#include <stdexcept>
#include <utility>
#include <locale>
#include <istream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <mailio/codec.hpp>
#include <mailio/base64.hpp>
#include <mailio/quoted_printable.hpp>
#include <mailio/bit7.hpp>
#include <mailio/bit8.hpp>
#include <mailio/q_codec.hpp>
#include <mailio/mime.hpp>
#include <mailio/message.hpp>


using std::string;
#if defined(__cpp_char8_t)
using std::u8string;
#endif
using std::vector;
using std::list;
using std::multimap;
using std::pair;
using std::make_pair;
using std::locale;
using std::ios_base;
using std::istream;
using std::ostream;
using std::stringstream;
using std::shared_ptr;
using std::make_shared;
using std::tuple;
using std::size_t;
using std::get;
using boost::trim_copy;
using boost::trim;
using boost::iequals;
using boost::split;
using boost::regex;
using boost::regex_match;
using boost::smatch;
using boost::sregex_iterator;
using boost::local_time::local_date_time;
using boost::local_time::local_time_input_facet;
using boost::local_time::not_a_date_time;
using boost::posix_time::second_clock;
using boost::posix_time::ptime;
using boost::local_time::time_zone_ptr;
using boost::local_time::posix_time_zone;
using boost::local_time::local_time_facet;


namespace mailio
{

const string message::ATEXT{"!#$%&'*+-./=?^_`{|}~"};
const string message::DTEXT{"!#$%&'*+-.@/=?^_`{|}~"}; // atext with monkey
const string message::FROM_HEADER{"From"};
const string message::SENDER_HEADER{"Sender"};
const string message::REPLY_TO_HEADER{"Reply-To"};
const string message::TO_HEADER{"To"};
const string message::CC_HEADER{"Cc"};
const string message::BCC_HEADER{"Bcc"};
const string message::MESSAGE_ID_HEADER{"Message-ID"};
const string message::IN_REPLY_TO_HEADER{"In-Reply-To"};
const string message::REFERENCES_HEADER{"References"};
const string message::SUBJECT_HEADER{"Subject"};
const string message::DATE_HEADER{"Date"};
const string message::DISPOSITION_NOTIFICATION_HEADER{"Disposition-Notification-To"};
const string message::MIME_VERSION_HEADER{"MIME-Version"};


message::message() : mime(), date_time_(second_clock::universal_time(), time_zone_ptr(new posix_time_zone("00:00")))
{
}


void message::format(string& message_str, bool dot_escape) const
{
    message_str += format_header();

    if (!parts_.empty())
    {
        if (!content_.empty())
        {
            mime content_part;
            content_part.content(content_);
            content_type_t ct(media_type_t::TEXT, "plain");
            ct.charset = content_type_.charset;
            content_part.content_type(ct);
            content_part.content_transfer_encoding(encoding_);
            content_part.line_policy(line_policy_, decoder_line_policy_);
            content_part.strict_mode(strict_mode_);
            content_part.strict_codec_mode(strict_codec_mode_);
            content_part.header_codec(header_codec_);
            string cps;
            content_part.format(cps, dot_escape);
            message_str += BOUNDARY_DELIMITER + boundary_ + codec::END_OF_LINE + cps + codec::END_OF_LINE;
        }

        // recursively format mime parts
        for (const auto& p: parts_)
        {
            string p_str;
            p.format(p_str, dot_escape);
            message_str += BOUNDARY_DELIMITER + boundary_ + codec::END_OF_LINE + p_str + codec::END_OF_LINE;
        }
        message_str += BOUNDARY_DELIMITER + boundary_ + BOUNDARY_DELIMITER + codec::END_OF_LINE;
    }
    else
        message_str += format_content(dot_escape);
}


#if defined(__cpp_char8_t)
void message::format(u8string& message_str, bool dot_escape) const
{
    string m = reinterpret_cast<const char*>(message_str.c_str());
    format(m, dot_escape);
}
#endif


void message::parse(const string& message_str, bool dot_escape)
{
    mime::parse(message_str, dot_escape);

    if (from_.addresses.size() == 0)
        throw message_error("No author address.");

    // There is no check if there is a sender in case of multiple authors, not sure if that logic is needed.
}


#if defined(__cpp_char8_t)
void message::parse(const u8string& message_str, bool dot_escape)
{
    parse(reinterpret_cast<const char*>(message_str.c_str()), dot_escape);
}
#endif


bool message::empty() const
{
    return content_.empty();
}


void message::from(const mail_address& mail)
{
    from_.clear();
    from_.addresses.push_back(mail);
}


mailboxes message::from() const
{
    return from_;
}


void message::add_from(const mail_address& mail)
{
    from_.addresses.push_back(mail);
}


string message::from_to_string() const
{
    return format_address_list(from_);
}


void message::sender(const mail_address& mail)
{
    sender_ = mail;
}


mail_address message::sender() const
{
    return sender_;
}


string message::sender_to_string() const
{
    return format_address(sender_.name, sender_.address);
}

void message::reply_address(const mail_address& mail)
{
    reply_address_ = mail;
}


mail_address message::reply_address() const
{
    return reply_address_;
}


string message::reply_address_to_string() const
{
    return format_address(reply_address_.name, reply_address_.address);
}


void message::add_recipient(const mail_address& mail)
{
    recipients_.addresses.push_back(mail);
}


void message::add_recipient(const mail_group& group)
{
    recipients_.groups.push_back(group);
}


mailboxes message::recipients() const
{
    return recipients_;
}


string message::recipients_to_string() const
{
    return format_address_list(recipients_);
}


void message::add_cc_recipient(const mail_address& mail)
{
    cc_recipients_.addresses.push_back(mail);
}


void message::add_cc_recipient(const mail_group& group)
{
    cc_recipients_.groups.push_back(group);
}


mailboxes message::cc_recipients() const
{
    return cc_recipients_;
}


string message::cc_recipients_to_string() const
{
    return format_address_list(cc_recipients_);
}


void message::add_bcc_recipient(const mail_address& mail)
{
    bcc_recipients_.addresses.push_back(mail);
}


void message::add_bcc_recipient(const mail_group& group)
{
    bcc_recipients_.groups.push_back(group);
}


mailboxes message::bcc_recipients() const
{
    return bcc_recipients_;
}


string message::bcc_recipients_to_string() const
{
    return format_address_list(bcc_recipients_);
}


void message::disposition_notification(const mail_address& address)
{
    disposition_notification_ = address;
}


mail_address message::disposition_notification() const
{
    return disposition_notification_;
}


string message::disposition_notification_to_string() const
{
    return format_address(disposition_notification_.name, disposition_notification_.address);
}


void message::message_id(string id)
{
    const regex r(strict_mode_ ? MESSAGE_ID_REGEX : MESSAGE_ID_REGEX_NS);
    smatch m;

    if (regex_match(id, m, r))
        message_id_ = id;
    else
        throw message_error("Invalid message ID.");
}


string message::message_id() const
{
    return message_id_;
}


void message::add_in_reply_to(const string& in_reply)
{
    const regex r(MESSAGE_ID_REGEX);
    smatch m;
    if (!regex_match(in_reply, m, r))
        throw message_error("Invalid In Reply To ID.");
    in_reply_to_.push_back(in_reply);
}


vector<string> message::in_reply_to() const
{
    return in_reply_to_;
}


void message::add_references(const string& reference_id)
{
    const regex r(MESSAGE_ID_REGEX);
    smatch m;
    if (!regex_match(reference_id, m, r))
        throw message_error("Invalid Reference ID.");
    references_.push_back(reference_id);
}


vector<string> message::references() const
{
    return references_;
}


void message::subject(const string& mail_subject)
{
    subject_.buffer = mail_subject;
    subject_.charset = codec::CHARSET_ASCII;
    if (codec::is_utf8_string(subject_.buffer))
        subject_.charset = codec::CHARSET_UTF8;
}


void message::subject_raw(const string_t& mail_subject)
{
    subject_ = mail_subject;
}


#if defined(__cpp_char8_t)

void message::subject(const u8string& mail_subject)
{
    subject_.buffer = string(reinterpret_cast<const char*>(mail_subject.c_str()));
    subject_.charset = codec::CHARSET_UTF8;
}


void message::subject_raw(const u8string_t& mail_subject)
{
    subject_.buffer = string(reinterpret_cast<const char*>(mail_subject.buffer.c_str()));
    subject_.charset = mail_subject.charset;
}

#endif


string message::subject() const
{
    return subject_.buffer;
}

string_t message::subject_raw() const
{
    return subject_;
}


local_date_time message::date_time() const
{
    return date_time_;
}


void message::date_time(const boost::local_time::local_date_time& mail_dt)
{
    date_time_ = mail_dt;
}


void message::attach(const istream& att_strm, const string& att_name, media_type_t type, const string& subtype)
{
    if (boundary_.empty())
        boundary_ = make_boundary();
    content_type_.type = media_type_t::MULTIPART;
    content_type_.subtype = "mixed";

    stringstream ss;
    ss << att_strm.rdbuf();
    string content = ss.str();

    mime m;
    m.header_codec(this->header_codec());
    m.content_type(content_type_t(type, subtype));
    // content type charset is not set, so it will be treated as us-ascii
    m.content_transfer_encoding(content_transfer_encoding_t::BASE_64);
    m.content_disposition(content_disposition_t::ATTACHMENT);
    m.name(att_name);
    m.content(content);
    parts_.push_back(m);
}


void message::attach(const list<tuple<istream&, string, content_type_t>>& attachments)
{
    if (boundary_.empty())
        boundary_ = make_boundary();

    // the content goes to the first mime part, and then it's deleted
    if (!content_.empty())
    {
        if (content_type_.type == media_type_t::NONE)
            content_type_ = content_type_t(media_type_t::TEXT, "plain");

        mime content_part;
        content_part.content(content_);
        content_part.content_type(content_type_);
        content_part.content_transfer_encoding(encoding_);
        content_part.line_policy(line_policy_, decoder_line_policy_);
        content_part.strict_mode(strict_mode_);
        content_part.strict_codec_mode(strict_codec_mode_);
        content_part.header_codec(header_codec_);
        parts_.push_back(content_part);
        content_.clear();
    }

    content_type_.type = media_type_t::MULTIPART;
    content_type_.subtype = "mixed";
    for (const auto& att : attachments)
    {
        stringstream ss;
        ss << std::get<0>(att).rdbuf();

        mime m;
        m.header_codec(this->header_codec());
        m.content_type(content_type_t(std::get<2>(att)));
        // content type charset is not set, so it will be treated as us-ascii
        m.content_transfer_encoding(content_transfer_encoding_t::BASE_64);
        m.content_disposition(content_disposition_t::ATTACHMENT);
        m.name(std::get<1>(att));
        m.content(ss.str());
        parts_.push_back(m);
    }
}


size_t message::attachments_size() const
{
    size_t no = 0;
    for (auto& m : parts_)
        if (m.content_disposition() == content_disposition_t::ATTACHMENT)
            no++;
    return no;
}


void message::attachment(size_t index, ostream& att_strm, string& att_name) const
{
    if (index == 0)
        throw message_error("Bad attachment index.");

    size_t no = 0;
    for (auto& m : parts_)
        if (m.content_disposition() == content_disposition_t::ATTACHMENT)
        {
            if (++no == index)
            {
                for (auto ch : m.content())
                    att_strm.put(ch);
                att_name = m.name();
                break;
            }
        }

    if (no > parts_.size())
        throw message_error("Bad attachment index.");
}


void message::add_header(const string& name, const string& value)
{
    smatch m;
    if (!regex_match(name, m, mime::HEADER_NAME_REGEX))
        throw message_error("Format failure of the header name `" + name + "`.");
    if (!regex_match(value, m, mime::HEADER_VALUE_REGEX))
        throw message_error("Format failure of the header value `" + value + "`.");
    headers_.insert(make_pair(name, value));
}


void message::remove_header(const std::string& name)
{
    headers_.erase(name);
}


multimap<string, string> message::headers() const
{
    return headers_;
}


string message::format_header() const
{
    if (!boundary_.empty() && content_type_.type != media_type_t::MULTIPART)
        throw message_error("No boundary for multipart message.");

    if (from_.addresses.size() == 0)
        throw message_error("No author.");

    if (from_.addresses.size() > 1 && sender_.empty())
        throw message_error("No sender for multiple authors.");

    string header;
    for_each(headers_.begin(), headers_.end(),
        [&header, this](const auto& hdr)
        {
            header += fold_header_line(hdr.first + HEADER_SEPARATOR_STR + hdr.second) + codec::END_OF_LINE;
        }
    );

    header += FROM_HEADER + HEADER_SEPARATOR_STR + from_to_string() + codec::END_OF_LINE;
    header += sender_.address.empty() ? "" : SENDER_HEADER + HEADER_SEPARATOR_STR + sender_to_string() + codec::END_OF_LINE;
    header += reply_address_.name.buffer.empty() ? "" : REPLY_TO_HEADER + HEADER_SEPARATOR_STR + reply_address_to_string() + codec::END_OF_LINE;
    header += TO_HEADER + HEADER_SEPARATOR_STR + recipients_to_string() + codec::END_OF_LINE;
    header += cc_recipients_.empty() ? "" : CC_HEADER + HEADER_SEPARATOR_STR + cc_recipients_to_string() + codec::END_OF_LINE;
    header += disposition_notification_.empty() ? "" : DISPOSITION_NOTIFICATION_HEADER + HEADER_SEPARATOR_STR +
        format_address(disposition_notification_.name,disposition_notification_.address) + codec::END_OF_LINE;
    string msg_id = format_many_ids(message_id_);
    header += message_id_.empty() ? "" : MESSAGE_ID_HEADER + HEADER_SEPARATOR_STR + msg_id + codec::END_OF_LINE;
    string in_reply = format_many_ids(in_reply_to_);
    header += in_reply_to_.empty() ? "" : IN_REPLY_TO_HEADER + HEADER_SEPARATOR_STR + in_reply + codec::END_OF_LINE;
    string references = format_many_ids(references_);
    header += references_.empty() ? "" : REFERENCES_HEADER + HEADER_SEPARATOR_STR + references + codec::END_OF_LINE;

    // TODO: move formatting datetime to a separate method
    if (!date_time_.is_not_a_date_time())
    {
        stringstream ss;
        ss.exceptions(std::ios_base::failbit);
        local_time_facet* facet = new local_time_facet("%a, %d %b %Y %H:%M:%S %q");
        ss.imbue(locale(ss.getloc(), facet));
        ss << date_time_;
        header += DATE_HEADER + HEADER_SEPARATOR_STR + ss.str() + codec::END_OF_LINE;
    }

    if (!parts_.empty())
        header += MIME_VERSION_HEADER + HEADER_SEPARATOR_STR + version_ + codec::END_OF_LINE;
    header += mime::format_header();

    header += SUBJECT_HEADER + HEADER_SEPARATOR_STR + format_subject().buffer + codec::END_OF_LINE;

    return header;
}


/*
TODO: parsing address list does not check the line policy
TODO: other headers should check for the line policy as well?

Some of the headers cannot be empty by RFC, but still they can occur. Thus, parser strict mode has to be introduced; in case it's false, default
values are set. The following headers are recognized by the parser:
- `From` cannot be empty by RFC 5322, section 3.6.2. So far, empty field did not occur, so no need to set default mode when empty.
- `Reply-To` is optional by RFC 5322, section 3.6.2. So far, empty field did not occur, so no need to set default mode when empty.
- `Sender` is optional by RFC 5322, section 3.6.2.
- `To` cannot be empty by RFC 5322, section 3.6.3. So far, empty field did not occur, so no need to set default mode when empty.
- `Cc` cannot be empty by RFC 5322, section 3.6.3. So far, empty field did not occur, so no need to set default mode when empty.
- `Subject` can be empty.
- `Date` can be empty.
- `Message-ID` cannot be empty.
- `MIME-Version` cannot be empty by RFC 2045, section 4. In case it's empty, set it to `1.0`.
*/
void message::parse_header_line(const string& header_line)
{
    mime::parse_header_line(header_line);

    // TODO: header name and header value already parsed in `mime::parse_header_line`, so this is not the optimal way to do it
    string header_name, header_value;
    parse_header_name_value(header_line, header_name, header_value);

    if (iequals(header_name, FROM_HEADER))
    {
        from_ = parse_address_list(header_value);
        if (from_.addresses.empty())
            throw message_error("Empty author header.");
    }
    else if (iequals(header_name, SENDER_HEADER))
    {
        mailboxes mbx = parse_address_list(header_value);
        if (!mbx.addresses.empty())
            sender_ = mbx.addresses[0];
    }
    else if (iequals(header_name, REPLY_TO_HEADER))
    {
        mailboxes mbx = parse_address_list(header_value);
        if (!mbx.addresses.empty())
            reply_address_ = mbx.addresses[0];
    }
    else if (iequals(header_name, TO_HEADER))
    {
        recipients_ = parse_address_list(header_value);
    }
    else if (iequals(header_name, CC_HEADER))
    {
        cc_recipients_ = parse_address_list(header_value);
    }
    else if (iequals(header_name, DISPOSITION_NOTIFICATION_HEADER))
    {
        mailboxes mbx = parse_address_list(header_value);
        if (!mbx.addresses.empty())
            disposition_notification_ = mbx.addresses[0];
    }
    else if (iequals(header_name, MESSAGE_ID_HEADER))
    {
        auto ids = parse_many_ids(header_value);
        if (!ids.empty())
            message_id_ = ids[0];
    }
    else if (iequals(header_name, IN_REPLY_TO_HEADER))
        in_reply_to_ = parse_many_ids(header_value);
    else if (iequals(header_name, REFERENCES_HEADER))
        references_ = parse_many_ids(header_value);
    else if (iequals(header_name, SUBJECT_HEADER))
        std::tie(subject_.buffer, subject_.charset) = parse_subject(header_value);
    else if (iequals(header_name, DATE_HEADER))
        date_time_ = parse_date(trim_copy(header_value));
    else if (iequals(header_name, MIME_VERSION_HEADER))
        version_ = trim_copy(header_value);
    else
    {
        if (!iequals(header_name, CONTENT_TYPE_HEADER) && !iequals(header_name, CONTENT_TRANSFER_ENCODING_HEADER) &&
            !iequals(header_name, CONTENT_DISPOSITION_HEADER))
        {
            headers_.insert(make_pair(header_name, header_value));
        }
    }
}


string message::format_address_list(const mailboxes& mailbox_list) const
{
    const regex ATEXT_REGEX{R"([a-zA-Z0-9\!#\$%&'\*\+\-\./=\?\^\_`\{\|\}\~]*)"};
    smatch m;
    string mailbox_str;

    for (auto ma = mailbox_list.addresses.begin(); ma != mailbox_list.addresses.end(); ma++)
    {
        if (mailbox_list.addresses.size() > 1 && ma != mailbox_list.addresses.begin())
            mailbox_str += NEW_LINE_INDENT + format_address(ma->name, ma->address);
        else
            mailbox_str += format_address(ma->name, ma->address);

        if (ma != mailbox_list.addresses.end() - 1)
            mailbox_str += ADDRESS_SEPARATOR + codec::END_OF_LINE;
    }

    if (!mailbox_list.groups.empty() && !mailbox_list.addresses.empty())
        mailbox_str += ADDRESS_SEPARATOR + codec::END_OF_LINE + NEW_LINE_INDENT;

    for (auto mg = mailbox_list.groups.begin(); mg != mailbox_list.groups.end(); mg++)
    {
        if (!regex_match(mg->name, m, ATEXT_REGEX))
            throw message_error("Formatting failure of address list, bad group name `" + mg->name + "`.");

        mailbox_str += mg->name + MAILGROUP_NAME_SEPARATOR + codec::SPACE_CHAR;
        for (auto ma = mg->members.begin(); ma != mg->members.end(); ma++)
        {
            if (mg->members.size() > 1 && ma != mg->members.begin())
                mailbox_str += NEW_LINE_INDENT + format_address(ma->name, ma->address);
            else
                mailbox_str += format_address(ma->name, ma->address);

            if (ma != mg->members.end() - 1)
                mailbox_str += ADDRESS_SEPARATOR + codec::END_OF_LINE;
        }
        mailbox_str += mg != mailbox_list.groups.end() - 1 ? string(1, MAILGROUP_SEPARATOR) + codec::END_OF_LINE + NEW_LINE_INDENT : string(1, MAILGROUP_SEPARATOR);
    }

    return mailbox_str;
}


string message::format_address(const string_t& name, const string& address) const
{
    if (name.buffer.empty() && address.empty())
        return "";

    // TODO: no need for regex, simple string comparaison can be used
    const regex QTEXT_REGEX{R"([a-zA-Z0-9\ \t\!#\$%&'\(\)\*\+\,\-\.@/\:;<=>\?\[\]\^\_`\{\|\}\~]*)"};
    const regex DTEXT_REGEX{R"([a-zA-Z0-9\!#\$%&'\*\+\-\.\@/=\?\^\_`\{\|\}\~]*)"};

    string name_formatted;
    string addr;
    smatch m;

    // The charset has precedence over the header codec. Only for the non-ascii characters, consider the header encoding.

    if (name.charset == codec::CHARSET_ASCII)
    {
        // Check the name format.

        if (regex_match(name.buffer, m, regex(R"([A-Za-z0-9\ \t]*)")))
            name_formatted = name.buffer;
        else if (regex_match(name.buffer, m, QTEXT_REGEX))
            name_formatted = codec::QUOTE_CHAR + name.buffer + codec::QUOTE_CHAR;
        else
            throw message_error("Formatting failure of name `" + name.buffer + "`.");
    }
    else if (header_codec_ == header_codec_t::UTF8)
    {
        name_formatted = name.buffer;
    }
    else
    {
        q_codec qc(line_policy_, decoder_line_policy_);
        vector<string> enc_name = qc.encode(name.buffer, name.charset, header_codec_);
        for (auto sit = enc_name.begin(); sit != enc_name.end(); sit++)
        {
            name_formatted += *sit;
            if (sit != enc_name.end() - 1)
                name_formatted += codec::SPACE_STR;
        }
    }

    // Check address format.

    if (!address.empty())
    {
        if (codec::is_utf8_string(address))
        {
            addr = ADDRESS_BEGIN_CHAR + address + ADDRESS_END_CHAR;
        }
        else
        {
            if (regex_match(address, m, DTEXT_REGEX))
                addr = ADDRESS_BEGIN_CHAR + address + ADDRESS_END_CHAR;
            else
                throw message_error("Formatting failure of address `" + address + "`.");
        }
    }

    string addr_name = (name_formatted.empty() ? addr : name_formatted + (addr.empty() ? "" : codec::SPACE_STR + addr));
    return fold_header_line(addr_name);
}


/*
See [rfc 5322, section 3.4, page 16-18].

Implementation goes by using state machine. Diagram is shown in graphviz dot language:
```
digraph address_list
{
    rankdir=LR;
    node [shape = box];
    begin -> begin [label = "space"];
    begin -> nameaddrgrp [label = "atext"];
    begin -> qnameaddrbeg [label = "quote"];
    begin -> addrbrbeg [label="left_bracket"];
    nameaddrgrp -> nameaddrgrp [label = "atext"];
    nameaddrgrp -> name [label = "space"];
    nameaddrgrp -> addr [label = "monkey"];
    nameaddrgrp -> groupbeg [label = "colon"];
    nameaddrgrp -> addrbrbeg [label = "left_bracket"];
    nameaddrgrp -> begin [label = "comma"];
    nameaddrgrp -> qnameaddrbeg [label = "quote" style="dashed"];
    name -> name [label = "atext, space"];
    name -> addrbrbeg [label = "left_bracket"];
    name -> qnameaddrbeg [label = "quote" style="dashed"];
    addr -> addr [label = "atext"];
    addr -> begin [label = "comma"];
    addr -> groupend [label = "semicolon"];
    addr -> addrbrbeg [label="monkey" style="dashed"]
    addr -> commbeg [label = "left_parenthesis"];
    addr -> end [label = "eol"];
    qnameaddrbeg -> qnameaddrbeg [label = "qtext"];
    qnameaddrbeg -> qnameaddrend [label = "quote"];
    qnameaddrend -> qnameaddrend [label = "space"];
    qnameaddrend -> addrbrbeg [label = "left_bracket"];
    addrbrbeg -> addrbrbeg [label = "dtext"];
    addrbrbeg -> addrbrend [label = "right_bracket"];
    addrbrend -> begin [label = "comma"];
    addrbrend -> addrbrend [label = "space"];
    addrbrend -> groupend [label = "semicolon"];
    addrbrend -> commbeg [label = "left_parenthesis"];
    addrbrend -> end [label = "eol"];
    groupbeg -> begin [label = "atext"];
    groupbeg -> groupend [label = "semicolon"];
    groupbeg -> addrbrbeg [label = "left_bracket"];
    groupend -> begin [label = "atext"];
    groupend -> commbeg [label = "left_parenthesis"];
    groupend -> end [label = "eol"];
    commbeg -> commbeg [label = "atext"];
    commbeg -> commend [label = "right_parenthesis"];
    commend -> commend [label = "space"];
    commend -> end [label = "eol"];
}
```
Meanings of the labels:
- nameaddrgrp: begin of a name or address or group without qoutes
- name: a name without address
- addr: an address only
- qnameaddrbeg: begin of a quoted name
- qnameaddrend: end of a quoted name
- addrbrbeg: begin of an address in angle brackets
- addrbrend: end of an address in angle brackets
- groupbeg: begin of a group
- groupend: end of a group
- commbeg: begin of a comment
- commend: end of a comment
*/
mailboxes message::parse_address_list(const string& address_list)
{
    enum class state_t {BEGIN, NAMEADDRGRP, QNAMEADDRBEG, ADDR, NAME, QNAMEADDREND, ADDRBRBEG, ADDRBREND, GROUPBEG, GROUPEND, COMMBEG, COMMEND, EOL};

    vector<mail_address> mail_list;
    vector<mail_group> mail_group_list;
    mail_address cur_address;
    mail_group cur_group;
    // temporary mail list containing recipients or group members
    vector<mail_address> mail_addrs;
    state_t state = state_t::BEGIN;
    // flag if monkey char is found in the address part
    bool monkey_found = false;
    // flag if mailing group is being parsed, used to determine if addresses are part of a group or not
    bool group_found = false;
    // string being parsed so far
    string token;

    for (auto ch = address_list.begin(); ch != address_list.end(); ch++)
    {
        switch (state)
        {
            case state_t::BEGIN:
            {
                if (isspace(*ch))
                    ;
                else if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || codec::is_8bit_char(*ch))
                {
                    token += *ch;
                    state = state_t::NAMEADDRGRP;
                }
                else if (*ch == codec::QUOTE_CHAR)
                    state = state_t::QNAMEADDRBEG;
                else if (*ch == ADDRESS_BEGIN_CHAR)
                    state = state_t::ADDRBRBEG;
                else
                    throw message_error("Parsing failure of address or group at `" + string(1, *ch) + "`.");

                if (ch == address_list.end() - 1)
                {
                    if (state == state_t::BEGIN)
                        ;
                    // one character only, so it's the name part of the address
                    else if (state == state_t::NAMEADDRGRP)
                    {
                        if (group_found)
                            throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");
                        else
                        {
                            if (!token.empty())
                            {
                                cur_address.name = token;
                                trim(cur_address.name.buffer);
                                mail_list.push_back(cur_address);
                            }
                        }
                    }
                    // `QNAMEADDRBEG` or `ADDRBRBEG`
                    else
                        throw message_error("Parsing failure of name or address at `" + string(1, *ch) + "`.");
                }

                break;
            }

            case state_t::NAMEADDRGRP:
            {
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || codec::is_8bit_char(*ch))
                    token += *ch;
                else if (*ch == codec::MONKEY_CHAR)
                {
                    token += *ch;
                    state = state_t::ADDR;
                    monkey_found = true;
                }
                else if (*ch == codec::QUOTE_CHAR && !strict_mode_)
                    state = state_t::QNAMEADDRBEG;
                else if (isspace(*ch))
                {
                    token += *ch;
                    state = state_t::NAME;
                }
                else if (*ch == ADDRESS_SEPARATOR)
                {
                    cur_address.name = token;
                    trim(cur_address.name.buffer);
                    token.clear();
                    mail_addrs.push_back(cur_address);
                    cur_address.clear();
                    monkey_found = false;
                    state = state_t::BEGIN;
                }
                else if (*ch == MAILGROUP_NAME_SEPARATOR)
                {
                    if (group_found)
                        throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");

                    // if group is reached, store already found addresses in the list
                    mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                    mail_addrs.clear();
                    cur_group.name = token;
                    token.clear();
                    group_found = true;
                    state = state_t::GROUPBEG;
                }
                else if (*ch == ADDRESS_BEGIN_CHAR)
                {
                    cur_address.name = token;
                    trim(cur_address.name.buffer);
                    cur_address.name = parse_address_name(cur_address.name.buffer);
                    token.clear();
                    state = state_t::ADDRBRBEG;
                }
                else
                    throw message_error("Parsing failure of address or group at `" + string(1, *ch) + "`.");

                if (ch == address_list.end() - 1)
                {
                    if (state == state_t::NAMEADDRGRP)
                    {
                        if (group_found)
                            throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");

                        if (!token.empty())
                        {
                            cur_address.name = token;
                            mail_addrs.push_back(cur_address);
                            mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                        }
                    }
                    else if (state == state_t::ADDR)
                        throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");
                    else if (state == state_t::NAME)
                        throw message_error("Parsing failure of name at `" + string(1, *ch) + "` .");
                    else if (state == state_t::BEGIN)
                    {
                        if (group_found)
                            throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");

                        mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                    }
                    else if (state == state_t::GROUPBEG)
                        throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");
                }

                break;
            }

            case state_t::NAME:
            {
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || isspace(*ch) || codec::is_8bit_char(*ch))
                    token += *ch;
                else if (*ch == codec::QUOTE_CHAR && !strict_mode_)
                    state = state_t::QNAMEADDRBEG;
                else if (*ch == ADDRESS_BEGIN_CHAR)
                {
                    cur_address.name = token;
                    trim(cur_address.name.buffer);
                    cur_address.name = parse_address_name(cur_address.name.buffer);
                    token.clear();
                    state = state_t::ADDRBRBEG;
                }
                else
                    throw message_error("Parsing failure of name at `" + string(1, *ch) + "`.");

                // not allowed to end address list in this state
                if (ch == address_list.end() - 1)
                    throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");

                break;
            }

            case state_t::ADDR:
            {
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || codec::is_8bit_char(*ch))
                    token += *ch;
                else if (*ch == codec::MONKEY_CHAR)
                {
                    token += *ch;
                    monkey_found = true;
                }
                else if (*ch == ADDRESS_BEGIN_CHAR && !strict_mode_)
                {
                    cur_address.name = token;
                    trim(cur_address.name.buffer);
                    token.clear();
                    state = state_t::ADDRBRBEG;
                }
                // TODO: space is allowed in the address?
                else if (isspace(*ch))
                    ;
                else if (*ch == ADDRESS_SEPARATOR)
                {
                    cur_address.address = token;
                    token.clear();
                    mail_addrs.push_back(cur_address);
                    cur_address.clear();
                    if (!monkey_found)
                        throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");
                    monkey_found = false;
                    state = state_t::BEGIN;
                }
                else if (*ch == MAILGROUP_SEPARATOR)
                {
                    if (group_found)
                    {
                        cur_address.address = token;
                        token.clear();
                        mail_addrs.push_back(cur_address);
                        cur_address.clear();
                        cur_group.add(mail_addrs);
                        mail_addrs.clear();
                        mail_group_list.push_back(cur_group);
                        cur_group.clear();
                        group_found = false;
                        state = state_t::GROUPEND;
                    }
                    else
                        throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");
                }
                else if (*ch == codec::LEFT_PARENTHESIS_CHAR)
                {
                    if (group_found)
                        throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");
                    else
                    {
                        cur_address.address = token;
                        token.clear();
                        mail_addrs.push_back(cur_address);
                        cur_address.clear();
                        if (!monkey_found)
                            throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");
                        mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                    }
                    state = state_t::COMMBEG;
                }
                else
                    throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");

                if (ch == address_list.end() - 1)
                {
                    if (state == state_t::ADDR)
                    {
                        if (group_found)
                            throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");
                        if (!monkey_found)
                            throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");

                        if (!token.empty())
                        {
                            cur_address.address = token;
                            mail_addrs.push_back(cur_address);
                            mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                        }
                    }
                    else if (state == state_t::BEGIN)
                    {
                        if (group_found)
                            throw message_error("Parsing failure of address or group at `" + string(1, *ch) + "`.");

                        mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                    }
                    else if (state == state_t::GROUPEND)
                        ;
                    else if (state == state_t::COMMBEG)
                        throw message_error("Parsing failure of comment at `" + string(1, *ch) + "`.");
                }

                break;
            }

            case state_t::QNAMEADDRBEG:
            {
                if (isalpha(*ch) || isdigit(*ch) || isspace(*ch) || QTEXT.find(*ch) != string::npos || codec::is_8bit_char(*ch))
                    token += *ch;
                // backslash is invisible, see [rfc 5322, section 3.2.4]
                else if (*ch == codec::BACKSLASH_CHAR)
                    ;
                else if (*ch == codec::QUOTE_CHAR)
                {
                    cur_address.name = token;
                    cur_address.name = parse_address_name(cur_address.name.buffer);
                    token.clear();
                    state = state_t::QNAMEADDREND;
                }
                else
                    throw message_error("Parsing failure of name or address at `" + string(1, *ch) + "`.");

                // not allowed to end address list in this state in the strict mode
                if (ch == address_list.end() - 1)
                {
                    if (strict_mode_)
                        throw message_error("Parsing failure of name or address at `" + string(1, *ch) + "`.");
                    else
                        mail_list.push_back(cur_address);
                }

                break;
            }

            case state_t::QNAMEADDREND:
            {
               if (isspace(*ch))
                   ;
               else if (*ch == ADDRESS_BEGIN_CHAR)
                   state = state_t::ADDRBRBEG;
               else
                   throw message_error("Parsing failure of name or address at `" + string(1, *ch) + "`.");

               // not allowed to end address list in this state
               if (ch == address_list.end() - 1)
                   throw message_error("Parsing failure of name or address at `" + string(1, *ch) + "`.");

               break;
            }

            case state_t::ADDRBRBEG:
            {
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || codec::is_8bit_char(*ch))
                    token += *ch;
                else if (*ch == codec::MONKEY_CHAR)
                {
                    token += *ch;
                    monkey_found = true;
                }
                else if (*ch == ADDRESS_END_CHAR)
                {
                    cur_address.address = token;
                    token.clear();
                    mail_addrs.push_back(cur_address);
                    cur_address.clear();
                    if (!monkey_found)
                        throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");
                    monkey_found = false;
                    state = state_t::ADDRBREND;
                }
                else
                    throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");

                // not allowed to end address list in this state
                if (ch == address_list.end() - 1)
                {
                    if (state == state_t::ADDRBRBEG)
                        throw message_error("Parsing failure of address at `" + string(1, *ch) + "`.");
                    else if (state == state_t::ADDRBREND)
                    {
                        if (group_found)
                        {
                            cur_group.add(mail_addrs);
                            mail_group_list.push_back(cur_group);
                        }
                        else
                            mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                    }

                }

                break;
            }

            case state_t::ADDRBREND:
            {
                if (isspace(*ch))
                    ;
                else if (*ch == ADDRESS_SEPARATOR)
                    state = state_t::BEGIN;
                else if (*ch == MAILGROUP_SEPARATOR)
                {
                    if (group_found)
                    {
                        cur_group.add(mail_addrs);
                        mail_addrs.clear();
                        group_found = false;
                        mail_group_list.push_back(cur_group);
                        cur_group.clear();
                        group_found = false;
                        state = state_t::GROUPEND;
                    }
                    else
                        throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");
                }
                else if (*ch == codec::LEFT_PARENTHESIS_CHAR)
                {
                    if (group_found)
                        throw message_error("Parsing failure of comment at `" + string(1, *ch) + "`.");
                    else
                        mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                    state = state_t::COMMBEG;
                }

                if (ch == address_list.end() - 1)
                {
                    if (state == state_t::ADDRBREND || state == state_t::BEGIN)
                    {
                        if (group_found)
                            throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");

                        mail_list.insert(mail_list.end(), mail_addrs.begin(), mail_addrs.end());
                    }
                    else if (state == state_t::COMMBEG)
                        throw message_error("Parsing failure of comment at `" + string(1, *ch) + "`.");
                }

                break;
            }

            case state_t::GROUPBEG:
            {
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || codec::is_8bit_char(*ch))
                {
                    token += *ch;
                    state = state_t::BEGIN;
                }
                else if (isspace(*ch))
                    ;
                else if (*ch == ADDRESS_BEGIN_CHAR)
                {
                    state = state_t::ADDRBRBEG;
                }
                else if (*ch == MAILGROUP_SEPARATOR)
                {
                    cur_group.add(mail_addrs);
                    mail_addrs.clear();
                    mail_group_list.push_back(cur_group);
                    cur_group.clear();
                    group_found = false;
                    state = state_t::GROUPEND;
                }

                if (ch == address_list.end() - 1)
                {
                    if (state == state_t::BEGIN || state == state_t::ADDRBRBEG)
                        throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");
                }

                break;
            }

            case state_t::GROUPEND:
            {
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || codec::is_8bit_char(*ch))
                {
                    token += *ch;
                    state = state_t::BEGIN;
                }
                else if (*ch == codec::LEFT_PARENTHESIS_CHAR)
                {
                    state = state_t::COMMBEG;
                }
                else if (isspace(*ch))
                {
                    ;
                }

                if (ch == address_list.end() - 1)
                {
                    if (state == state_t::BEGIN || state == state_t::COMMBEG)
                        throw message_error("Parsing failure of group at `" + string(1, *ch) + "`.");
                }

                break;
            }

            case state_t::COMMBEG:
            {
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || isspace(*ch))
                    ;
                else if (*ch == codec::RIGHT_PARENTHESIS_CHAR)
                    state = state_t::COMMEND;
                else
                    throw message_error("Parsing failure of comment at `" + string(1, *ch) + "`.");
                break;
            }

            case state_t::COMMEND:
            {
                if (isspace(*ch))
                    ;
                else
                    throw message_error("Parsing failure of comment at `" + string(1, *ch) + "`.");
                break;
            }

            // TODO: check if this case is ever reached
            case state_t::EOL:
            {
                mail_addrs.push_back(cur_address);
                break;
            }
        }
    }

    return mailboxes(mail_list, mail_group_list);
}


/*
See [rfc 5322, section 3.3, page 14-16].
*/
local_date_time message::parse_date(const string& date_str) const
{
    try
    {
        // date format to be parsed is like "Thu, 17 Jul 2014 10:31:49 +0200 (CET)"
        regex r(R"(([A-Za-z]{3}[\ \t]*,)[\ \t]+(\d{1,2}[\ \t]+[A-Za-z]{3}[\ \t]+\d{4})[\ \t]+(\d{2}:\d{2}:\d{2}[\ \t]+(\+|\-)\d{4}).*)");
        smatch m;
        if (regex_match(date_str, m, r))
        {
            // TODO: regex manipulation to be replaced with time facet format?

            // if day has single digit, then prepend it with zero
            string dttz = m[1].str() + " " + (m[2].str()[1] == ' ' ? "0" : "") + m[2].str() + " " + m[3].str().substr(0, 12) + ":" + m[3].str().substr(12);
            stringstream ss(dttz);
            local_time_input_facet* facet = new local_time_input_facet("%a %d %b %Y %H:%M:%S %ZP");
            ss.exceptions(std::ios_base::failbit);
            ss.imbue(locale(ss.getloc(), facet));
            local_date_time ldt(not_a_date_time);
            ss >> ldt;
            return ldt;
        }
        return local_date_time(not_a_date_time);
    }
    catch (...)
    {
        throw message_error("Parsing failure of date.");
    }
}


string message::fold_header_line(const string& header_line) const
{
    const string DELIMITERS = " ,;";
    string folded_line;
    string::size_type pos = 0, pos_len = 0;

    do
    {
        string line = header_line.substr(pos, string::size_type(line_policy_));
        if (line.length() < string::size_type(line_policy_))
        {
            folded_line += line;
            break;
        }

        pos_len = line.find_last_of(DELIMITERS);
        if (pos_len == string::npos)
        {
            if (strict_mode_)
                throw message_error("Header folding failure.");
            folded_line += header_line.substr(pos, string::npos);
        }
        else
            folded_line += header_line.substr(pos, pos_len) + codec::END_OF_LINE + codec::SPACE_STR + codec::SPACE_STR;
        pos += pos_len + 1;
    }
    while (pos_len != string::npos);

    return folded_line;
}


string_t message::format_subject() const
{
    string_t subject;

    if (subject_.charset != codec::CHARSET_ASCII && header_codec_ != header_codec_t::UTF8)
    {
        q_codec qc(line_policy_, decoder_line_policy_);
        vector<string> hdr = qc.encode(subject_.buffer, subject_.charset, header_codec_);
        subject.buffer += hdr.at(0) + codec::END_OF_LINE;
        if (hdr.size() > 1)
            for (auto h = hdr.begin() + 1; h != hdr.end(); h++)
                subject.buffer += codec::SPACE_STR + *h + codec::END_OF_LINE;
    }
    else
        subject.buffer += fold_header_line(subject_.buffer) + codec::END_OF_LINE;

    return subject;
}


tuple<string, string> message::parse_subject(const string& subject)
{
    if (codec::is_utf8_string(subject))
        return make_tuple(subject, codec::CHARSET_UTF8);
    else
    {
        q_codec qc(line_policy_, decoder_line_policy_);
        auto subject_dec = qc.check_decode(subject);
        header_codec_t subject_encoding = get<2>(subject_dec);
        if (subject_encoding != header_codec_t::UTF8)
            header_codec_ = subject_encoding;
        return make_tuple(get<0>(subject_dec), get<1>(subject_dec));
    }
}


string_t message::parse_address_name(const string& address_name)
{
    q_codec qc(line_policy_, decoder_line_policy_);
    const string::size_type Q_CODEC_SEPARATORS_NO = 4;
    string::size_type addr_len = address_name.size();
    bool is_q_encoded = address_name.size() >= Q_CODEC_SEPARATORS_NO && address_name.at(0) == codec::EQUAL_CHAR &&
        address_name.at(1) == codec::QUESTION_MARK_CHAR && address_name.at(addr_len - 1) == codec::EQUAL_CHAR &&
        address_name.at(addr_len - 2) == codec::QUESTION_MARK_CHAR;

    // TODO: What if the address name starts with `?=` but does not end with `=?` Is it an error or just a raw string?

    if (is_q_encoded)
    {
        auto parts = split_qc_string(address_name);
        string parts_str, charset;
        for (const auto& p : parts)
        {
            string::size_type p_len = p.length();
            auto an = qc.decode(p.substr(0, p_len - 2));
            parts_str += get<0>(an);
            if (charset.empty())
                charset = get<1>(an);
            if (charset != get<1>(an))
                throw message_error("Inconsistent Q encodings.");
            header_codec_ = get<2>(an);
        }
        return string_t(parts_str, charset);
    }

    if (codec::is_utf8_string(address_name))
        return string_t(address_name, codec::CHARSET_UTF8);
    else
        return string_t(address_name, codec::CHARSET_ASCII);
}


vector<string> message::split_qc_string(const string& text)
{
    const string Q_ENCODING_BEGIN = "?=";
    const string Q_ENCODING_END = "=?";
    vector<string> parts;
    string::size_type begin_pos = text.find(Q_ENCODING_END);
    while (begin_pos != string::npos)
    {
        string::size_type charset_pos = text.find(codec::QUESTION_MARK_CHAR, begin_pos + 1);
        string::size_type method_pos = text.find(codec::QUESTION_MARK_CHAR, charset_pos + 1);
        string::size_type content_begin = text.find(codec::QUESTION_MARK_CHAR, method_pos + 1);
        string::size_type content_end = text.find(Q_ENCODING_BEGIN, content_begin + 1);
        parts.push_back(text.substr(begin_pos, content_end + 2 - begin_pos));
        begin_pos = text.find(Q_ENCODING_END, content_end + 2);
    }
    return parts;
}


} // namespace mailio
