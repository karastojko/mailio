/*

percent.cpp
-----------

Copyright (C) 2024, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <sstream>
#include <iomanip>
#include <mailio/percent.hpp>


using std::string;
using std::stringstream;


namespace mailio
{


// TODO: `codec` constructor is to satisfy compiler. Once these policies are removed, no need to set them.
percent::percent(string::size_type line1_policy, string::size_type lines_policy) :
    codec(line_len_policy_t::RECOMMENDED, line_len_policy_t::RECOMMENDED),
    line1_policy_(line1_policy), lines_policy_(lines_policy)
{
}


string percent::encode(const string& txt) const
{
    stringstream enc_text;
    for (string::const_iterator ch = txt.begin(); ch != txt.end(); ch++)
        if (isalnum(*ch))
            enc_text << *ch;
        else
            enc_text << codec::PERCENT_CHAR << std::setfill('0') << std::hex << std::uppercase << std::setw(2) <<
                static_cast<unsigned int>(static_cast<uint8_t>(*ch));
    return enc_text.str();
}


string percent::decode(const string& txt) const
{
    string dec_text;
    for (string::const_iterator ch = txt.begin(); ch != txt.end(); ch++)
    {
        if (*ch == codec::PERCENT_CHAR)
        {
            if (ch + 1 == txt.end() || ch + 2 == txt.end())
                throw codec_error("Bad character.");
            if (std::isxdigit(*(ch + 1)) == 0 || std::isxdigit(*(ch + 2)) == 0)
                throw codec_error("Bad character.");

            char next_char = toupper(*(ch + 1));
            char next_next_char = toupper(*(ch + 2));
            int nc_val = codec::hex_digit_to_int(next_char);
            int nnc_val = codec::hex_digit_to_int(next_next_char);
            dec_text += ((nc_val << 4) + nnc_val);
            ch += 2;
        }
        else
            dec_text += *ch;
    }
    return dec_text;
}


} // namespace mailio
