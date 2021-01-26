/*

mime.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <random>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <mailio/base64.hpp>
#include <mailio/quoted_printable.hpp>
#include <mailio/bit7.hpp>
#include <mailio/bit8.hpp>
#include <mailio/binary.hpp>
#include <mailio/q_codec.hpp>
#include <mailio/mime.hpp>


using std::string;
using std::ifstream;
using std::stringstream;
using std::pair;
using std::multimap;
using std::vector;
using std::map;
using boost::regex;
using boost::regex_match;
using boost::smatch;
using boost::to_lower_copy;
using boost::trim_copy;
using boost::trim;
using boost::trim_right;
using boost::iequals;


namespace mailio
{


const string mime::content_type_t::ATTR_CHARSET{"charset"};
const string mime::content_type_t::ATTR_BOUNDARY{"boundary"};


mime::content_type_t::content_type_t() : type(media_type_t::NONE)
{
}


mime::content_type_t::content_type_t(media_type_t media_type, const string& media_subtype, const string& content_charset)
{
    type = media_type;
    subtype = to_lower_copy(media_subtype);
    charset = content_charset;
}


mime::content_type_t& mime::content_type_t::operator=(const mime::content_type_t& cont_type)
{
    if (this != &cont_type)
    {
        type = cont_type.type;
        subtype = to_lower_copy(cont_type.subtype);
        charset = cont_type.charset;
    }
    return *this;
}


const string mime::CONTENT_TYPE_HEADER{"Content-Type"};
const string mime::CONTENT_TRANSFER_ENCODING_HEADER{"Content-Transfer-Encoding"};
const string mime::CONTENT_TRANSFER_ENCODING_BASE64{"Base64"};
const string mime::CONTENT_TRANSFER_ENCODING_BIT7{"7bit"};
const string mime::CONTENT_TRANSFER_ENCODING_BIT8{"8bit"};
const string mime::CONTENT_TRANSFER_ENCODING_QUOTED_PRINTABLE{"Quoted-Printable"};
const string mime::CONTENT_TRANSFER_ENCODING_BINARY{"Binary"};
const string mime::CONTENT_DISPOSITION_HEADER{"Content-Disposition"};
const string mime::CONTENT_DISPOSITION_ATTACHMENT{"attachment"};
const string mime::CONTENT_DISPOSITION_INLINE{"inline"};
const string mime::NEW_LINE_INDENT{"  "};
const string mime::HEADER_SEPARATOR_STR{": "};
const string mime::NAME_VALUE_SEPARATOR_STR{"="};
const string mime::ATTRIBUTES_SEPARATOR_STR{"; "};
const string mime::ATTRIBUTE_NAME{"name"};
const string mime::ATTRIBUTE_FILENAME{"filename"};
const string mime::BOUNDARY_DELIMITER(2, '-');
const string mime::QTEXT{"\t !#$%&'()*+,-.@/:;<=>?[]^_`{|}~"};
// exluded double quote and backslash characters from the header name
const regex mime::HEADER_NAME_REGEX{R"(([a-zA-Z0-9\!#\$%&'\(\)\*\+\-\./;\<=\>\?@\[\\\]\^\_`\{\|\}\~]+))"};
const regex mime::HEADER_VALUE_REGEX{R"(([a-zA-Z0-9\ \!\"#\$%&'\(\)\*\+\,\-\./:;\<=\>\?@\[\\\]\^\_`\{\|\}\~]+))"};
const string mime::CONTENT_ATTR_ALPHABET{"!#$%&*+-.^_`|~"};
const string mime::CONTENT_HEADER_VALUE_ALPHABET{"!#$%&*+-./^_`|~"};


mime::mime() : _version("1.0"), _line_policy(codec::line_len_policy_t::RECOMMENDED),
    _decoder_line_policy(codec::line_len_policy_t::RECOMMENDED), _strict_mode(false), _strict_codec_mode(false),
    _header_codec(header_codec_t::QUOTED_PRINTABLE), _content_type(media_type_t::NONE, ""), _encoding(content_transfer_encoding_t::NONE),
    _disposition(content_disposition_t::NONE), _parsing_header(true), _mime_status(mime_parsing_status_t::NONE)
{
}


void mime::format(string& mime_str, bool dot_escape) const
{
    if (!_boundary.empty() && _content_type.type != media_type_t::MULTIPART)
        throw mime_error("Formatting failure, non multipart message with boundary.");

    mime_str += format_header() + codec::END_OF_LINE;
    string content = format_content(dot_escape);
    mime_str += content;

    if (!_parts.empty())
    {
        if (!content.empty())
            mime_str += codec::END_OF_LINE;
        // recursively format mime parts
        for (auto& p : _parts)
        {
            string p_str;
            p.format(p_str, dot_escape);
            mime_str += BOUNDARY_DELIMITER + _boundary + codec::END_OF_LINE + p_str + codec::END_OF_LINE;
        }
        mime_str += BOUNDARY_DELIMITER + _boundary + BOUNDARY_DELIMITER + codec::END_OF_LINE;
    }
}


void mime::parse(const string& mime_string, bool dot_escape)
{
    string::size_type line_begin = 0;
    string::size_type line_end = mime_string.find(codec::END_OF_LINE, line_begin);
    string line;
    while (line_end != string::npos)
    {
        line = mime_string.substr(line_begin, line_end - line_begin);
        parse_by_line(line, dot_escape);
        if (line_end != string::npos)
        {
            line_end += codec::END_OF_LINE.length();
            line_begin = line_end;
        }
        line_end = mime_string.find(codec::END_OF_LINE, line_begin);
    }
    line = mime_string.substr(line_begin);
    if (!line.empty())
        parse_by_line(line, dot_escape);
    parse_by_line(codec::END_OF_LINE, dot_escape);
}


mime& mime::parse_by_line(const string& line, bool dot_escape)
{
    // mark end of header and parse it
    if (_parsing_header && line.empty())
    {
        _parsing_header = false;
        parse_header();
    }
    else
    {
        if (_parsing_header)
            _parsed_headers.push_back(line);
        else
        {
            // end of message reached, decode the body
            if (line == codec::END_OF_LINE)
            {
                parse_content();
                _mime_status = mime_parsing_status_t::END;
            }
            // parsing the body
            else
            {
                // mime part sequence begins
                if (line == BOUNDARY_DELIMITER + _boundary && !_boundary.empty())
                {
                    _mime_status = mime_parsing_status_t::BEGIN;
                    // begin of another mime part means that the current part (if exists) is ended and parsed; another part is created
                    if (!_parts.empty())
                        _parts.back().parse_by_line(codec::END_OF_LINE);
                    mime m;
                    m.line_policy(_line_policy, _decoder_line_policy);
                    m.strict_codec_mode(_strict_codec_mode);
                    _parts.push_back(m);
                }
                // mime part sequence ends, so parse the last mime part
                else if (line == BOUNDARY_DELIMITER + _boundary + BOUNDARY_DELIMITER && !_boundary.empty())
                {
                    _mime_status = mime_parsing_status_t::END;
                    _parts.back().parse_by_line(codec::END_OF_LINE);
                }
                // mime content being parsed
                else
                {
                    // parser entered mime body
                    if (_mime_status == mime_parsing_status_t::BEGIN)
                        _parts.back().parse_by_line(line, dot_escape);
                    // put the line into `_parsed_body` until the whole body is read for parsing
                    else
                    {
                        if (dot_escape && line[0] == codec::DOT_CHAR)
                            _parsed_body.push_back(line.substr(1));
                        else
                            _parsed_body.push_back(line);
                    }
                }
            }
        }
    }

    return *this;
}


void mime::content_type(const content_type_t& cont_type)
{
    if (cont_type.type != media_type_t::NONE && cont_type.subtype.empty())
        throw mime_error("Bad content type.");
    _content_type = cont_type;
}


void mime::content_type(media_type_t media_type, const string& media_subtype, const string& charset)
{
    content_type_t c(media_type, media_subtype);
    c.charset = to_lower_copy(charset);
    content_type(c);
}


mime::content_type_t mime::content_type() const
{
    return _content_type;
}


void mime::name(const string& mime_name)
{
    _name = mime_name;
}


string mime::name() const
{
    return _name;
}


void mime::content_transfer_encoding(content_transfer_encoding_t encoding)
{
    _encoding = encoding;
}


mime::content_transfer_encoding_t mime::content_transfer_encoding() const
{
    return _encoding;
}


void mime::content_disposition(content_disposition_t disposition)
{
    _disposition = disposition;
}


mime::content_disposition_t mime::content_disposition() const
{
    return _disposition;
}


void mime::boundary(const string& bound)
{
    _boundary = bound;
}


string mime::boundary() const
{
    return _boundary;
}


void mime::content(const string& content_str)
{
    _content = content_str;
}


string mime::content() const
{
    return _content;
}


void mime::add_part(const mime& part)
{
    _parts.push_back(part);
}


vector<mime> mime::parts() const
{
    return _parts;
}


void mime::line_policy(codec::line_len_policy_t encoder_line_policy, codec::line_len_policy_t decoder_line_policy)
{
    _line_policy = encoder_line_policy;
    _decoder_line_policy = decoder_line_policy;
}


codec::line_len_policy_t mime::line_policy() const
{
    return _line_policy;
}


codec::line_len_policy_t mime::decoder_line_policy() const
{
    return _decoder_line_policy;
}


void mime::strict_mode(bool mode)
{
    _strict_mode = mode;
}


bool mime::strict_mode() const
{
    return _strict_mode;
}


void mime::strict_codec_mode(bool mode)
{
    _strict_codec_mode = mode;
}


bool mime::strict_codec_mode() const
{
    return _strict_codec_mode;
}


void mime::header_codec(header_codec_t hdr_codec)
{
    _header_codec = hdr_codec;
}


mime::header_codec_t mime::header_codec() const
{
    return _header_codec;
}


string mime::format_header() const
{
    return format_content_type() + format_transfer_encoding() + format_content_disposition();
}


string mime::format_content(bool dot_escape) const
{
    vector<string> content_lines;
    switch (_encoding)
    {
        case content_transfer_encoding_t::BASE_64:
        {
            base64 b(_line_policy, _decoder_line_policy);
            b.strict_mode(_strict_codec_mode);
            content_lines = b.encode(_content);
            break;
        }

        case content_transfer_encoding_t::QUOTED_PRINTABLE:
        {
            quoted_printable qp(_line_policy, _decoder_line_policy);
            qp.strict_mode(_strict_codec_mode);
            content_lines = qp.encode(_content);
            break;
        }

        // TODO: check how to handle 8bit chars, see [rfc 2045, section 2.8]
        case content_transfer_encoding_t::BIT_8:
        {
            bit8 b8(_line_policy, _decoder_line_policy);
            b8.strict_mode(_strict_codec_mode);
            content_lines = b8.encode(_content);
            break;
        }

        // TODO: check again if handling non-7bit chars is okay, see [rfc 2045, section 2.7]
        case content_transfer_encoding_t::BIT_7:
        case content_transfer_encoding_t::NONE:
        {
            bit7 b7(_line_policy, _decoder_line_policy);
            b7.strict_mode(_strict_codec_mode);
            content_lines = b7.encode(_content);
            break;
        }

        case content_transfer_encoding_t::BINARY:
        {
            // TODO: probably bug when `\0` is part of the content
            binary b(_line_policy, _decoder_line_policy);
            b.strict_mode(_strict_codec_mode);
            content_lines = b.encode(_content);
            break;
        }

        // default encoding is seven bit, so no `default` clause
    }

    string content;
    for (const auto& s : content_lines)
        if (dot_escape && s[0] == codec::DOT_CHAR)
            content += string(1, codec::DOT_CHAR) + s + codec::END_OF_LINE;
        else
            content += s + codec::END_OF_LINE;

    return content;
}


string mime::format_content_type() const
{
    string line;

    if (_content_type.type != media_type_t::NONE)
    {
        line += CONTENT_TYPE_HEADER + HEADER_SEPARATOR_STR + mime_type_as_str(_content_type.type) + CONTENT_SUBTYPE_SEPARATOR + _content_type.subtype;
        if (!_content_type.charset.empty())
            line += ATTRIBUTES_SEPARATOR_STR + content_type_t::ATTR_CHARSET + NAME_VALUE_SEPARATOR_STR + _content_type.charset;
        if (!_name.empty())
        {
            string mn = format_mime_name(_name);
            const string::size_type line_new_size = line.size() + ATTRIBUTES_SEPARATOR_STR.size() + ATTRIBUTE_NAME.size() + NAME_VALUE_SEPARATOR_STR.size() +
                sizeof(codec::QUOTE_CHAR) + mn.size() + sizeof(codec::QUOTE_CHAR);
            line += ATTRIBUTES_SEPARATOR_STR;
            if (line_new_size >= string::size_type(_line_policy) - codec::END_OF_LINE.length())
                line += codec::END_OF_LINE + NEW_LINE_INDENT;
            line += ATTRIBUTE_NAME + NAME_VALUE_SEPARATOR_STR + codec::QUOTE_STR + mn + codec::QUOTE_STR;
        }
        if (!_boundary.empty())
            line += ATTRIBUTES_SEPARATOR_STR + content_type_t::ATTR_BOUNDARY + NAME_VALUE_SEPARATOR_STR + codec::QUOTE_CHAR + _boundary + codec::QUOTE_CHAR;
        line += codec::END_OF_LINE;
    }

    return line;
}


string mime::format_transfer_encoding() const
{
    string line;

    switch (_encoding)
    {
        case content_transfer_encoding_t::BASE_64:
            line += CONTENT_TRANSFER_ENCODING_HEADER + HEADER_SEPARATOR_STR + CONTENT_TRANSFER_ENCODING_BASE64 + codec::END_OF_LINE;
            break;

        case content_transfer_encoding_t::QUOTED_PRINTABLE:
            line += CONTENT_TRANSFER_ENCODING_HEADER + HEADER_SEPARATOR_STR + CONTENT_TRANSFER_ENCODING_QUOTED_PRINTABLE + codec::END_OF_LINE;
            break;

        case content_transfer_encoding_t::BIT_8:
            line += CONTENT_TRANSFER_ENCODING_HEADER + HEADER_SEPARATOR_STR + CONTENT_TRANSFER_ENCODING_BIT8 + codec::END_OF_LINE;
            break;

        case content_transfer_encoding_t::BIT_7:
            line += CONTENT_TRANSFER_ENCODING_HEADER + HEADER_SEPARATOR_STR + CONTENT_TRANSFER_ENCODING_BIT7 + codec::END_OF_LINE;
            break;

        // default is no transfer encoding specified
        case content_transfer_encoding_t::NONE:
        default:
            break;
    }

    return line;
}


string mime::format_content_disposition() const
{
    string line;

    switch (_disposition)
    {
        case content_disposition_t::ATTACHMENT:
        {
            string name = format_mime_name(_name);
            line += CONTENT_DISPOSITION_HEADER + HEADER_SEPARATOR_STR + CONTENT_DISPOSITION_ATTACHMENT + ATTRIBUTES_SEPARATOR_STR;
            const string::size_type line_new_size = CONTENT_DISPOSITION_HEADER.size() + HEADER_SEPARATOR_STR.size() + CONTENT_DISPOSITION_ATTACHMENT.size() +
                ATTRIBUTE_FILENAME.size() + sizeof(codec::EQUAL_CHAR) + sizeof(codec::QUOTE_CHAR) + name.size() + sizeof(codec::QUOTE_CHAR) +
                codec::END_OF_LINE.size();
            if (line_new_size >= string::size_type(_line_policy) - codec::END_OF_LINE.length())
                line += codec::END_OF_LINE + NEW_LINE_INDENT;
            line += ATTRIBUTE_FILENAME + NAME_VALUE_SEPARATOR_STR + codec::QUOTE_CHAR + name + codec::QUOTE_STR + codec::END_OF_LINE;

            break;
        }

        case content_disposition_t::INLINE:
        {
            string name = format_mime_name(_name);
            line += CONTENT_DISPOSITION_HEADER + HEADER_SEPARATOR_STR + CONTENT_DISPOSITION_INLINE + ATTRIBUTES_SEPARATOR_STR;
            const string::size_type line_new_size = CONTENT_DISPOSITION_HEADER.size() + HEADER_SEPARATOR_STR.size() + CONTENT_DISPOSITION_INLINE.size() +
                ATTRIBUTE_FILENAME.size() + NAME_VALUE_SEPARATOR_STR.size() + sizeof(codec::QUOTE_CHAR) + name.size() + sizeof(codec::QUOTE_CHAR) + codec::END_OF_LINE.size();
            if (line_new_size >= string::size_type(_line_policy) - codec::END_OF_LINE.length())
                line += codec::END_OF_LINE + NEW_LINE_INDENT;
            line += ATTRIBUTE_FILENAME + NAME_VALUE_SEPARATOR_STR + codec::QUOTE_CHAR + name + codec::QUOTE_CHAR + codec::END_OF_LINE;

            break;
        }

        default:
            break;
    }

    return line;
}


string mime::format_mime_name(const string& name) const
{
    if (codec::is_utf8_string(name))
    {
        // if the attachment name exceeds mandatory length, the rest is discarded
        q_codec qc(codec::line_len_policy_t::MANDATORY, _decoder_line_policy, _header_codec);
        vector<string> hdr = qc.encode(name);
        return hdr.at(0);
    }

    return name;
}


void mime::parse_header()
{
    string line;
    for (const auto& hdr : _parsed_headers)
    {
        if (isspace(hdr[0]))
            line += trim_copy(hdr);
        else
        {
            if (!line.empty())
            {
                parse_header_line(line);
                line.clear();
            }
            line = hdr;
        }
    }
    if (!line.empty())
        parse_header_line(line);
}


void mime::parse_content()
{
    strip_body();

    switch (_encoding)
    {
        case content_transfer_encoding_t::BASE_64:
        {
            base64 b64(_line_policy, _decoder_line_policy);
            b64.strict_mode(_strict_codec_mode);
            _content = b64.decode(_parsed_body);
            break;
        }

        case content_transfer_encoding_t::QUOTED_PRINTABLE:
        {
            quoted_printable qp(_line_policy, _decoder_line_policy);
            qp.strict_mode(_strict_codec_mode);
            _content = qp.decode(_parsed_body);
            break;
        }

        case content_transfer_encoding_t::BIT_8:
        {
            bit8 b8(_line_policy, _decoder_line_policy);
            b8.strict_mode(_strict_codec_mode);
            _content = b8.decode(_parsed_body);
            break;
        }

        case content_transfer_encoding_t::BIT_7:
        case content_transfer_encoding_t::NONE:
        {
            bit7 b7(_line_policy, _decoder_line_policy);
            b7.strict_mode(_strict_codec_mode);
            _content = b7.decode(_parsed_body);
            break;
        }

        case content_transfer_encoding_t::BINARY:
        {
            binary b(_line_policy, _decoder_line_policy);
            b.strict_mode(_strict_codec_mode);
            _content = b.decode(_parsed_body);
            break;
        }

        // default encoding is seven bit, so no `default` clause
    }
}


/*
Some of the headers cannot be empty by RFC, but still they can occur. Thus, parser strict mode has to be introduced; in case it's false, default
values are set. The following headers are recognized by the parser:
- `Content-Type` cannot be empty by RFC 2045 section 5. In case it's empty, default value `text/plain` can be set.
- `Content-Transfer-Encoding` is not mandatory RFC 2045, section 6.1. In case it's empty, it should be seven bit by default.
- `Content-Disposition` cannot be empty by RFC 2183, section 2. In case it's empty, consider it to be inline.

By RFC 2045 section 6.4, if an entity is of type Multipart, the content transfer encoding must be Seven Bit, Eight Bit or Binary.
*/
void mime::parse_header_line(const string& header_line)
{
    string header_name, header_value;
    parse_header_name_value(header_line, header_name, header_value);

    if (iequals(header_name, CONTENT_TYPE_HEADER))
    {
        media_type_t media_type;
        string media_subtype;
        attributes_t attributes;
        parse_content_type(header_value, media_type, media_subtype, attributes);

        _content_type.type = media_type;
        _content_type.subtype = to_lower_copy(media_subtype);
        attributes_t::iterator bound_it = attributes.find(content_type_t::ATTR_BOUNDARY);
        if (bound_it != attributes.end())
            _boundary = bound_it->second;
        attributes_t::iterator charset_it = attributes.find(content_type_t::ATTR_CHARSET);
        if (charset_it != attributes.end())
            _content_type.charset = to_lower_copy(charset_it->second);
        attributes_t::iterator name_it = attributes.find(ATTRIBUTE_NAME);
        // name is set if not already set by content disposition
        if (name_it != attributes.end() && _name.empty())
        {
            q_codec qc(_line_policy, _decoder_line_policy, _header_codec);
            _name = qc.check_decode(name_it->second);
        }
    }
    else if (iequals(header_name, CONTENT_TRANSFER_ENCODING_HEADER))
    {
        attributes_t attributes;
        parse_content_transfer_encoding(header_value, _encoding, attributes);
    }
    else if (iequals(header_name, CONTENT_DISPOSITION_HEADER))
    {
        attributes_t attributes;
        parse_content_disposition(header_value, _disposition, attributes);
        // filename is stored no matter if name is already set by content type
        attributes_t::iterator filename_it = attributes.find(ATTRIBUTE_FILENAME);
        if (filename_it != attributes.end())
        {
            q_codec qc(_line_policy, _decoder_line_policy, _header_codec);
            _name = qc.check_decode(filename_it->second);
        }
    }
}


