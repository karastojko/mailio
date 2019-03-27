/*

base64.cpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string> 
#include <boost/algorithm/string/trim.hpp>
#include <mailio/base64.hpp>


using std::string;
using std::vector;
using boost::trim_right;


namespace mailio
{


const string base64::CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


base64::base64(codec::line_len_policy_t encoder_line_policy, codec::line_len_policy_t decoder_line_policy)
  : codec(encoder_line_policy, decoder_line_policy)
{
}


vector<string> base64::encode(const string& text, string::size_type reserved) const
{
    vector<string> enc_text;
    unsigned char group_8bit[3];
    unsigned char group_6bit[4];
    int count_3_chars = 0;
    string line;
    string::size_type line_len = 0;

    for (string::size_type cur_char = 0; cur_char < text.length(); cur_char++)
    {
        group_8bit[count_3_chars++] = text[cur_char];
        if (count_3_chars == 3) 
        {
            group_6bit[0] = (group_8bit[0] & 0xfc) >> 2;
            group_6bit[1] = ((group_8bit[0] & 0x03) << 4) + ((group_8bit[1] & 0xf0) >> 4);
            group_6bit[2] = ((group_8bit[1] & 0x0f) << 2) + ((group_8bit[2] & 0xc0) >> 6);
            group_6bit[3] = group_8bit[2] & 0x3f;

            for(int i = 0; i < 4; i++)
                line += CHARSET[group_6bit[i]];
            count_3_chars = 0;
            line_len += 4;
        }
        
        if (line_len >= string::size_type(_line_policy) - reserved - 2)
        {
            enc_text.push_back(line);
            line.clear();
            line_len = 0;
        }
    }

    // encode remaining characters if any

    if (count_3_chars > 0)
    {
        for (int i = count_3_chars; i < 3; i++)
            group_8bit[i] = '\0';

        group_6bit[0] = (group_8bit[0] & 0xfc) >> 2;
        group_6bit[1] = ((group_8bit[0] & 0x03) << 4) + ((group_8bit[1] & 0xf0) >> 4);
        group_6bit[2] = ((group_8bit[1] & 0x0f) << 2) + ((group_8bit[2] & 0xc0) >> 6);
        group_6bit[3] = group_8bit[2] & 0x3f;

        for (int i = 0; i < count_3_chars + 1; i++)
        {
            if (line_len >= string::size_type(_line_policy) - reserved - 2)
            {
                enc_text.push_back(line);
                line.clear();
                line_len = 0;
            }
            line += CHARSET[group_6bit[i]];
            line_len++;
        }

        while (count_3_chars++ < 3)
        {
            if (line_len >= string::size_type(_line_policy) - reserved - 2)
            {
                enc_text.push_back(line);
                line.clear();
                line_len = 0;
            }            
            line += EQUAL_CHAR;
            line_len++;
        }
    }
    
    if (!line.empty())
        enc_text.push_back(line);

    return enc_text;
}


string base64::decode(const vector<string>& text) const
{
    string dec_text;
    unsigned char group_6bit[4];
    unsigned char group_8bit[3];
    int count_4_chars = 0;

    for (const auto& line : text)
    {
        if (line.length() > string::size_type(_decoder_line_policy) - 2)
            throw codec_error("Bad line policy.");

        for (string::size_type ch = 0; ch < line.length() && line[ch] != EQUAL_CHAR; ch++)
        {
            if (!is_allowed(line[ch]))
                throw codec_error("Bad character `" + string(1, line[ch]) + "`.");
    
            group_6bit[count_4_chars++] = line[ch];
            if (count_4_chars == 4)
            {
                for (int i = 0; i < 4; i++)
                    group_6bit[i] = static_cast<unsigned char>(CHARSET.find(group_6bit[i]));
    
                group_8bit[0] = (group_6bit[0] << 2) + ((group_6bit[1] & 0x30) >> 4);
                group_8bit[1] = ((group_6bit[1] & 0xf) << 4) + ((group_6bit[2] & 0x3c) >> 2);
                group_8bit[2] = ((group_6bit[2] & 0x3) << 6) + group_6bit[3];
    
                for (int i = 0; i < 3; i++)
                    dec_text += group_8bit[i];
                count_4_chars = 0;
            }
        }
    
        // decode remaining characters if any
    
        if (count_4_chars > 0) 
        {
            for (int i = count_4_chars; i < 4; i++)
                group_6bit[i] = '\0';
    
            for (int i = 0; i < 4; i++)
                group_6bit[i] = static_cast<unsigned char>(CHARSET.find(group_6bit[i]));
    
            group_8bit[0] = (group_6bit[0] << 2) + ((group_6bit[1] & 0x30) >> 4);
            group_8bit[1] = ((group_6bit[1] & 0xf) << 4) + ((group_6bit[2] & 0x3c) >> 2);
            group_8bit[2] = ((group_6bit[2] & 0x3) << 6) + group_6bit[3];
    
            for (int i = 0; i < count_4_chars - 1; i++)
                dec_text += group_8bit[i];
        }
    }
    
    return dec_text;
}


string base64::decode(const string& text) const
{
    vector<string> v;
    v.push_back(text);
    return decode(v);
}


bool base64::is_allowed(char ch) const
{
    return (isalnum(ch) || ch == PLUS_CHAR || ch == SLASH_CHAR);
}


} // namespace mailio
