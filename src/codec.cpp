/*

codec.cpp
---------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <codec.hpp>


using std::string;


namespace mailio
{

const string codec::HEX_DIGITS = "0123456789ABCDEF";

const string codec::CRLF = "\r\n";


int codec::hex_digit_to_int(char digit)
{
    return digit >= ZERO_CHAR && digit <= NINE_CHAR ? digit - ZERO_CHAR : digit - A_CHAR + 10;
}


bool codec::is_utf8_string(const string& txt)
{
    for (auto ch : txt)
        if (static_cast<unsigned>(ch) > 127)
            return true;
    return false;
}


codec::codec(codec::line_len_policy_t line_policy) : _line_policy(line_policy), _strict_mode(false)
{
}


void codec::strict_mode(bool mode)
{
    _strict_mode = mode;
}


bool codec::strict_mode() const
{
    return _strict_mode;
}


} // namespace mailio
