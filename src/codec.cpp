/*

codec.cpp
---------

Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <codec.hpp>


using std::string;


namespace mailio
{

/*
TODO:
-----

In order to support strict parsing mode (i.e. to allow more relaxed parsing rules), a strict flag should be added. Thus, specific codecs could decide
if they will allow violation of RFC. For instance, Quoted Printable may allow tab character, or Seven Bit may allow eight bit characters.
*/


const string codec::HEX_DIGITS = "0123456789ABCDEF";

const string codec::CRLF = "\r\n";


int codec::hex_digit_to_int(char digit)
{
    return digit >= ZERO_CHAR && digit <= NINE_CHAR ? digit - ZERO_CHAR : digit - A_CHAR + 10;
}


codec::codec(codec::line_len_policy_t line_policy) : _line_policy(line_policy)
{
}


} // namespace mailio
