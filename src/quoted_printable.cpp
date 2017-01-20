/*

quoted_printable.cpp
--------------------

Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <vector>
#include <stdexcept>
#include <boost/algorithm/string/trim.hpp>
#include <quoted_printable.hpp>
 

using std::string;
using std::vector;
using std::runtime_error;
using boost::trim_right;


namespace mailio
{


quoted_printable::quoted_printable(codec::line_len_policy_t line_policy) : codec(line_policy)
{
}


vector<string> quoted_printable::encode(const string& text) const
{
    vector<string> enc_text;
    string line;
    string::size_type line_len = 0;
    for (auto ch = text.begin(); ch != text.end(); ch++)
    {
        if (*ch > SPACE_CHAR && *ch <= TILDE_CHAR && *ch != EQUAL_CHAR)
        {
            // add soft break
            if (line_len >= string::size_type(_line_policy) - 3)
            {
                line += EQUAL_CHAR;
                enc_text.push_back(line);
                line.clear();
                line_len = 0;
            }
            line += *ch;
            line_len++;
        }
        else if (*ch == SPACE_CHAR)
        {
            // add soft break after the current space character
            if (line_len == string::size_type(_line_policy) - 4)
            {
                line += SPACE_CHAR;
                line += EQUAL_CHAR;
                enc_text.push_back(line);
                line.clear();
                line_len = 0;
            }
            // add soft break before the current space character
            else if (line_len == string::size_type(_line_policy) - 3)
            {
                line += EQUAL_CHAR;
                enc_text.push_back(line);
                line.clear();
                line += SPACE_CHAR;
                line_len = 1;
                
            }
            else
            {
                line += SPACE_CHAR;
                line_len++;
            }
        }
        else if (*ch == CR_CHAR)
        {
            if (ch + 1 == text.end() || (ch + 1 != text.end() && *(ch + 1) != LF_CHAR))
                throw codec_error("Bad crlf sequence.");
            enc_text.push_back(line);
            line.clear();
            line_len = 0;
            // two characters have to be skipped
            ch++;
        }
        else
        {
            // add soft break before the current character
            if (line_len >= string::size_type(_line_policy) - 5)
            {
                line += EQUAL_CHAR;
                enc_text.push_back(line);
                line.clear();
                line_len = 0;
            }
            line += EQUAL_CHAR;
            line += HEX_DIGITS[((*ch >> 4) & 0x0F)];
            line += HEX_DIGITS[(*ch & 0x0F)];
            line_len += 3;
        }
    }
    if (!line.empty())
        enc_text.push_back(line);
    while (!enc_text.empty() && enc_text.back().empty())
        enc_text.pop_back();
    
    return enc_text;
}


string quoted_printable::decode(const vector<string>& text) const
{
    string dec_text;
    for (const auto& line : text)
    {
        if (line.length() > string::size_type(_line_policy) - 2)
            throw codec_error("Bad line policy.");

        bool soft_break = false;
        for (string::const_iterator ch = line.begin(); ch != line.end(); ch++)
        {
            if (!is_allowed(*ch))
                throw codec_error("Bad character.");

            if (*ch == EQUAL_CHAR)
            {
                if ((ch + 1) == line.end())
                {
                    soft_break = true;
                    continue;
                }
                
                char next_char = *(ch + 1);
                char next_next_char = *(ch + 2);
                if (!is_allowed(next_char) || !is_allowed(next_next_char))
                    throw codec_error("Bad character.");
    
                if (HEX_DIGITS.find(next_char) == string::npos || HEX_DIGITS.find(next_next_char) == string::npos)
                    throw codec_error("Bad hexadecimal digit.");
                int nc_val = hex_digit_to_int(next_char);
                int nnc_val = hex_digit_to_int(next_next_char);
                dec_text += ((nc_val << 4) + nnc_val);
                ch += 2;
            }
            else
                dec_text += *ch;            
        }
        if (!soft_break)
            dec_text += CRLF;
    }
    trim_right(dec_text);
    
    return dec_text;
}


bool quoted_printable::is_allowed(char ch) const
{
    return ((ch >= SPACE_CHAR && ch <= TILDE_CHAR) || ch == '\t');
}


} // namespace mailio