void mime::parse_header_name_value(const string& header_line, string& header_name, string& header_value) const
{
    string::size_type colon_pos = header_line.find(HEADER_SEPARATOR_CHAR);
    if (colon_pos == string::npos)
        throw mime_error("Parsing failure of header line.");

    header_name = header_line.substr(0, colon_pos);
    trim(header_name);
    header_value = header_line.substr(colon_pos + 1);
    trim(header_value);

    if (header_name.empty())
        throw mime_error("Parsing failure, header name or value empty: " + header_line);
    smatch m;
    if (!regex_match(header_name, m, HEADER_NAME_REGEX))
        throw mime_error("Format failure of the header name `" + header_name + "`.");
    if (header_value.empty())
    {
        if (_strict_mode)
        {
            throw mime_error("Parsing failure, header name or value empty: " + header_line);
        }
        else
        {
            return;
        }
    }

    if (!regex_match(header_value, m, HEADER_VALUE_REGEX))
        throw mime_error("Format failure of the header value `" + header_value + "`.");
}


void mime::parse_content_type(const string& content_type_hdr, media_type_t& media_type, string& media_subtype, attributes_t& attributes) const
{
    string header_value;
    parse_header_value_attributes(content_type_hdr, header_value, attributes);
    string media_type_s;
    // flag if media type or subtype is being parsed
    bool is_media_type = true;
    for (auto ch : header_value)
    {
        if (ch == CONTENT_SUBTYPE_SEPARATOR)
        {
            media_type = mime_type_as_enum(media_type_s);
            is_media_type = false;
        }
        else if (!isalpha(ch) && !isdigit(ch) && CONTENT_ATTR_ALPHABET.find(ch) == string::npos)
        {
            throw mime_error("Parsing content type value failure.");
        }
        else if (is_media_type)
        {
            media_type_s += ch;
        }
        else if (!is_media_type)
        {
            media_subtype += ch;
        }
    }
}


