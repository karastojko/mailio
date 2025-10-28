/*

codec.hpp
---------

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
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include "export.hpp"


namespace mailio
{


/**
Base class for codecs, contains various constants and miscellaneous functions for encoding/decoding purposes.

@todo `encode()` and `decode()` as abstract methods?
**/
class MAILIO_EXPORT codec
{
public:

    /**
    Calculating value of the given hex digit.
    **/
    static int hex_digit_to_int(char digit);

    /**
    Checking if a character is eight bit.

    @param ch Character to check.
    @return   True if eight bit, false if seven bit.
    **/
    static bool is_8bit_char(char ch);

    /**
    Escaping the specified characters in the given string.

    @param text           String where to escape certain characters.
    @param escaping_chars Characters to be escaped.
    @return               The given string with the escaped characters.
    **/
    static std::string escape_string(const std::string& text, const std::string& escaping_chars);

    /**
    Surrounding the given string with the given character.

    @param text          String to surround.
    @param surround_char Character to be used for the surrounding.
    @return              Surrounded string.
    **/
    static std::string surround_string(const std::string& text, char surround_char = '"');

    /**
    Checking if a string is eight bit encoded.

    @param txt String to check.
    @return    True if it's eight bit, false if not.
    **/
    static bool is_utf8_string(const std::string& txt);

    /**
    Nil character.
    **/
    static const char NIL_CHAR = '\0';

    /**
    Carriage return character.
    **/
    static const char CR_CHAR = '\r';

    /**
    Line feed character.
    **/
    static const char LF_CHAR = '\n';

    /**
    Plus character.
    **/
    static const char PLUS_CHAR = '+';

    /**
    Minus character.
    **/
    static const char MINUS_CHAR = '-';

    /**
    Percent character.
    **/
    static const char PERCENT_HEX_FLAG = '%';

    /**
    Slash character.
    **/
    static const char SLASH_CHAR = '/';

    /**
    Backslash character.
    **/
    static const char BACKSLASH_CHAR = '\\';

    /**
    Equal character.
    **/
    static const char EQUAL_CHAR = '=';

    /**
    Equal character as string.
    **/
    static const std::string EQUAL_STR;

    /**
    Space character.
    **/
    static const char SPACE_CHAR = ' ';

    /**
    Space character as string.
    **/
    static const std::string SPACE_STR;

    /**
    Exclamation mark character.
    **/
    static const char EXCLAMATION_CHAR = '!';

    /**
    Question mark character.
    **/
    static const char QUESTION_MARK_CHAR = '?';

    /**
    Dot character.
    **/
    static const char DOT_CHAR = '.';

    /**
    Dot character string.
    **/
    static const std::string DOT_STR;

    /**
    Comma character.
    **/
    static const char COMMA_CHAR = ',';

    /**
    Comma character as string.
    **/
    static const std::string COMMA_STR;

    /**
    Colon character.
    **/
    static const char COLON_CHAR = ':';

    /**
    Colon character as string.
    **/
    static const std::string COLON_STR;

    /**
    Semicolon character.
    **/
    static const char SEMICOLON_CHAR = ';';

    /**
    Semicolon character as string.
    **/
    static const std::string SEMICOLON_STR;

    /**
    Zero number character.
    **/
    static const char ZERO_CHAR = '0';

    /**
    Nine number character.
    **/
    static const char NINE_CHAR = '9';

    /**
    Letter A character.
    **/
    static const char A_CHAR = 'A';

    /**
    Tilde character.
    **/
    static const char TILDE_CHAR = '~';

    /**
    Quote character.
    **/
    static const char QUOTE_CHAR = '"';

    /**
    Quote character as string.
    **/
    static const std::string QUOTE_STR;

    /**
    Left parenthesis character.
    **/
    static const char LEFT_PARENTHESIS_CHAR = '(';

    /**
    Right parenthesis character.
    **/
    static const char RIGHT_PARENTHESIS_CHAR = ')';

    /**
    Left bracket chartacter.
    **/
    static const char LEFT_BRACKET_CHAR = '[';

