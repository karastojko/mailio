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
#if defined(__cpp_char8_t)
using std::u8string;
#endif
using std::ifstream;
using std::stringstream;
using std::pair;
using std::multimap;
using std::vector;
using std::map;
using std::get;
using boost::trim_left_if;
using boost::trim_right_if;
using boost::regex;
using boost::regex_match;
using boost::smatch;
using boost::match_flag_type;
using boost::match_results;
using boost::to_lower_copy;
using boost::trim_copy;
using boost::trim;
using boost::trim_right;
using boost::iequals;
using boost::algorithm::is_any_of;


namespace mailio
{


const string mime::CONTENT_ID_HEADER{"Content-ID"};
const string mime::ADDRESS_BEGIN_STR(1, ADDRESS_BEGIN_CHAR);
const string mime::ADDRESS_END_STR(1, ADDRESS_END_CHAR);
const string mime::MESSAGE_ID_REGEX = "([a-zA-Z0-9\\!#\\$%&'\\*\\+\\-\\./=\\?\\^\\_`\\{\\|\\}\\~]+)"
    "\\@([a-zA-Z0-9\\!#\\$%&'\\*\\+\\-\\./=\\?\\^\\_`\\{\\|\\}\\~]+)";
const string mime::MESSAGE_ID_REGEX_NS = "([a-zA-Z0-9\\!#\\$%&'\\*\\+\\-\\./=\\?\\^\\_`\\{\\|\\}\\~\\@\\\\ \\t<\\>]*)";
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
const regex mime::HEADER_VALUE_REGEX{R"(([a-zA-Z0-9\ \t\!\"#\$%&'\(\)\*\+\,\-\./:;\<=\>\?@\[\\\]\^\_`\{\|\}\~]+))"};
const string mime::CONTENT_ATTR_ALPHABET{"!#$%&*+-.^_`|~"};
const string mime::CONTENT_HEADER_VALUE_ALPHABET{"!#$%&*+-./^_`|~"};


mime::mime() : version_("1.0"), line_policy_(codec::line_len_policy_t::RECOMMENDED),
    decoder_line_policy_(codec::line_len_policy_t::RECOMMENDED), strict_mode_(false), strict_codec_mode_(false),
    header_codec_(header_codec_t::UTF8), content_type_(media_type_t::NONE, ""), encoding_(content_transfer_encoding_t::NONE),
    disposition_(content_disposition_t::NONE), parsing_header_(true), mime_status_(mime_parsing_status_t::NONE)
{
}


void mime::format(string& mime_str, bool dot_escape) const
{
    if (!boundary_.empty() && content_type_.type != media_type_t::MULTIPART)
        throw mime_error("Formatting failure, non multipart message with boundary.");

    mime_str += format_header() + codec::END_OF_LINE;
    string content = format_content(dot_escape);
    mime_str += content;

    if (!parts_.empty())
    {
        if (!content.empty())
            mime_str += codec::END_OF_LINE;
        // recursively format mime parts
        for (auto& p : parts_)
        {
            string p_str;
            p.format(p_str, dot_escape);
            mime_str += BOUNDARY_DELIMITER + boundary_ + codec::END_OF_LINE + p_str + codec::END_OF_LINE;
        }
        mime_str += BOUNDARY_DELIMITER + boundary_ + BOUNDARY_DELIMITER + codec::END_OF_LINE;
    }
}


#if defined(__cpp_char8_t)
void mime::format(u8string& mime_str, bool dot_escape) const
{
    string ms = reinterpret_cast<const char*>(mime_str.c_str());
    format(ms, dot_escape);
}
#endif


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


#if defined(__cpp_char8_t)
void mime::parse(const u8string& mime_string, bool dot_escape)
{
    parse(reinterpret_cast<const char*>(mime_string.c_str()), dot_escape);
}
#endif


mime& mime::parse_by_line(const string& line, bool dot_escape)
{
    if (line.length() > string::size_type(decoder_line_policy_))
        throw mime_error("Line policy overflow in a header.");

    // mark end of header and parse it
    if (parsing_header_ && line.empty())
    {
        parsing_header_ = false;
        parse_header();
    }
    else
    {
        if (parsing_header_)
            parsed_headers_.push_back(line);
        else
        {
            // end of message reached, decode the body
            if (line == codec::END_OF_LINE)
            {
                parse_content();
                mime_status_ = mime_parsing_status_t::END;
            }
            // parsing the body
            else
            {
                // mime part sequence begins
                if (line == BOUNDARY_DELIMITER + boundary_ && !boundary_.empty())
                {
                    mime_status_ = mime_parsing_status_t::BEGIN;
                    // begin of another mime part means that the current part (if exists) is ended and parsed; another part is created
                    if (!parts_.empty())
                        parts_.back().parse_by_line(codec::END_OF_LINE);
                    mime m;
                    m.line_policy(line_policy_, decoder_line_policy_);
                    m.strict_codec_mode(strict_codec_mode_);
                    parts_.push_back(m);
                }
                // mime part sequence ends, so parse the last mime part
                else if (line == BOUNDARY_DELIMITER + boundary_ + BOUNDARY_DELIMITER && !boundary_.empty())
                {
                    mime_status_ = mime_parsing_status_t::END;
                    parts_.back().parse_by_line(codec::END_OF_LINE);
                }
                // mime content being parsed
                else
                {
                    // parser entered mime body
                    if (mime_status_ == mime_parsing_status_t::BEGIN)
                        parts_.back().parse_by_line(line, dot_escape);
                    // put the line into `parsed_body_` until the whole body is read for parsing
                    else
                    {
                        if (dot_escape && line[0] == codec::DOT_CHAR)
                            parsed_body_.push_back(line.substr(1));
                        else
                            parsed_body_.push_back(line);
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
    content_type_ = cont_type;
}


void mime::content_type(media_type_t media_type, const string& media_subtype, const string& charset)
{
    content_type_t c(media_type, media_subtype);
    c.charset = to_lower_copy(charset);
    content_type(c);
}


mime::content_type_t mime::content_type() const
{
    return content_type_;
}


void mime::content_id(string id)
{
    const regex r(strict_mode_ ? MESSAGE_ID_REGEX : MESSAGE_ID_REGEX_NS);
    smatch m;

    if (regex_match(id, m, r))
        content_id_ = id;
    else
        throw mime_error("Invalid content ID.");
}


string mime::content_id() const
{
    return content_id_;
}


void mime::name(const string& mime_name)
{
    name_ = mime_name;
}


string mime::name() const
{
    return name_;
}


void mime::content_transfer_encoding(content_transfer_encoding_t encoding)
{
    encoding_ = encoding;
}


mime::content_transfer_encoding_t mime::content_transfer_encoding() const
{
    return encoding_;
}


void mime::content_disposition(content_disposition_t disposition)
{
    disposition_ = disposition;
}


mime::content_disposition_t mime::content_disposition() const
{
    return disposition_;
}


void mime::boundary(const string& bound)
{
    boundary_ = bound;
}


string mime::boundary() const
{
    return boundary_;
}


void mime::content(const string& content_str)
{
    content_ = content_str;
}


#if defined(__cpp_char8_t)
void mime::content(const u8string& content_str)
{
    content_ = string(reinterpret_cast<const char*>(content_str.c_str()));
}
#endif


string mime::content() const
{
    return content_;
}


void mime::add_part(const mime& part)
{
    parts_.push_back(part);
}


vector<mime> mime::parts() const
{
    return parts_;
}


void mime::line_policy(codec::line_len_policy_t encoder_line_policy, codec::line_len_policy_t decoder_line_policy)
{
    line_policy_ = encoder_line_policy;
    decoder_line_policy_ = decoder_line_policy;
}


codec::line_len_policy_t mime::line_policy() const
{
    return line_policy_;
}


codec::line_len_policy_t mime::decoder_line_policy() const
{
    return decoder_line_policy_;
}


void mime::strict_mode(bool mode)
{
    strict_mode_ = mode;
}


bool mime::strict_mode() const
{
    return strict_mode_;
}


void mime::strict_codec_mode(bool mode)
{
    strict_codec_mode_ = mode;
}


bool mime::strict_codec_mode() const
{
    return strict_codec_mode_;
}


void mime::header_codec(header_codec_t hdr_codec)
{
    header_codec_ = hdr_codec;
}


mime::header_codec_t mime::header_codec() const
{
    return header_codec_;
}


string mime::format_many_ids(const vector<string>& ids)
{
    string ids_str;
    for (auto s = ids.begin(); s != ids.end(); s++)
    {
        ids_str += ADDRESS_BEGIN_STR + *s + ADDRESS_END_STR;
        if (s != ids.end() - 1)
            ids_str += codec::SPACE_STR;
    }
    return ids_str;
}


string mime::format_many_ids(const string& id)
{
    return format_many_ids(vector<string>{id});
}


vector<string> mime::parse_many_ids(const string& ids) const
{
    if (!strict_mode_)
        return vector<string>{ids};

    vector<string> idv;
    auto start = ids.cbegin();
    auto end = ids.cend();
    match_flag_type flags = boost::match_default | boost::match_not_null;
    match_results<string::const_iterator> tokens;
    bool all_tokens_parsed = false;
    const string ANGLED_MESSAGE_ID_REGEX = codec::LESS_THAN_STR + MESSAGE_ID_REGEX + codec::GREATER_THAN_STR;
    const regex rgx(ANGLED_MESSAGE_ID_REGEX);
    while (regex_search(start, end, tokens, rgx, flags))
    {
        string id = tokens[0];
        trim_left_if(id, is_any_of(codec::LESS_THAN_STR));
        trim_right_if(id, is_any_of(codec::GREATER_THAN_STR));
        idv.push_back(id);
        start = tokens[0].second;
        all_tokens_parsed = (start == end);
    }

    if (!all_tokens_parsed)
        throw mime_error("Parsing failure of the ID: " + ids);

    return idv;
}


string mime::format_header() const
{
    return format_content_type() + format_transfer_encoding() + format_content_disposition() + format_content_id();
}


string mime::format_content(bool dot_escape) const
{
    vector<string> content_lines;
    switch (encoding_)
    {
        case content_transfer_encoding_t::BASE_64:
        {
            base64 b(line_policy_, decoder_line_policy_);
            b.strict_mode(strict_codec_mode_);
            content_lines = b.encode(content_);
            break;
        }

        case content_transfer_encoding_t::QUOTED_PRINTABLE:
        {
            quoted_printable qp(line_policy_, decoder_line_policy_);
            qp.strict_mode(strict_codec_mode_);
            content_lines = qp.encode(content_);
            break;
        }

        // TODO: check how to handle 8bit chars, see [rfc 2045, section 2.8]
        case content_transfer_encoding_t::BIT_8:
        {
            bit8 b8(line_policy_, decoder_line_policy_);
            b8.strict_mode(strict_codec_mode_);
            content_lines = b8.encode(content_);
            break;
        }

        // TODO: check again if handling non-7bit chars is okay, see [rfc 2045, section 2.7]
        case content_transfer_encoding_t::BIT_7:
        case content_transfer_encoding_t::NONE:
        {
            bit7 b7(line_policy_, decoder_line_policy_);
            b7.strict_mode(strict_codec_mode_);
            content_lines = b7.encode(content_);
            break;
        }

        case content_transfer_encoding_t::BINARY:
        {
            // TODO: probably bug when `\0` is part of the content
            binary b(line_policy_, decoder_line_policy_);
            b.strict_mode(strict_codec_mode_);
            content_lines = b.encode(content_);
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

    if (content_type_.type != media_type_t::NONE)
    {
        line += CONTENT_TYPE_HEADER + HEADER_SEPARATOR_STR + mime_type_as_str(content_type_.type) + CONTENT_SUBTYPE_SEPARATOR + content_type_.subtype;
        if (!content_type_.charset.empty())
            line += ATTRIBUTES_SEPARATOR_STR + content_type_t::ATTR_CHARSET + NAME_VALUE_SEPARATOR_STR + content_type_.charset;
        if (!name_.empty())
        {
            string mn = format_mime_name(name_);
            const string::size_type line_new_size = line.size() + ATTRIBUTES_SEPARATOR_STR.size() + ATTRIBUTE_NAME.size() + NAME_VALUE_SEPARATOR_STR.size() +
                sizeof(codec::QUOTE_CHAR) + mn.size() + sizeof(codec::QUOTE_CHAR);
            line += ATTRIBUTES_SEPARATOR_STR;
            if (line_new_size >= string::size_type(line_policy_) - codec::END_OF_LINE.length())
                line += codec::END_OF_LINE + NEW_LINE_INDENT;
            line += ATTRIBUTE_NAME + NAME_VALUE_SEPARATOR_STR + codec::QUOTE_STR + mn + codec::QUOTE_STR;
        }
        if (!boundary_.empty())
            line += ATTRIBUTES_SEPARATOR_STR + content_type_t::ATTR_BOUNDARY + NAME_VALUE_SEPARATOR_STR + codec::QUOTE_CHAR + boundary_ + codec::QUOTE_CHAR;
        line += codec::END_OF_LINE;
    }

    return line;
}


string mime::format_transfer_encoding() const
{
    string line;

    switch (encoding_)
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

    switch (disposition_)
    {
        case content_disposition_t::ATTACHMENT:
        {
            string name = format_mime_name(name_);
            line += CONTENT_DISPOSITION_HEADER + HEADER_SEPARATOR_STR + CONTENT_DISPOSITION_ATTACHMENT + ATTRIBUTES_SEPARATOR_STR;
            const string::size_type line_new_size = CONTENT_DISPOSITION_HEADER.size() + HEADER_SEPARATOR_STR.size() + CONTENT_DISPOSITION_ATTACHMENT.size() +
                ATTRIBUTE_FILENAME.size() + sizeof(codec::EQUAL_CHAR) + sizeof(codec::QUOTE_CHAR) + name.size() + sizeof(codec::QUOTE_CHAR) +
                codec::END_OF_LINE.size();
            if (line_new_size >= string::size_type(line_policy_) - codec::END_OF_LINE.length())
                line += codec::END_OF_LINE + NEW_LINE_INDENT;
            line += ATTRIBUTE_FILENAME + NAME_VALUE_SEPARATOR_STR + codec::QUOTE_CHAR + name + codec::QUOTE_STR + codec::END_OF_LINE;

            break;
        }

        case content_disposition_t::INLINE:
        {
            string name = format_mime_name(name_);
            line += CONTENT_DISPOSITION_HEADER + HEADER_SEPARATOR_STR + CONTENT_DISPOSITION_INLINE + ATTRIBUTES_SEPARATOR_STR;
            const string::size_type line_new_size = CONTENT_DISPOSITION_HEADER.size() + HEADER_SEPARATOR_STR.size() + CONTENT_DISPOSITION_INLINE.size() +
                ATTRIBUTE_FILENAME.size() + NAME_VALUE_SEPARATOR_STR.size() + sizeof(codec::QUOTE_CHAR) + name.size() + sizeof(codec::QUOTE_CHAR) + codec::END_OF_LINE.size();
            if (line_new_size >= string::size_type(line_policy_) - codec::END_OF_LINE.length())
                line += codec::END_OF_LINE + NEW_LINE_INDENT;
            line += ATTRIBUTE_FILENAME + NAME_VALUE_SEPARATOR_STR + codec::QUOTE_CHAR + name + codec::QUOTE_CHAR + codec::END_OF_LINE;

            break;
        }

        default:
            break;
    }

    return line;
}


string mime::format_content_id() const
{
    return content_id_.empty() ? "" : CONTENT_ID_HEADER + HEADER_SEPARATOR_STR + format_many_ids(content_id_) + codec::END_OF_LINE;
}


string mime::format_mime_name(const string& name) const
{
    if (codec::is_utf8_string(name))
    {
        // if the attachment name exceeds mandatory length, the rest is discarded
        q_codec qc(codec::line_len_policy_t::MANDATORY, decoder_line_policy_);
        vector<string> hdr = qc.encode(name, codec::CHARSET_UTF8, header_codec_);
        return hdr.at(0);
    }

    return name;
}


void mime::parse_header()
{
    string line;
    for (const auto& hdr : parsed_headers_)
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

    switch (encoding_)
    {
        case content_transfer_encoding_t::BASE_64:
        {
            base64 b64(line_policy_, decoder_line_policy_);
            b64.strict_mode(strict_codec_mode_);
            content_ = b64.decode(parsed_body_);
            break;
        }

        case content_transfer_encoding_t::QUOTED_PRINTABLE:
        {
            quoted_printable qp(line_policy_, decoder_line_policy_);
            qp.strict_mode(strict_codec_mode_);
            content_ = qp.decode(parsed_body_);
            break;
        }

        case content_transfer_encoding_t::BIT_8:
        {
            bit8 b8(line_policy_, decoder_line_policy_);
            b8.strict_mode(strict_codec_mode_);
            content_ = b8.decode(parsed_body_);
            break;
        }

        case content_transfer_encoding_t::BIT_7:
        case content_transfer_encoding_t::NONE:
        {
            bit7 b7(line_policy_, decoder_line_policy_);
            b7.strict_mode(strict_codec_mode_);
            content_ = b7.decode(parsed_body_);
            break;
        }

        case content_transfer_encoding_t::BINARY:
        {
            binary b(line_policy_, decoder_line_policy_);
            b.strict_mode(strict_codec_mode_);
            content_ = b.decode(parsed_body_);
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

        content_type_.type = media_type;
        content_type_.subtype = to_lower_copy(media_subtype);
        attributes_t::iterator bound_it = attributes.find(content_type_t::ATTR_BOUNDARY);
        if (bound_it != attributes.end())
            boundary_ = bound_it->second;
        attributes_t::iterator charset_it = attributes.find(content_type_t::ATTR_CHARSET);
        if (charset_it != attributes.end())
            content_type_.charset = to_lower_copy(charset_it->second);
        attributes_t::iterator name_it = attributes.find(ATTRIBUTE_NAME);
        // name is set if not already set by content disposition
        if (name_it != attributes.end() && name_.empty())
        {
            q_codec qc(line_policy_, decoder_line_policy_);
            name_ = get<0>(qc.check_decode(name_it->second));
        }
    }
    else if (iequals(header_name, CONTENT_TRANSFER_ENCODING_HEADER))
    {
        attributes_t attributes;
        parse_content_transfer_encoding(header_value, encoding_, attributes);
    }
    else if (iequals(header_name, CONTENT_DISPOSITION_HEADER))
    {
        attributes_t attributes;
        parse_content_disposition(header_value, disposition_, attributes);
        merge_attributes(attributes);
        // filename is stored no matter if name is already set by content type
        attributes_t::iterator filename_it = attributes.find(ATTRIBUTE_FILENAME);
        if (filename_it != attributes.end())
        {
            q_codec qc(line_policy_, decoder_line_policy_);
            name_ = get<0>(qc.check_decode(filename_it->second));
        }
    }
    else if (iequals(header_name, CONTENT_ID_HEADER))
    {
        auto ids = parse_many_ids(header_value);
        if (!ids.empty())
            content_id_ = ids[0];
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
        if (strict_mode_)
        {
            throw mime_error("Parsing failure, header name or value empty: " + header_line);
        }
        else
        {
            return;
        }
    }

    if (!codec::is_utf8_string(header_value) && !regex_match(header_value, m, HEADER_VALUE_REGEX))
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
    else if (strict_mode_)
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
        if (strict_mode_)
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
    attrname -> preequal [label="space" style="dashed"]
    preequal [style="dashed"];
    preequal -> attrsep [label="equal_sign"];
    preequal -> preequal [label="space" style="dashed"];
    attrsep -> postequal [label="space" style="dashed"]
    postequal [style="dashed"];
    postequal -> qattrvaluebegin [label="quote"];
    postequal -> attrvaluebegin [label="token"];
    postequal -> postequal [label="space" style="dashed"];
    qattrvaluebegin -> qattrvaluebegin [label="qtext"];
    qattrvaluebegin -> qattrvaluebegin [label="backslash" style="dashed"];
    qattrvaluebegin -> attrvalueend [label="quote"];
    attrvaluebegin -> attrvaluebegin [label="token"];
    attrvaluebegin -> attrvaluebegin [label="backslash" style="dashed"];
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
    enum class state_t {BEGIN, VALUE, ATTR_BEGIN, ATTR_NAME, PRE_EQUAL, ATTR_SEP, POST_EQUAL, QATTR_VALUE_BEGIN, ATTR_VALUE_BEGIN, ATTR_VALUE_END, END};
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
                    state = state_t::ATTR_SEP;
                else
                    throw mime_error("Parsing attribute name failure at `" + string(1, *ch) + "`.");
                break;

            case state_t::ATTR_NAME:
                if (isalpha(*ch) || isdigit(*ch) || CONTENT_ATTR_ALPHABET.find(*ch) != string::npos)
                    attribute_name += *ch;
                else if (isspace(*ch) && !strict_mode_)
                    state = state_t::PRE_EQUAL;
                else if (*ch == NAME_VALUE_SEPARATOR_CHAR)
                    state = state_t::ATTR_SEP;
                break;

            case state_t::PRE_EQUAL:
                if (isspace(*ch) && !strict_mode_)
                    ;
                else if (*ch == NAME_VALUE_SEPARATOR_CHAR)
                    state = state_t::ATTR_SEP;
                break;

            case state_t::ATTR_SEP:
                if (isspace(*ch) && !strict_mode_)
                    state = state_t::POST_EQUAL;
                else if (*ch == codec::QUOTE_CHAR)
                    state = state_t::QATTR_VALUE_BEGIN;
                else if (isalpha(*ch) || isdigit(*ch) || CONTENT_ATTR_ALPHABET.find(*ch) != string::npos)
                {
                    state = state_t::ATTR_VALUE_BEGIN;
                    attribute_value += *ch;
                }
                else
                    throw mime_error("Parsing attribute value failure at `" + string(1, *ch) + "`.");
                break;

            case state_t::POST_EQUAL:
                if (isspace(*ch) && !strict_mode_)
                    ;
                else if (*ch == codec::QUOTE_CHAR)
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
                else if (!strict_mode_ && *ch == codec::BACKSLASH_CHAR)
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
                else if (!strict_mode_ && *ch == codec::BACKSLASH_CHAR)
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


void mime::merge_attributes(attributes_t& attributes) const
{
    map<string, map<int, string>> attribute_parts;
    for (auto attr = attributes.begin(); attr != attributes.end(); )
    {
        auto full_attr_name = attr->first;
        auto attr_value = attr->second;
        string::size_type asterisk_pos = full_attr_name.find(ATTRIBUTE_MULTIPLE_NAME_INDICATOR);
        if (asterisk_pos != string::npos)
        {
            string attr_name = full_attr_name.substr(0, asterisk_pos);
            try
            {
                if (!full_attr_name.substr(asterisk_pos + 1).empty())
                {
                    int attr_part = stoi(full_attr_name.substr(asterisk_pos + 1));
                    attribute_parts[attr_name][attr_part] = attr_value;
                    attr = attributes.erase(attr);
                }
            }
            catch (const std::invalid_argument& exc)
            {
                throw mime_error("Parsing attribute failure at `" + attr_name + "`.");
            }
            catch (const std::out_of_range& exc)
            {
                throw mime_error("Parsing attribute failure at `" + attr_name + "`.");
            }
        }
        else
            attr++;
    }

    for (auto attr = attribute_parts.begin(); attr != attribute_parts.end(); attr++)
    {
        string attr_name = attr->first;
        string attr_value;
        for (auto part = attr->second.begin(); part != attr->second.end(); part++)
            attr_value += part->second;
        attributes[attr_name] = attr_value;
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
    else if (strict_mode_)
        throw mime_error("Bad media type.");
    return media_type;
}


void mime::strip_body()
{
    while (!parsed_body_.empty())
        if (parsed_body_.back().empty())
            parsed_body_.pop_back();
        else
            break;
}

} // namespace mailio