void mime::parse_content_transfer_encoding(const string& transfer_encoding_hdr, content_transfer_encoding_t& encoding, attributes_t& attributes) const
{
    string header_value;
    parse_header_value_attributes(transfer_encoding_hdr, header_value, attributes);

    if (iequals(header_value, CONTENT_TRANSFER_ENCODING_BASE64))
        encoding = content_transfer_encoding_t::BASE_64;
    else if (iequals(header_value, CONTENT_TRANSFER_ENCODING_QUOTED_PRINTABLE))
        encoding = content_transfer_encoding_t::QUOTED_PRINTABLE;
    else if (iequals(header_value, CONTENT_TRANSFER_ENCODING_BIT7))
        encoding = content_transfer_encoding_t::BIT_7;
    else if (iequals(header_value, CONTENT_TRANSFER_ENCODING_BIT8))
        encoding = content_transfer_encoding_t::BIT_8;
    else if (iequals(header_value, CONTENT_TRANSFER_ENCODING_BINARY))
        encoding = content_transfer_encoding_t::BINARY;
    else if (_strict_mode)
        throw mime_error("Parsing content transfer encoding failure.");
    else
        encoding = content_transfer_encoding_t::BIT_7;
}


void mime::parse_content_disposition(const string& content_disp_hdr, content_disposition_t& disposition, attributes_t& attributes) const
{
    string header_value;
    parse_header_value_attributes(content_disp_hdr, header_value, attributes);
    if (iequals(header_value, CONTENT_DISPOSITION_ATTACHMENT))
        disposition = content_disposition_t::ATTACHMENT;
    else if (iequals(header_value, CONTENT_DISPOSITION_INLINE))
        disposition = content_disposition_t::INLINE;
    else
    {
        if (_strict_mode)
            throw mime_error("Parsing content disposition failure.");
        else
            disposition = content_disposition_t::ATTACHMENT;
    }
}


