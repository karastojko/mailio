/*

codec.cpp
---------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <algorithm>
#include <string>
#include <mailio/codec.hpp>


using std::string;


namespace mailio
{

const string codec::HEX_DIGITS{"0123456789ABCDEF"};
const string codec::ASTERISK_STR(1, codec::ASTERISK_CHAR);
const string codec::END_OF_LINE{"\r\n"};
const string codec::END_OF_MESSAGE{"."};
const string codec::EQUAL_STR(1, codec::EQUAL_CHAR);
const string codec::SPACE_STR(1, codec::SPACE_CHAR);
const string codec::DOT_STR(1, codec::DOT_CHAR);
const string codec::COMMA_STR(1, codec::COMMA_CHAR);
const string codec::COLON_STR(1, codec::COLON_CHAR);
const string codec::SEMICOLON_STR(1, codec::SEMICOLON_CHAR);
const string codec::QUOTE_STR(1, codec::QUOTE_CHAR);
const string codec::LESS_THAN_STR(1, codec::LESS_THAN_CHAR);
const string codec::GREATER_THAN_STR(1, codec::GREATER_THAN_CHAR);
const string codec::CHARSET_ASCII("ASCII");
const string codec::CHARSET_UTF8("UTF-8");


int codec::hex_digit_to_int(char digit)
{
    return digit >= ZERO_CHAR && digit <= NINE_CHAR ? digit - ZERO_CHAR : digit - A_CHAR + 10;
}


bool codec::is_8bit_char(char ch)
{
    return static_cast<unsigned>(ch) > 127;
}


bool codec::is_utf8_string(const string& txt)
{
    for (auto ch : txt)
        if (static_cast<unsigned>(ch) > 127)
            return true;
    return false;
}


string codec::escape_string(const string& text, const string& escaping_chars)
{
    string esc_str;
    esc_str.reserve(text.size());
    std::for_each(text.begin(), text.end(), [&](char ch) {
        if (escaping_chars.find(ch) != string::npos)
            esc_str += "\\";
        esc_str += ch;
    });
    return esc_str;
}


    string codec::surround_string(const string& text, char surround_char)
{
    return surround_char + text + surround_char;
}


codec::codec(line_len_policy_t encoder_line_policy, line_len_policy_t decoder_line_policy)
  : line_policy_(encoder_line_policy), decoder_line_policy_(decoder_line_policy), strict_mode_(false)
{
}


void codec::strict_mode(bool mode)
{
    strict_mode_ = mode;
}


bool codec::strict_mode() const
{
    return strict_mode_;
}


} // namespace mailio
