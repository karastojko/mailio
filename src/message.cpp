/*

message.cpp
-----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/ 


#include <string>
#include <vector>
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
#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <codec.hpp>
#include <base64.hpp>
#include <quoted_printable.hpp>
#include <bit7.hpp>
#include <bit8.hpp>
#include <q_codec.hpp>
#include <mime.hpp>
#include <message.hpp>


using std::string;
using std::vector;
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
using std::size_t;
using boost::trim_copy;
using boost::trim;
using boost::iequals;
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
const string message::REPLY_TO_HEADER{"Reply-To"};
const string message::TO_HEADER{"To"};
const string message::CC_HEADER{"Cc"};
const string message::BCC_HEADER{"Bcc"};
const string message::SUBJECT_HEADER{"Subject"};
const string message::DATE_HEADER{"Date"};
const string message::MIME_VERSION_HEADER{"MIME-Version"};


message::message() : mime()
{
    time_zone_ptr tz(new posix_time_zone("00:00"));
    _date_time = make_shared<local_date_time>(second_clock::local_time(), tz);
}


void message::format(string& message_str, bool dot_escape) const
{
    message_str += format_header();

    if (!_parts.empty())
    {
        if (!_content.empty())
        {
            mime content_part;
            content_part.content(_content);
            content_type_t ct(media_type_t::TEXT, "plain");
            ct.charset = _content_type.charset;
            content_part.content_type(ct);
            content_part.content_transfer_encoding(_encoding);
            content_part.line_policy(_line_policy);
            content_part.strict_mode(_strict_mode);
            content_part.strict_codec_mode(_strict_codec_mode);
            content_part.header_codec(_header_codec);
            string cps;
            content_part.format(cps, dot_escape);
            message_str += "--" + _boundary + codec::CRLF + cps + codec::CRLF;
        }

        // recursively format mime parts
        for (const auto& p: _parts)
        {
            string p_str;
            p.format(p_str, dot_escape);
            message_str += "--" + _boundary + codec::CRLF + p_str + codec::CRLF;
        }
        message_str += "--" + _boundary + "--" + codec::CRLF;
    }
    else
        message_str += format_content(dot_escape);
}


bool message::empty() const
{
    return _content.empty();
}


void message::sender(const mail_address& mail)
{
    _sender = mail;
}


mail_address message::sender() const
{
    return _sender;
}


string message::sender_to_string() const
{
    return format_address(_sender.name, _sender.address);
}


void message::reply_address(const mail_address& mail)
{
    _reply_address = mail;
}


mail_address message::reply_address() const
{
    return _reply_address;
}


string message::reply_address_to_string() const
{
    return format_address(_reply_address.name, _reply_address.address);
}


void message::add_recipient(const mail_address& mail)
{
    _recipients.addresses.push_back(mail);
}


void message::add_recipient(const mail_group& group)
{
    _recipients.groups.push_back(group);
}


mailboxes message::recipients() const
{
    return _recipients;
}


string message::recipients_to_string() const
{
    return format_address_list(_recipients);
}


void message::add_cc_recipient(const mail_address& mail)
{
    _cc_recipients.addresses.push_back(mail);
}


void message::add_cc_recipient(const mail_group& group)
{
    _cc_recipients.groups.push_back(group);
}


mailboxes message::cc_recipients() const
{
    return _cc_recipients;
}


string message::cc_recipients_to_string() const
{
    return format_address_list(_cc_recipients);
}


void message::add_bcc_recipient(const mail_address& mail)
{
    _bcc_recipients.addresses.push_back(mail);
}


void message::add_bcc_recipient(const mail_group& mail)
{
    _bcc_recipients.groups.push_back(mail);
}


mailboxes message::bcc_recipients() const
{
    return _bcc_recipients;
}


string message::bcc_recipients_to_string() const
{
    return format_address_list(_bcc_recipients);
}


void message::subject(const string& mail_subject)
{
    _subject = mail_subject;
}


string message::subject() const
{
    return _subject;
}


local_date_time message::date_time() const
{
    return *_date_time;
}


void message::date_time(const boost::local_time::local_date_time& mail_dt)
{
    *_date_time = mail_dt;
}


void message::attach(const istream& att_strm, const string& att_name, media_type_t type, const string& subtype)
{
    if (_boundary.empty())
        _boundary = make_boundary();
    _content_type.type = media_type_t::MULTIPART;
    _content_type.subtype = "mixed";

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
    _parts.push_back(m);
}


size_t message::attachments_size() const
{
    size_t no = 0;
    for (auto& m : _parts)
        if (m.content_disposition() == content_disposition_t::ATTACHMENT)
            no++;
    return no;
}


void message::attachment(size_t index, ostream& att_strm, string& att_name) const
{
    if (index == 0)
        throw message_error("Bad attachment index.");
    
    size_t no = 0;
    for (auto& m : _parts)
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
    
    if (no > _parts.size())
        throw message_error("Bad attachment index.");
}


string message::format_header() const
{
    string header;
    
    if (!_boundary.empty() && _content_type.type != media_type_t::MULTIPART)
        throw message_error("Formatting failure of non multipart message with boundary.");

    header += FROM_HEADER + COLON + sender_to_string() + codec::CRLF;
    header += _reply_address.name.empty() ? "" : REPLY_TO_HEADER + COLON + reply_address_to_string() + codec::CRLF;
    header += TO_HEADER + COLON + recipients_to_string() + codec::CRLF;
    header += _cc_recipients.empty() ? "" : CC_HEADER + COLON + cc_recipients_to_string() + codec::CRLF;
    header += _bcc_recipients.empty() ? "" : BCC_HEADER + COLON + bcc_recipients_to_string() + codec::CRLF;

    // TODO: move formatting datetime to a separate method
    if (!_date_time->is_not_a_date_time())
    {
        stringstream ss;
        local_time_facet* facet = new local_time_facet("%a, %d %b %Y %H:%M:%S %q");
        ss.exceptions(std::ios_base::failbit);
        ss.imbue(locale(ss.getloc(), facet));
        ss << *_date_time;
        header += DATE_HEADER + COLON + ss.str() + codec::CRLF;
    }

    if (!_parts.empty())
        header += MIME_VERSION_HEADER + COLON + _version + codec::CRLF;
    header += mime::format_header();

    header += SUBJECT_HEADER + COLON + format_subject() + codec::CRLF;
    
    return header;
}


/*
TODO: parsing address list does not check the line policy
TODO: other headers should check for the line policy as well?

Some of the headers cannot be empty by RFC, but still they can occur. Thus, parser strict mode has to be introduced; in case it's false, default
values are set. The following headers are recognized by the parser:
- `From` cannot be empty by RFC 5322, section 3.6.2. So far, empty field did not occur, so no need to set default mode when empty.
- `Reply-To` is optional by RFC 5322, section 3.6.2. So far, empty field did not occur, so no need to set default mode when empty.
- `To` cannot be empty by RFC 5322, section 3.6.3. So far, empty field did not occur, so no need to set default mode when empty.
- `Cc` cannot be empty by RFC 5322, section 3.6.3. So far, empty field did not occur, so no need to set default mode when empty.
- `Subject` can be empty.
- `Date` can be empty.
- `MIME-Version` cannot be empty by RFC 2045, section 4. In case it's empty, set it to `1.0`.
*/
void message::parse_header_line(const string& header_line)
{
    mime::parse_header_line(header_line);

    if (header_line.length() > string::size_type(_line_policy))
        throw message_error("Parsing failure of header, line policy overflow.");

    // TODO: header name and header value already parsed in `mime::parse_header_line`, so this is not the optimal way to do it
    string header_name, header_value;
    parse_header_name_value(header_line, header_name, header_value);
    if (iequals(header_name, FROM_HEADER))
    {
        mailboxes mbx = parse_address_list(header_value);
        if (mbx.addresses.empty())
            throw message_error("Parsing failure of header, no sender.");
        _sender = mbx.addresses[0];
    }
    else if (iequals(header_name, REPLY_TO_HEADER))
    {
        mailboxes mbx = parse_address_list(header_value);
        if (!mbx.addresses.empty())
            _reply_address = mbx.addresses[0];
    }
    else if (iequals(header_name, TO_HEADER))
    {
        _recipients = parse_address_list(header_value);
    }
    else if (iequals(header_name, CC_HEADER))
    {
        _cc_recipients = parse_address_list(header_value);
    }
    else if (iequals(header_name, SUBJECT_HEADER))
        _subject = parse_subject(header_value);
    else if (iequals(header_name, DATE_HEADER))
        *_date_time = parse_date(trim_copy(header_value));
    else if (iequals(header_name, MIME_VERSION_HEADER))
        _version = trim_copy(header_value);
}