    /**
    Right bracket chartacter.
    **/
    static const char RIGHT_BRACKET_CHAR = ']';

    /**
    Left brace character.
    **/
    static const char LEFT_BRACE_CHAR = '{';

    /**
    Right brace character.
    **/
    static const char RIGHT_BRACE_CHAR = '}';

    /**
    Monkey character.
    **/
    static const char MONKEY_CHAR = '@';

    /**
    Less than character.
    **/
    static const char LESS_THAN_CHAR = '<';

    /**
    Less than character as string.
    **/
    static const std::string LESS_THAN_STR;

    /**
    Greater than character.
    **/
    static const char GREATER_THAN_CHAR = '>';

    /**
    Greater than character as string.
    **/
    static const std::string GREATER_THAN_STR;

    /**
    Underscore character.
    **/
    static const char UNDERSCORE_CHAR = '_';

    /**
    Hexadecimal alphabet.
    **/
    static const std::string HEX_DIGITS;

    /**
    Carriage return plus line feed string.
    **/
    static const std::string END_OF_LINE;

    /**
    Dot character is the end of message for SMTP.
    **/
    static const std::string END_OF_MESSAGE;

    /**
    ASCII charset label.
    **/
    static const std::string CHARSET_ASCII;

    /**
    UTF-8 charset label.
    **/
    static const std::string CHARSET_UTF8;

    /**
    Attribute indicator for the charset and language parameters.
    **/
    static const char ATTRIBUTE_CHARSET_SEPARATOR{'\''};

    /**
    Attribute indicator for the charset and language parameters as string.
    **/
    static const std::string ATTRIBUTE_CHARSET_SEPARATOR_STR;

    /**
    Line length policy.
    **/
    enum class line_len_policy_t : std::string::size_type {RECOMMENDED = 78, MANDATORY = 998, NONE = UINT_MAX,
        VERYLARGE [[deprecated]] = 16384};

    /**
    Methods used for the MIME header encoding/decoding.
    **/
    enum class codec_t {ASCII, BASE64, QUOTED_PRINTABLE, UTF8, PERCENT};

    /**
    Setting the encoder and decoder line policies.

    @param line1_policy First line policy to set.
    @param lines_policy Other lines policy than the first one to set.
    **/
    codec(std::string::size_type line1_policy, std::string::size_type lines_policy);

    codec(const codec&) = delete;

    codec(codec&&) = delete;

    /**
    Default destructor.
    **/
    virtual ~codec() = default;

    void operator=(const codec&) = delete;

    void operator=(codec&&) = delete;

    /**
    Enabling/disabling the strict mode.

    @param mode True to enable strict mode, false to disable.
    **/
    void strict_mode(bool mode);

    /**
    Returning the strict mode status.

    @return True if strict mode enabled, false if disabled.
    **/
    bool strict_mode() const;

protected:

    /**
    Policy applied for encoding of the first line.
    **/
    std::string::size_type line1_policy_;

    /**
    Policy applied for encoding of the lines other than first one, and for decoding of all lines including the first one.
    **/
    std::string::size_type lines_policy_;

    /**
    Strict mode for encoding/decoding.
    **/
    bool strict_mode_;
};


/**
Error thrown by codecs.
**/
class codec_error : public std::runtime_error
{
public:

    /**
    Calling parent constructor.

    @param msg Error message.
    **/
    explicit codec_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calling parent constructor.

    @param msg Error message.
    **/
    explicit codec_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


/**
String which contains charset together with the representation.
**/
template<typename Buf, typename Char>
struct String
{
    /**
    String content.
    **/
    Buf buffer;


    /**
    String charset.
    **/
    std::string charset;


    /**
    String codec.
    **/
    codec::codec_t codec_type;

    /**
    Default constructor.
    **/
    String() : buffer(), charset(codec::CHARSET_ASCII), codec_type(codec::codec_t::ASCII)
    {
    }


    /**
    Default copy constructor.
    **/
    String(const String&) = default;


    /**
    Default move constructor.
    **/
    String(String&&) = default;