/*
Implementation goes by using state machine. Diagram is shown in graphviz dot language:
```
digraph header_value_attributes
{
    rankdir=LR;
    node [shape = box];

    begin -> begin [label="space"];
    begin -> value [label="alphanumeric"];
    value -> value [label="token"];
    value -> attrbegin [label="semicolon"];
    attrbegin -> attrbegin [label="space"];
    attrbegin -> attrname [label="token"];
    attrname -> attrname [label="token"];
    attrname -> attrvalue [label="equal_sign"];
    attrvalue -> qattrvaluebegin [label="quote"];
    qattrvaluebegin -> qattrvaluebegin [label="qtext"];
    qattrvaluebegin -> attrvalueend [label="quote"];
    attrvalue -> attrvaluebegin [label="token"];
    attrvaluebegin -> attrvaluebegin [label="token"];
    attrvaluebegin -> attrvalueend [label="space"];
    attrvaluebegin -> attrbegin [label="semicolon"];
    attrvalueend -> attrbegin [label="semicolon"];
    attrvalueend -> end [label="eol"];
    attrvalueend -> attrvalueend [label="space"];
}
```
For details see [rfc 2045, section 5.1].
*/
void mime::parse_header_value_attributes(const string& header, string& header_value, attributes_t& attributes) const
{
    enum class state_t {BEGIN, VALUE, ATTR_BEGIN, ATTR_NAME, ATTR_VALUE, QATTR_VALUE_BEGIN, ATTR_VALUE_BEGIN, ATTR_VALUE_END, END};
    state_t state = state_t::BEGIN;
    string attribute_name, attribute_value;

    for (auto ch = header.begin(); ch != header.end(); ch++)
    {
        switch (state)
        {
            case state_t::BEGIN:
                if (isspace(*ch))
                    ;
                else if (isalpha(*ch) || isdigit(*ch))
                {
                    state = state_t::VALUE;
                    header_value += *ch;
                }
                else
                    throw mime_error("Parsing header value failure at `" + string(1, *ch) + "`.");
                break;

            case state_t::VALUE:
                if (isalpha(*ch) || isdigit(*ch) || CONTENT_HEADER_VALUE_ALPHABET.find(*ch) != string::npos)
                    header_value += *ch;
                else if (*ch == ATTRIBUTES_SEPARATOR_CHAR)
                    state = state_t::ATTR_BEGIN;
                else
                    throw mime_error("Parsing header value failure at `" + string(1, *ch) + "`.");
                break;

            case state_t::ATTR_BEGIN:
                if (isspace(*ch))
                    ;
                else if (isalpha(*ch) || isdigit(*ch) || CONTENT_ATTR_ALPHABET.find(*ch) != string::npos)
                {
                    state = state_t::ATTR_NAME;
                    attribute_name += *ch;
                }
                else if (*ch == NAME_VALUE_SEPARATOR_CHAR)
                    state = state_t::ATTR_VALUE;
                else
                    throw mime_error("Parsing attribute name failure at `" + string(1, *ch) + "`.");
                break;

            case state_t::ATTR_NAME:
                if (isalpha(*ch) || isdigit(*ch) || CONTENT_ATTR_ALPHABET.find(*ch) != string::npos)
                    attribute_name += *ch;
                else if (*ch == NAME_VALUE_SEPARATOR_CHAR)
                    state = state_t::ATTR_VALUE;
                break;

            case state_t::ATTR_VALUE:
                if (*ch == codec::QUOTE_CHAR)
                    state = state_t::QATTR_VALUE_BEGIN;
                else if (isalpha(*ch) || isdigit(*ch) || CONTENT_ATTR_ALPHABET.find(*ch) != string::npos)
                {
                    state = state_t::ATTR_VALUE_BEGIN;
                    attribute_value += *ch;
                }
                else
                    throw mime_error("Parsing attribute value failure at `" + string(1, *ch) + "`.");
                break;

            case state_t::QATTR_VALUE_BEGIN:
                if (isalpha(*ch) || isdigit(*ch) || QTEXT.find(*ch) != string::npos)
                    attribute_value += *ch;
                else if (*ch == codec::QUOTE_CHAR)
                    state = state_t::ATTR_VALUE_END;
                else
                    throw mime_error("Parsing attribute value failure at `" + string(1, *ch) + "`.");
                if (ch == header.end() - 1)
                    attributes[attribute_name] = attribute_value;
                break;

            case state_t::ATTR_VALUE_BEGIN:
                if (isalpha(*ch) || isdigit(*ch) || CONTENT_ATTR_ALPHABET.find(*ch) != string::npos)
                    attribute_value += *ch;
                else if (isspace(*ch))
                    state = state_t::ATTR_VALUE_END;
                else if (*ch == ATTRIBUTES_SEPARATOR_CHAR)
                {
                    state = state_t::ATTR_BEGIN;
                    attributes[attribute_name] = attribute_value;
                    attribute_name.clear();
                    attribute_value.clear();
                }
                else
                    throw mime_error("Parsing attribute value failure at `" + string(1, *ch) + "`.");
                if (ch == header.end() - 1)
                    attributes[attribute_name] = attribute_value;
                break;

            case state_t::ATTR_VALUE_END:
                attributes[attribute_name] = attribute_value;
                attribute_name.clear();
                attribute_value.clear();

                if (isspace(*ch))
                    ;
                else if (*ch == ATTRIBUTES_SEPARATOR_CHAR)
                    state = state_t::ATTR_BEGIN;
                else
                    throw mime_error("Parsing attribute value failure at `" + string(1, *ch) + "`.");
                break;

            case state_t::END:
                return;
        }
    }
}


