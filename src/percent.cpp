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
#include <boost/algorithm/string.hpp>
#include <mailio/percent.hpp>


using std::string;
using std::stringstream;
using std::vector;
using boost::to_upper_copy;


namespace mailio
{


percent::percent(string::size_type line1_policy, string::size_type lines_policy) :
    codec(line1_policy, lines_policy)
{
}


vector<string> percent::encode(const string& txt, const string& charset) const
{
    vector<string> enc_text;
    string line;
    string::size_type line_len = 0;
    // Soon as the first line is added, switch the policy to the other lines policy.
    string::size_type policy = line1_policy_;

    stringstream enc_line;
    enc_line << to_upper_copy(charset) + ATTRIBUTE_CHARSET_SEPARATOR_STR + ATTRIBUTE_CHARSET_SEPARATOR_STR;
    for (string::const_iterator ch = txt.begin(); ch != txt.end(); ch++)
    {
        if (isalnum(*ch))
        {
            enc_line << *ch;
            line_len++;
        }
        else
        {
            enc_line << codec::PERCENT_HEX_FLAG << std::setfill('0') << std::hex << std::uppercase << std::setw(2) <<
                static_cast<unsigned int>(static_cast<uint8_t>(*ch));
            line_len += 3;
        }

        if (line_len >= policy - 3)
        {
            enc_text.push_back(enc_line.str());
            enc_line.str("");
            line_len = 0;
            policy = lines_policy_;
        }
    }
    enc_text.push_back(enc_line.str());

    return enc_text;
}


string percent::decode(const string& txt) const
{
    string dec_text;
    for (string::const_iterator ch = txt.begin(); ch != txt.end(); ch++)
    {
        if (*ch == codec::PERCENT_HEX_FLAG)
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