string message::format_address_list(const mailboxes& mailbox_list) const
{
    const regex ATEXT_REGEX{R"([a-zA-Z0-9\!#\$%&'\*\+\-\./=\?\^\_`\{\|\}\~]*)"};
    smatch m;
    string mailbox_str;

    for (auto ma = mailbox_list.addresses.begin(); ma != mailbox_list.addresses.end(); ma++)
    {
        if (mailbox_list.addresses.size() > 1 && ma != mailbox_list.addresses.begin())
            mailbox_str += "  " + format_address(ma->name, ma->address);
        else
            mailbox_str += format_address(ma->name, ma->address);

        if (ma != mailbox_list.addresses.end() - 1)
            mailbox_str += "," + codec::CRLF;
    }

    if (!mailbox_list.groups.empty() && !mailbox_list.addresses.empty())
        mailbox_str +=  "," + codec::CRLF + "  ";

    for (auto mg = mailbox_list.groups.begin(); mg != mailbox_list.groups.end(); mg++)
    {
        if (!regex_match(mg->name, m, ATEXT_REGEX))
            throw message_error("Formatting failure of address list, bad group name `" + mg->name + "`.");

        mailbox_str += mg->name + codec::COLON_CHAR + codec::SPACE_CHAR;
        for (auto ma = mg->members.begin(); ma != mg->members.end(); ma++)
        {
            if (mg->members.size() > 1 && ma != mg->members.begin())
                mailbox_str += "  " + format_address(ma->name, ma->address);
            else
                mailbox_str += format_address(ma->name, ma->address);

            if (ma != mg->members.end() - 1)
                mailbox_str += "," + codec::CRLF;
        }
        mailbox_str += mg != mailbox_list.groups.end() - 1 ? ";" + codec::CRLF + "  " : ";";
    }

    return mailbox_str;
}


