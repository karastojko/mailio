/*

bit8.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <vector>
#include <boost/algorithm/string/trim.hpp>
#include <mailio/bit8.hpp>


using std::string;
using std::vector;
using boost::trim_right;


namespace mailio
{


bit8::bit8(codec::line_len_policy_t encoder_line_policy, codec::line_len_policy_t decoder_line_policy)
  : codec(encoder_line_policy, decoder_line_policy)
{
}


vector<string> bit8::encode(const string& text) const
{
    vector<string> enc_text;
    string line;
    string::size_type line_len = 0;
    for (auto ch = text.begin(); ch != text.end(); ch++)
    {
        if (is_allowed(*ch))
        {
            line += *ch;
            line_len++;
        }
        else if (*ch == '\r' && (ch + 1) != text.end() && *(ch + 1) == '\n')
        {
            enc_text.push_back(line);
            line.clear();
            line_len = 0;
            // skip both crlf characters
            ch++;
        }
        else
            throw codec_error("Bad character `" + string(1, *ch) + "`.");
        
        if (line_len == string::size_type(_line_policy))
        {
            enc_text.push_back(line);
            line.clear();
            line_len = 0;
        }
    }
    if (!line.empty())
        enc_text.push_back(line);
    while (!enc_text.empty() && enc_text.back().empty())
        enc_text.pop_back();
    
    return enc_text;
}


string bit8::decode(const vector<string>& text) const
{
    string dec_text;
    for (const auto& line : text)
    {
        if (line.length() > string::size_type(_decoder_line_policy))
            throw codec_error("Line policy overflow.");
        
        for (auto ch : line)
        {
            if (!is_allowed(ch))
                throw codec_error("Bad character `" + string(1, ch) + "`.");

            dec_text += ch;
        }
        dec_text += "\r\n";
    }
    trim_right(dec_text);
    
    return dec_text;
}


/*
For details see [rfc 2045, section 2.8].
*/
bool bit8::is_allowed(char ch) const
{
    return (ch != NIL_CHAR && ch != CR_CHAR && ch != LF_CHAR);
}


} // namespace mailio
