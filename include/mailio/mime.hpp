/*

mime.hpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <map>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include "codec.hpp"
#include "export.hpp"


namespace mailio
{


/*
To parse elements of a mime part the following alphabet is used as described in [rfc 5322]:
```
atext = ALPHA / DIGIT / "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "/" / "=" / "?" / "~" / "_" / "`" / "{" / "|" / "}" / "~"
atom = [CFWS] 1 * atext [CFWS]
dot-atom-text = 1 * atext * ("." 1 * atext)
dot-atom = [CFWS] dot-atom-text [CFWS]
quoted-pair = ("\SLASH" (VCHAR / WSP))
comment = "(" *([FWS] ctext / quoted-pair) [FWS] ")"
CFWS = (1 * ([FWS] comment) [FWS]) / FWS
qtext = \%d33 / \%d35-\%d91 / \%d93-\%d126 ; printable ascii without backslash and quote
qcontent = qtext / quoted-pair
quoted-string = [CFWS] DQUOTE * ([FWS] qcontent) [FWS] DQUOTE [CFWS]
word = atom / quoted-string
phrase = 1 * word
dtext = \%d33-\%d90 / \%d94-\%d126 ; printable ascii not including brackets and backslash
address = [phrase] angle-addr / addr-spec
addr-spec = (dot-atom / quoted-string) "@" (dot-atom / dtext)
```
*/

/**
Mime part implementation.
**/
class MAILIO_EXPORT mime
{
public:

    /**
    `Content-ID` header name.
    **/
    static const std::string CONTENT_ID_HEADER;

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
    Top level media types.

    @todo Mixed subtype is missing.
    **/
    enum class media_type_t {NONE, TEXT, IMAGE, AUDIO, VIDEO, APPLICATION, MULTIPART, MESSAGE};

    /**
    Content type.
    **/
    struct MAILIO_EXPORT content_type_t
    {
        /**
        Charset attribute name.
        **/
        static const std::string ATTR_CHARSET;

        /**
        Boundary attribute name.
        **/
        static const std::string ATTR_BOUNDARY;

        /**
        Media type attribute.
        **/
        media_type_t type;

        /**
        Media subtype attribute.
        **/
        std::string subtype;

        /**
        Charset attribute.
        **/
        std::string charset;

        /**
        Initializing the media type to none, subtype and charset to empty strings.
        **/
        content_type_t();

        /**
        Copy constructor is default.
        **/
        content_type_t(const content_type_t&) = default;

        /**
        Initializing the content type with the given media type and subtype.

        @param media_type      Media type to set.
        @param media_subtype   Media subtype to set.
        @param content_charset Charset to set.
        **/
        content_type_t(media_type_t media_type, const std::string& media_subtype, const std::string& content_charset = "");

        /**
        Assignment operator.

        @param cont_type Value to set.
        @return          Object itself.
        **/
        content_type_t& operator=(const content_type_t& cont_type);
    };

    /**
    Content transfer encodings.

    @todo What is a differrence between ASCII and 7bit?
    **/
    enum class content_transfer_encoding_t {NONE, BIT_7, BIT_8, BASE_64, QUOTED_PRINTABLE, BINARY};

    /**
    Content disposition.

    @todo Content disposition should be struct to keep this enum and filename?
    **/
    enum class content_disposition_t {NONE, INLINE, ATTACHMENT};

    /**
    Default default constructor.
    **/
    mime();

    /**
    Default copy constructor.
    **/
    mime(const mime&) = default;

    /**
    Default move constructor.
    **/
    mime(mime&&) = default;

    /**
    Default destructor.
    **/
    virtual ~mime() = default;

    /**
    Default copy assignment operator.
    **/
    mime& operator=(const mime&) = default;

    /**
    Default move assignment operator.
    **/
    mime& operator=(mime&&) = default;

    /**
    Formatting the mime part to a string.

    If a line contains leading dot, then it can be escaped as required by mail protocols.

    @param message_str String to store the message.
    @param dot_escape  Flag if the leading dot should be escaped.
    @throw mime_error  Formatting failure, non multipart message with boundary.
    @throw *           `format_header()`, `format_content(bool)`.
    **/
    void format(std::string& mime_str, bool dot_escape = true) const;

    /**
    Overload of `format(string&, bool)`.

    Because of the way the u8string is comverted to string, it's more expensive when used with C++20.
    **/
#if defined(__cpp_char8_t)
    void format(std::u8string& mime_str, bool dot_escape = true) const;
#endif