string message::format_address(const string& name, const string& address) const
{
    if (name.empty() && address.empty())
        return "";

    // TODO: no need for regex, simple string comparaison can be used
    const regex QTEXT_REGEX{R"([a-zA-Z0-9\ \t\!#\$%&'\(\)\*\+\,\-\.@/\:;<=>\?\[\]\^\_`\{\|\}\~]*)"};
    const regex DTEXT_REGEX{R"([a-zA-Z0-9\!#\$%&'\*\+\-\.\@/=\?\^\_`\{\|\}\~]*)"};

    string name_formatted;
    string addr;
    smatch m;

    if (codec::is_utf8_string(name))
    {
        q_codec qc(_line_policy, _header_codec);
        vector<string> n = qc.encode(name);
        // mail has to be formatted into a single line, otherwise it's an error
        if (n.size() > 1)
            throw message_error("Formatting failure of name, line policy overflow.");
        name_formatted = n[0];
    }
    else
    {
        if (regex_match(name, m, regex(R"([A-Za-z0-9\ \t]*)")))
            name_formatted = name;
        else if (regex_match(name, m, QTEXT_REGEX))
            name_formatted = codec::QUOTE_CHAR + name + codec::QUOTE_CHAR;
        else
            throw message_error("Formatting failure of name `" + name + "`.");
    }

    if (!address.empty())
    {
        if (regex_match(address, m, DTEXT_REGEX))
            addr = codec::LESS_THAN_CHAR + address + codec::GREATER_THAN_CHAR;
        else
            throw message_error("Formatting failure of address `" + address + "`.");
    }
    
    string addr_name = (name_formatted.empty() ? addr : name_formatted + (addr.empty() ? "" : " " + addr));
    if (addr_name.length() > string::size_type(_line_policy))
        throw message_error("Formatting failure of address, line policy overflow.");
    return addr_name;
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
    nameaddrgrp -> begin [label = "comma"];
    name -> name [label = "atext, space"];
    name -> addrbrbeg [label = "left_bracket"];
    addr -> addr [label = "atext"];
    addr -> begin [label = "comma"];
    addr -> groupend [label = "semicolon"];
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
- comment: end of a comment
*/
mailboxes message::parse_address_list(const string& address_list) const
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
                else if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos)
                {
                    token += *ch;
                    state = state_t::NAMEADDRGRP;
                }
                else if (*ch == codec::QUOTE_CHAR)
                    state = state_t::QNAMEADDRBEG;
                else if (*ch == codec::LESS_THAN_CHAR)
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
                                trim(cur_address.name);
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
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos)
                    token += *ch;
                else if (*ch == codec::MONKEY_CHAR)
                {
                    token += *ch;
                    state = state_t::ADDR;
                    monkey_found = true;
                }
                else if (isspace(*ch))
                {
                    token += *ch;
                    state = state_t::NAME;
                }
                else if (*ch == codec::COMMA_CHAR)
                {
                    cur_address.name = token;
                    trim(cur_address.name);
                    token.clear();
                    mail_addrs.push_back(cur_address);
                    cur_address.clear();
                    monkey_found = false;
                    state = state_t::BEGIN;
                }
                else if (*ch == codec::COLON_CHAR)
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
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos || isspace(*ch))
                    token += *ch;
                else if (*ch == codec::LESS_THAN_CHAR)
                {
                    cur_address.name = token;
                    trim(cur_address.name);
                    cur_address.name = parse_address_name(cur_address.name);
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
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos)
                    token += *ch;
                else if (*ch == codec::MONKEY_CHAR)
                {
                    token += *ch;
                    monkey_found = true;
                }
                // TODO: space is allowed in the address?
                else if (isspace(*ch))
                    ;
                else if (*ch == codec::COMMA_CHAR)
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
                else if (*ch == codec::SEMICOLON_CHAR)
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
                if (isalpha(*ch) || isdigit(*ch) || isspace(*ch) || QTEXT.find(*ch) != string::npos)
                    token += *ch;
                // backslash is invisible, see [rfc 5322, section 3.2.4]
                else if (*ch == codec::BACKSLASH_CHAR)
                    ;
                else if (*ch == codec::QUOTE_CHAR)
                {
                    cur_address.name = token;
                    token.clear();
                    state = state_t::QNAMEADDREND;
                }
                else
                    throw message_error("Parsing failure of name or address at `" + string(1, *ch) + "`.");

                // not allowed to end address list in this state
                if (ch == address_list.end() - 1)
                    throw message_error("Parsing failure of name or address at `" + string(1, *ch) + "`.");

                break;
            }

            case state_t::QNAMEADDREND:
            {
               if (isspace(*ch))
                   ;
               else if (*ch == codec::LESS_THAN_CHAR)
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
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos)
                    token += *ch;
                else if (*ch == codec::MONKEY_CHAR)
                {
                    token += *ch;
                    monkey_found = true;
                }
                else if (*ch == codec::GREATER_THAN_CHAR)
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
                else if (*ch == codec::COMMA_CHAR)
                    state = state_t::BEGIN;
                else if (*ch == codec::SEMICOLON_CHAR)
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
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos)
                {
                    token += *ch;
                    state = state_t::BEGIN;
                }
                else if (isspace(*ch))
                    ;
                else if (*ch == codec::LESS_THAN_CHAR)
                {
                    state = state_t::ADDRBRBEG;
                }
                else if (*ch == codec::SEMICOLON_CHAR)
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
                if (isalpha(*ch) || isdigit(*ch) || ATEXT.find(*ch) != string::npos)
                {
                    token += *ch;
                    state = state_t::BEGIN;
                }
                else if (*ch == codec::LEFT_PARENTHESIS_CHAR)
                    state = state_t::COMMBEG;
                else if (isspace(*ch))
                    ;

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