    /**
    Initializing of the buffer and charset.

    @param buffer_s  Content of the string.
    @param charset_s Charset of the string.
    @param codec_s   Codec of the string.
    **/
    String(const Buf& buffer_s, const std::string& charset_s = codec::CHARSET_ASCII, codec::codec_t codec_s = codec::codec_t::ASCII) :
        buffer(buffer_s), charset(boost::to_upper_copy(charset_s)), codec_type(codec_s)
    {
    }


    /**
    Initializing of the buffer with the string literal.

    @param str       String literal.
    @param charset_s Charset of the string.
    @param codec_s   Codec of the string.
    **/
    String(const Char* str, const std::string& charset_s = codec::CHARSET_ASCII, codec::codec_t codec_s = codec::codec_t::ASCII) :
        String(Buf(str), charset_s, codec_s)
    {
    }


    /**
    Default copy assignment.
    **/
    String& operator=(const String& other) = default;


    /**
    Default move assignment.
    **/
    String& operator=(String&& other) = default;


    /**
    Conversion to the buffer type.
    **/
    operator Buf() const
    {
        return buffer;
    }


    bool empty() const
    {
        return buffer.empty();
    }
};


/**
Output stream standard insert operator.

@param os  Output stream to insert into.
@oaram str String to insert.
@return    The output stream itself.
**/
template<typename Buf, typename Char>
std::ostream& operator<<(std::ostream& os, const String<Buf, Char>& str)
{
    return os << str.buffer;
}


using string_t = String<std::string, char>;
#if defined(__cpp_char8_t)
using u8string_t = String<std::u8string, char8_t>;
#endif


// String operators.


/**
Deals only with the buffers. The left character set is taken, the right is ignored.

@param lhs First string to add.
@param rhs Second string to add.
@result    Concatenated given strings.
**/
template<typename Buf, typename Char>
String<Buf, Char> operator+(const String<Buf, Char>& lhs, const String<Buf, Char>& rhs)
{
    String<Buf, Char> result;
    result.buffer = lhs.buffer + rhs.buffer;
    result.charset = lhs.charset;
    return result;
}


/**
Deals only with the buffers. The left character set is taken, the right is ignored.

@param lhs String to be added to.
@param rhs String to add.
@result    Second string concatenated to the first one.
**/
template<typename Buf, typename Char>
String<Buf, Char>& operator+=(String<Buf, Char>& lhs, const String<Buf, Char>& rhs)
{
    lhs.buffer += rhs.buffer;
    return lhs;
}


/**
Checking whether the strings are equal by the content and charset.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if they are equal, false if not.
**/
template<typename Buf, typename Char>
bool operator==(const String<Buf, Char>& lhs, const String<Buf, Char>& rhs)
{
    return lhs.buffer == rhs.buffer && lhs.charset == rhs.charset && lhs.codec_type == rhs.codec_type;
}


/**
Checking whether the strings are not equal by the content or charset.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if they are not equal, false if they are.
**/
template<typename Buf, typename Char>
bool operator!=(const String<Buf, Char>& lhs, const String<Buf, Char>& rhs)
{
    return !operator==(lhs, rhs);
}


/**
Checking whether the first string is less than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is less than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator<(const String<Buf, Char>& lhs, const String<Buf, Char>& rhs)
{
    return lhs.buffer < rhs.buffer;
}


/**
Checking whether the first string is greater than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is greater than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator>(const String<Buf, Char>& lhs, const String<Buf, Char>& rhs)
{
    return operator<(rhs, lhs);
}


/**
Checking whether the first string is less or equal than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is less or equal than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator<=(const String<Buf, Char>& lhs, const String<Buf, Char>& rhs)
{
    return !operator>(rhs, lhs);
}


/**
Checking whether the first string is greater or equal than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is greater or equal than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator>=(const String<Buf, Char>& lhs, const String<Buf, Char>& rhs)
{
    return !operator<(rhs, lhs);
}


// String and std::string.


/**
Deals only with the buffers. The left character set is taken.

@param lhs First string to add.
@param rhs Second string to add.
@result    Concatenated given strings.
**/
template<typename Buf, typename Char>
String<Buf, Char> operator+(const String<Buf, Char>& lhs, const std::string& rhs)
{
    String<Buf, Char> result;
    result.buffer = lhs.buffer + rhs;
    result.charset = lhs.charset;
    return result;
}


/**
Deals only with the buffers. The left character set is taken.

@param lhs String to be added to.
@param rhs String to add.
@result    Second string concatenated to the first one.
**/
template<typename Buf, typename Char>
String<Buf, Char>& operator+=(String<Buf, Char>& lhs, const std::string& rhs)
{
    lhs.buffer += rhs;
    return lhs;
}


/**
Checking whether the strings are equal by the content.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if they are equal, false if not.
**/
template<typename Buf, typename Char>
bool operator==(const String<Buf, Char>& lhs, const std::string& rhs)
{
    return lhs.buffer == rhs;
}


/**
Checking whether the strings are not equal by the content.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if they are not equal, false if they are.
**/
template<typename Buf, typename Char>
bool operator!=(const String<Buf, Char>& lhs, const std::string& rhs)
{
    return !operator==(lhs, rhs);
}


/**
Checking whether the first string is less than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is less than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator<(const String<Buf, Char>& lhs, const std::string& rhs)
{
    return lhs.buffer < rhs;
}


/**
Checking whether the first string is greater than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is greater than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator>(const String<Buf, Char>& lhs, const std::string& rhs)
{
    return lhs.buffer > rhs;
}


/**
Checking whether the first string is less or equal than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is less or equal than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator<=(const String<Buf, Char>& lhs, const std::string& rhs)
{
    return !operator>(rhs, lhs);
}


/**
Checking whether the first string is greater or equal than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is greater or equal than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator>=(const String<Buf, Char>& lhs, const std::string& rhs)
{
    return !operator<(rhs, lhs);
}


#if defined(__cpp_char8_t)

// String and std::u8string.


/**
Deals only with the buffers. The left character set is taken.

@param lhs First string to add.
@param rhs Second string to add.
@result    Concatenated given strings.
**/
template<typename Buf, typename Char>
String<Buf, Char> operator+(const String<Buf, Char>& lhs, const std::u8string& rhs)
{
    String<Buf, Char> result;
    result.buffer = lhs.buffer + rhs;
    result.charset = lhs.charset;
    return result;
}


/**
Deals only with the buffers. The left character set is taken.

@param lhs String to be added to.
@param rhs String to add.
@result    Second string concatenated to the first one.
**/
template<typename Buf, typename Char>
String<Buf, Char>& operator+=(String<Buf, Char>& lhs, const std::u8string& rhs)
{
    lhs.buffer += rhs;
    return lhs;
}


/**
Checking whether the strings are equal by the content.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if they are equal, false if not.
**/
template<typename Buf, typename Char>
bool operator==(const String<Buf, Char>& lhs, const std::u8string& rhs)
{
    return lhs.buffer == rhs;
}


/**
Checking whether the strings are not equal by the content.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if they are not equal, false if they are.
**/
template<typename Buf, typename Char>
bool operator!=(const String<Buf, Char>& lhs, const std::u8string& rhs)
{
    return !operator==(lhs, rhs);
}


/**
Checking whether the first string is less than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is less than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator<(const String<Buf, Char>& lhs, const std::u8string& rhs)
{
    return lhs.buffer < rhs;
}


/**
Checking whether the first string is greater than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is greater than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator>(const String<Buf, Char>& lhs, const std::u8string& rhs)
{
    return lhs.buffer > rhs;
}


/**
Checking whether the first string is less or equal than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is less or equal than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator<=(const String<Buf, Char>& lhs, const std::u8string& rhs)
{
    return !operator>(rhs, lhs);
}


/**
Checking whether the first string is greater or equal than the second one.

@param lhs First string to compare.
@param rhs Second string to compare.
@return    True if the first one is greater or equal than the second one, false otherwise.
**/
template<typename Buf, typename Char>
bool operator>=(const String<Buf, Char>& lhs, const std::u8string& rhs)
{
    return !operator<(rhs, lhs);
}

#endif


} // namespace


#ifdef _MSC_VER
#pragma warning(pop)
#endif
