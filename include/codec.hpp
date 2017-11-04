/*

codec.hpp
-----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
#include <stdexcept>


namespace mailio
{


/**
Base class for codecs, contains various constants and miscellaneous functions for encoding/decoding purposes.

@todo `encode()` and `decode()` as abstract methods?
**/
class codec
{
public:

    /**
    Calculating value of the given hex digit.
    **/
    static int hex_digit_to_int(char digit);

    /**
    Checking if a string is UTF-8 encoded.

    @param txt String to check.
    @return    True if it's UTF-8, false if not.
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
    Space character.
    **/
    static const char SPACE_CHAR = ' ';

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
    Comma character.
    **/
    static const char COMMA_CHAR = ',';
    
    /**
    Colon character.
    **/
    static const char COLON_CHAR = ':';
    
    /**
    Semicolon character.
    */
    static const char SEMICOLON_CHAR = ';';

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
    Greater than character.
    **/
    static const char GREATER_THAN_CHAR = '>';

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
    static const std::string CRLF;

    /**
    Line length policy.

    @todo No need for `SUBJECT` line policy.
    **/
    enum class line_len_policy_t : std::string::size_type {NONE = 2048, RECOMMENDED = 78, MANDATORY = 998};

    /**
    Setting the line policy of the codec.

    @param line_policy Line policy to set.
    **/
    codec(line_len_policy_t line_policy);

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
    Line length policy.
    **/
    line_len_policy_t _line_policy;

    /**
    Strict mode for encoding/decoding.
    **/
    bool _strict_mode;
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


} // namespace