string message::format_subject() const
{
    string subject;

    if (codec::is_utf8_string(_subject))
    {
        q_codec qc(codec::line_len_policy_t::RECOMMENDED, _header_codec);
        vector<string> hdr = qc.encode(_subject);
        subject += hdr.at(0) + codec::CRLF;
        if (hdr.size() > 1)
            for (auto h = hdr.begin() + 1; h != hdr.end(); h++)
                subject += " " + *h + codec::CRLF;
    }
    else
        subject += _subject + codec::CRLF;

    return subject;
}


string message::parse_subject(const string& subject) const
{
    q_codec qc(codec::line_len_policy_t::RECOMMENDED, _header_codec);
    return qc.check_decode(subject);
}


string message::parse_address_name(const string& address_name) const
{
    q_codec qc(_line_policy, _header_codec);
    const string::size_type Q_CODEC_SEPARATORS_NO = 4;
    string::size_type addr_len = address_name.size();
    if (address_name.size() >= Q_CODEC_SEPARATORS_NO && address_name.at(0) == codec::EQUAL_CHAR && address_name.at(1) == codec::QUESTION_MARK_CHAR &&
        address_name.at(addr_len - 1) == codec::EQUAL_CHAR && address_name.at(addr_len - 2) == codec::QUESTION_MARK_CHAR)
    {
        return qc.decode(address_name.substr(1, addr_len - 3));
    }

    return address_name;
}


} // namespace mailio