    /**
    Parsing the mime part from a string.

    If a line contains leading dot, then it can be escaped as required by mail protocols.

    Essentially, the method goes line by line and calls the parsing by line.

    @param mime_string String to parse.
    @param dot_escape  Flag if the leading dot should be escaped.
    @throw *           `parse_by_line(const std::string&, bool)`.
    **/
    void parse(const std::string& mime_string, bool dot_escape = false);

    /**
    Overload of `parse(const string&, bool)`.

    Because of the way the u8string is comverted to string, it's more expensive when used with C++20.
    **/
#if defined(__cpp_char8_t)
    void parse(const std::u8string& mime_string, bool dot_escape = false);
#endif

    /**
    Parsing a line of mime part.

    The method is used to parse message string line by line, as received over the protocols. For each line a CRLF is appended. If the line is
    CRLF only, then the parsing ends.

    If a message with mime parts is parsed but the message also contains a content, then the content is stored instead to be skipped.

    The method relies on the virtual `parse_header_line()` which should specify headers of interest to the class. The message class uses it for
    its own headers.

    @param line       String to be parsed. If it's CRLF, then message parsing ends; any further parsing is undefined.
    @param dot_escape Flag if the leading dot should be escaped.
    @return           Mime itself.
    @throw *          `parse_header()`, `parse_content`.
    @todo             Determine a default charset.
    **/
    mime& parse_by_line(const std::string& line, bool dot_escape = false);

    /**
    Setting the content type.

    @param cont_type  Content type to be set.
    @throw mime_error Bad content type.
    **/
    void content_type(const content_type_t& cont_type);

    /**
    Setting the content type.

    @param media_type    Media type to set.
    @param media_subtype Media subtype to set.
    @param charset       Charset to set.
    @throw *             `content_type(const content_type_t&)`.
    **/
    void content_type(media_type_t media_type, const std::string& media_subtype, const std::string& charset = "");

    /**
    Getting the content type.

    @return Content type.
    **/
    content_type_t content_type() const;

    /**
    Setting the content ID.

    @param id         The content ID in the format `string1@string2`.
    @throw mime_error Invalid content ID.
    **/
    void content_id(std::string id);

    /**
    Getting the content ID.

    @return Content ID.
    **/
    std::string content_id() const;

    /**
    Setting the mime name.

    @param mime_name Mime name to set.
    **/
    void name(const std::string& mime_name);

    /**
    Getting the mime name.

    @return Mime name.
    **/
    std::string name() const;

    /**
    Setting the content transfer encoding.

    @param encoding Content transfer encoding to be set.
    **/
    void content_transfer_encoding(content_transfer_encoding_t encoding);

    /**
    Getting the content transfer encoding.

    @return Content transfer encoding.
    **/
    content_transfer_encoding_t content_transfer_encoding() const;

    /**
    Setting the content disposition.

    @param disposition Content disposition to set.
    **/
    void content_disposition(content_disposition_t disposition);

    /**
    Getting the content disposition.

    @return Content disposition.
    **/
    content_disposition_t content_disposition() const;

    /**
    Setting the boundary of the mime part.

    @param bound String to be used as the boundary.
    **/
    void boundary(const std::string& bound);

    /**
    Getting the boundary of the mime part.

    @return Boundary of the mime part.
    **/
    std::string boundary() const;

    /**
    Setting the content given as string.

    @param content_str Content to set.
    **/
    void content(const std::string& content_str);

#if defined(__cpp_char8_t)
    void content(const std::u8string& content_str);
#endif

    /**
    Getting the content as a string.

    @return Content as string.
    **/
    std::string content() const;

    /**
    Adding a mime part.

    @param part Mime part to be added.
    **/
    void add_part(const mime& part);

    /**
    Returning the mime parts.

    @return Vector of mime parts.
    **/
    std::vector<mime> parts() const;

    /**
    Setting the message decoding and encoding line policy.

    @param encoder_line_policy Encoder line policy to set.
    @param decoder_line_policy Decoder line policy to set.
    **/
    void line_policy(codec::line_len_policy_t encoder_line_policy, codec::line_len_policy_t decoder_line_policy);

    /**
    Getting the encoder message line policy.

    @return Encoder line policy.
    **/
    codec::line_len_policy_t line_policy() const;

