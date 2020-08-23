/*

binary.cpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <vector>
#include <mailio/binary.hpp>


using std::string;
using std::vector;


namespace mailio
{


binary::binary(codec::line_len_policy_t encoder_line_policy, codec::line_len_policy_t decoder_line_policy)
  : codec(encoder_line_policy, decoder_line_policy)
{
}


vector<string> binary::encode(const string& text) const
{
    vector<string> enc_text;
    enc_text.push_back(text);
    return enc_text;
}


string binary::decode(const vector<string>& text) const
{
    string dec_text;
    for (const auto& line : text)
        dec_text += line + END_OF_LINE;
    return dec_text;
}


} // namespace mailio