string mime::make_boundary() const
{
    string bound;
    bound.reserve(10);
    std::random_device rng;
    std::uniform_int_distribution<> index_dist(0, codec::HEX_DIGITS.size() - 1);
    for (int i = 0; i < 10; i++)
        bound += codec::HEX_DIGITS[index_dist(rng)];
    return BOUNDARY_DELIMITER + BOUNDARY_DELIMITER + BOUNDARY_DELIMITER + BOUNDARY_DELIMITER + bound;
}


string mime::mime_type_as_str(media_type_t media_type_val) const
{
    string mime_str;
    switch (media_type_val)
    {
        case media_type_t::TEXT:
            mime_str = "text";
            break;

        case media_type_t::IMAGE:
            mime_str = "image";
            break;

        case media_type_t::AUDIO:
            mime_str = "audio";
            break;

        case media_type_t::VIDEO:
            mime_str = "video";
            break;

        case media_type_t::APPLICATION:
            mime_str = "application";
            break;

        case media_type_t::MULTIPART:
            mime_str = "multipart";
            break;

        case media_type_t::MESSAGE:
            mime_str = "message";
            break;
        default:
            break;
    }
    return mime_str;
}


mime::media_type_t mime::mime_type_as_enum(const string& media_type_val) const
{
    media_type_t media_type = media_type_t::NONE;
    if (iequals(media_type_val, "text"))
        media_type = media_type_t::TEXT;
    else if (iequals(media_type_val, "image"))
        media_type = media_type_t::IMAGE;
    else if (iequals(media_type_val, "audio"))
        media_type = media_type_t::AUDIO;
    else if (iequals(media_type_val, "video"))
        media_type = media_type_t::VIDEO;
    else if (iequals(media_type_val, "application"))
        media_type = media_type_t::APPLICATION;
    else if (iequals(media_type_val, "multipart"))
        media_type = media_type_t::MULTIPART;
    else if (iequals(media_type_val, "message"))
        media_type = media_type_t::MESSAGE;
    else if (_strict_mode)
        throw mime_error("Bad media type.");
    return media_type;
}


void mime::strip_body()
{
    while (!_parsed_body.empty())
        if (_parsed_body.back().empty())
            _parsed_body.pop_back();
        else
            break;
}

} // namespace mailio