    /**
    Getting the message decoder line policy.

    @return Decoder line policy.
    **/
    codec::line_len_policy_t decoder_line_policy() const;

    /**
    Enabling/disabling the strict mode for the mime part.

    @param mode True to enable strict mode, false to disable.
    **/
    void strict_mode(bool mode);

    /**
    Returning the strict mode status of the mime part.

    @return True if strict mode enabled, false if disabled.
    **/
    bool strict_mode() const;

    /**
    Enabling/disabling the strict mode for codecs.

    @param mode True to enable strict mode, false to disable.
    **/
    void strict_codec_mode(bool mode);

    /**
    Returning the strict mode status of codecs.

    @return True if strict mode enabled, false if disabled.
    **/
    bool strict_codec_mode() const;

    using header_codec_t = codec::header_codec_t;

    /**
    Setting the headers codec.

    @param hdr_codec Codec to set.
    **/
    void header_codec(header_codec_t hdr_codec);

    /**
    Getting the headers codec.

    @return Codec set.
    **/
    header_codec_t header_codec() const;

protected:

    /**
    Comparator for the attributes map based on case insensitivity.
    **/
    struct attr_comp_t : public std::less<std::string>
    {
        bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            return boost::to_lower_copy(lhs) < boost::to_lower_copy(rhs);
        }
    };

    /**
    Attributes map which used the appropriate comparator.
    **/
    typedef std::map<std::string, std::string, attr_comp_t> attributes_t;

    /**
    Content type header name.
    **/
    static const std::string CONTENT_TYPE_HEADER;

    /**
    Content transfer encoding header name.
    **/
    static const std::string CONTENT_TRANSFER_ENCODING_HEADER;

    /**
    Content transfer encoding base64 string.
    **/
    static const std::string CONTENT_TRANSFER_ENCODING_BASE64;

    /**
    Content transfer encoding seven bit string.
    **/
    static const std::string CONTENT_TRANSFER_ENCODING_BIT7;

    /**
    Content transfer encoding eight bit string.
    **/
    static const std::string CONTENT_TRANSFER_ENCODING_BIT8;

    /**
    Content transfer encoding quoted printable string.
    **/
    static const std::string CONTENT_TRANSFER_ENCODING_QUOTED_PRINTABLE;

    /**
    Content transfer encoding binary string.
    **/
    static const std::string CONTENT_TRANSFER_ENCODING_BINARY;

    /**
    Content disposition header name.
    **/
    static const std::string CONTENT_DISPOSITION_HEADER;

    /**
    Content disposition attachment string.
    **/
    static const std::string CONTENT_DISPOSITION_ATTACHMENT;

    /**
    Content disposition inline string.
    **/
    static const std::string CONTENT_DISPOSITION_INLINE;

    /**
    Indentation used by headers for the continuation.
    **/
    static const std::string NEW_LINE_INDENT;

    /**
    Content type and subtype separator.
    **/
    static const char CONTENT_SUBTYPE_SEPARATOR{'/'};

    /**
    Header name and value separator character.
    **/
    static const char HEADER_SEPARATOR_CHAR{':'};

    /**
    Colon string used to separate header name from the header value.
    **/
    static const std::string HEADER_SEPARATOR_STR;

    /**
    Attribute name and value separator.
    **/
    static const char NAME_VALUE_SEPARATOR_CHAR{'='};

    /**
    Attribute name and value separator as string.
    **/
    static const std::string NAME_VALUE_SEPARATOR_STR;

    /**
    Separator of multiple attributes.
    **/
    static const char ATTRIBUTES_SEPARATOR_CHAR{';'};

    /**
    Semicolon string used to separate header attributes.
    **/
    static const std::string ATTRIBUTES_SEPARATOR_STR;

    /**
    Attribute indicator for the parameter continuation.
    **/
    static const char ATTRIBUTE_MULTIPLE_NAME_INDICATOR{'*'};

    /**
    Attribute name part.
    **/
    static const std::string ATTRIBUTE_NAME;

    /**
    Attribute filename part.
    **/
    static const std::string ATTRIBUTE_FILENAME;

    /**
    Boundary special characters.
    **/
    static const std::string BOUNDARY_DELIMITER;

    /**
    Alphanumerics plus some special character allowed in the quoted text.
    **/
    static const std::string QTEXT;

    /**
    Allowed characters for the header name. They consist of the printable ASCII characters without the colon.
    **/
    static const boost::regex HEADER_NAME_REGEX;

    /**
    Allowed characters for the header value. They consist of the printable ASCII characters with the space.
    **/
    static const boost::regex HEADER_VALUE_REGEX;

    /**
    Content type attribute value allowed characters.
    **/
    static const std::string CONTENT_ATTR_ALPHABET;

    /**
    Content header value allowed characters.
    */
    static const std::string CONTENT_HEADER_VALUE_ALPHABET;

    /**
    Regex for the message id in the strict mode.
    **/
    static const std::string MESSAGE_ID_REGEX;

    /**
    Regex for the message id in the non-strict mode.
    **/
    static const std::string MESSAGE_ID_REGEX_NS;

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

    @param ids        String of IDs within the angle brackets.
    @return           Vector of IDs.
    @throw mime_error Parsing failure of the message ID.
    **/
    std::vector<std::string> parse_many_ids(const std::string& ids) const;

    /**
    Formatting header.

    @return Mime header as string.
    **/
    virtual std::string format_header() const;

    /**
    Formatting content by using the codec.

    @param dot_escape Flag if leading dots in lines should be escaped.
    @return           Formatted content.
    @throw *          `bit7::encode(const string&)`, `bit8::encode(const string&)`, `base64::encode(const string&)`, `quoted_printable::encode(const string&)`.
    **/
    std::string format_content(bool dot_escape) const;

    /**
    Formatting content type to a string.

    @return Content type as string.
    **/
    std::string format_content_type() const;

    /**
    Formatting transfer encoding to a string.

    @return Transfer encoding as string.
    **/
    std::string format_transfer_encoding() const;

    /**
    Formatting content disposition to a string.

    @return Content disposition as string.
    **/
    std::string format_content_disposition() const;

    /**
    Formating content ID.

    @return Content ID header.
    **/
    std::string format_content_id() const;

    /**
    Formats mime name.

    The name has to fit to mandatory line policy, otherwise the rest is truncated.

    @param name Mime name to format.
    @return     Formatted name.
    **/
    std::string format_mime_name(const std::string& name) const;

    /**
    Parsing header by going through header lines and calling `parse_header_line()`.

    @throw * `parse_header_line(const string&)`.
    **/
    void parse_header();

    /**
    Parsing the content by using the appropriate codec.

    @throw * `bit7::decode(const string&)`, `bit8::decode(const string&)`, `base64::decode(const string&)`, `quoted_printable::decode(const string&)`.
    **/
    void parse_content();

    /**
    Parsing a header line for a specific header.

    @param header_line Header line to be parsed.
    @throw *           `parse_header_name_value(const string&, string&, string&)`,
                       `parse_content_type(const string&, media_type_t&, string&, map<string, string>&)`,
                       `parse_content_transfer_encoding(const string&, content_transfer_encoding_t& encoding, map<string, string>&)`,
                       `parse_content_disposition(const string&, content_disposition_t& disposition, map<string, string>&)`.
    **/
    virtual void parse_header_line(const std::string& header_line);

    /**
    Parsing a header for the name and value.

    @param header_line  Header to parse.
    @param header_name  Header name parsed.
    @param header_value Header value parsed.
    @throw mime_error   Parsing failure of header name.
    @throw mime_error   Parsing failure of header line.
    @throw mime_error   Parsing failure, header name or value empty.
    **/
    void parse_header_name_value(const std::string& header_line, std::string& header_name, std::string& header_value) const;

    /**
    Continued attribute parameters are merged into a single attribute parameter, the others remain as they are.

    @param attributes Attribute parameters where the merging is to be done.
    @throw mime_error Parsing attribute failure.
    **/
    void merge_attributes(attributes_t& attributes) const;

    /**
    Parsing the content type and its attributes.

    @param content_type_hdr Content type header line to be parsed.
    @param media_type       Media type parsed from the header.
    @param media_subtype    Media subtype parsed from the header.
    @param attributes       Attributes parsed from the header.
    @throw mime_error       Parsing content type value failure.
    @throw *                `parse_header_value_attributes(const string&, string&, map<string, string>&)`, `mime_type_as_enum(const string&)`.
    **/
    void parse_content_type(const std::string& content_type_hdr, media_type_t& media_type, std::string& media_subtype, attributes_t& attributes) const;

    /**
    Parsing the content transfer encoding value and attributes.

    @param transfer_encoding_hdr Content transfer encoding header without name
    @param encoding              Content transfer encoding value parsed.
    @param attributes            Content transfer encoding attributes parsed in the the key/value format.
    @throw mime_error            Parsing content transfer encoding failure.
    @throw *                     `parse_header_value_attributes(const string&, string&, map<string, string>&)`.
    **/
    void parse_content_transfer_encoding(const std::string& transfer_encoding_hdr, content_transfer_encoding_t& encoding, attributes_t& attributes) const;

    /**
    Parsing the content disposition value and attributes.

    @param content_disp_hdr Content disposition header without name.
    @param disposition      Content disposition value parsed.
    @param attributes       Content disposition attributes parsed in the the key/value format.
    @throw mime_error       Parsing content disposition failure.
    @throw *                `parse_header_value_attributes(const string&, string&, map<string, string>&)`.
    **/
    void parse_content_disposition(const std::string& content_disp_hdr, content_disposition_t& disposition, attributes_t& attributes) const;

    /**
    Parsing value and attributes of the so called content headers.

    @param header     Header (without name) to be parsed.
    @param value      Header value parsed.
    @param attributes Header attributes parsed in the the key/value format.
    @throw mime_error Parsing header value failure.
    @throw mime_error Parsing attribute name failure.
    @throw mime_error Parsing attribute value failure.
    @todo             Allowed characters are more strict than required?
    **/
    void parse_header_value_attributes(const std::string& header, std::string& value, attributes_t& attributes) const;

    /**
    Creating the random boundary for the mime part.

    @return Boundary for the mime part.
    **/
    std::string make_boundary() const;

    /**
    Top level mime type represented as a string.

    @param media_type_val Mime type to be converted.
    @return               Mime type string representation.
    **/
    std::string mime_type_as_str(media_type_t media_type_val) const;

    /**
    Top level mime type string represented as the enum.

    @param media_type_val Mime type string to be converted.
    @return               Enum value of the mime type string.
    @throw mime_error     Bad media type.
    **/
    media_type_t mime_type_as_enum(const std::string& media_type_val) const;

    /**
    Removing trailing empty lines from the body.
    **/
    void strip_body();

    /**
    Boundary for the mime part.
    **/
    std::string boundary_;

    /**
    Mime version, should always be 1.0.
    **/
    std::string version_;

    /**
    Encoder line policy to be applied for the mime part.
    **/
    codec::line_len_policy_t line_policy_;

    /**
    Decoder line policy to be applied for the mime part.
    **/
    codec::line_len_policy_t decoder_line_policy_;

    /**
    Strict mode for mime part.
    **/
    bool strict_mode_;

    /**
    Strict mode for encoding/decoding.
    **/
    bool strict_codec_mode_;

    /**
    Codec used for headers.
    **/
    header_codec_t header_codec_;

    /**
    Content type as a pair of top level media type and media subtype.
    **/
    content_type_t content_type_;

    /**
    Name of mime.

    @todo Should it contain filename of the attachment?
    **/
    std::string name_;

    /**
    Content ID.
    **/
    std::string content_id_;

    /**
    Content transfer encoding of the mime part.
    **/
    content_transfer_encoding_t encoding_;

    /**
    Content disposition of the mime part.
    **/
    content_disposition_t disposition_;

    /**
    Raw representation of the content.

    The content is in the form as presented to the user, thus no limits such as line policy are applied here. For such purpose, the format method
    is used,
    **/
    std::string content_;

    /**
    Keeps containing mime parts, if any; otherwise, it's empty vector.
    **/
    std::vector<mime> parts_;

    /**
    Flag if the header is being parsed.
    **/
    bool parsing_header_;

    /**
    Representation of the header in lines.
    **/
    std::vector<std::string> parsed_headers_;

    /**
    Representation of the body in lines.
    **/
    std::vector<std::string> parsed_body_;

    /**
    Status of mime parsing - is parsing of the mime part started or stopped.
    **/
    enum class mime_parsing_status_t {NONE, BEGIN, END} mime_status_;
};


/**
Exception reported by `mime` class.
**/
class mime_error : public std::runtime_error
{
public:

    /**
    Calling parent constructor.

    @param msg Error message.
    **/
    explicit mime_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calling parent constructor.

    @param msg Error message.
    **/
    explicit mime_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


} // namespace mailio


#ifdef _MSC_VER
#pragma warning(pop)
#endif
