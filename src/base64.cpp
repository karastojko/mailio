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


base64::base64(string::size_type line1_policy, string::size_type lines_policy) :
    codec(line1_policy, lines_policy)
{
    // Line policies to be divisible by four.
    line1_policy_ -= line1_policy_ % SEXTETS_NO;
    lines_policy_ -= lines_policy_ % SEXTETS_NO;
}


vector<string> base64::encode(const string& text) const
{
    vector<string> enc_text;
    unsigned char octets[OCTETS_NO];
    unsigned char sextets[SEXTETS_NO];
    int sextets_counter = 0;
    string line;
    string::size_type line_len = 0;
    string::size_type policy = line1_policy_;

    auto add_new_line = [&enc_text, &line_len, &policy, this](string& line)
    {
        enc_text.push_back(line);
        line.clear();
        line_len = 0;
        policy = lines_policy_;
    };

    for (string::size_type cur_char = 0; cur_char < text.length(); cur_char++)
    {
        octets[sextets_counter++] = text[cur_char];
        if (sextets_counter == OCTETS_NO)
        {
            sextets[0] = (octets[0] & 0xfc) >> 2;
            sextets[1] = ((octets[0] & 0x03) << 4) + ((octets[1] & 0xf0) >> 4);
            sextets[2] = ((octets[1] & 0x0f) << 2) + ((octets[2] & 0xc0) >> 6);
            sextets[3] = octets[2] & 0x3f;

            for(int i = 0; i < SEXTETS_NO; i++)
                line += CHARSET[sextets[i]];
            sextets_counter = 0;
            line_len += SEXTETS_NO;
        }

        if (line_len >= policy)
            add_new_line(line);
    }

    // encode remaining characters if any

    if (sextets_counter > 0)
    {
        // If the remaining three characters match exatcly rest of the line, then move them onto next line. Email clients do not show properly subject when
        // the next line has the empty content, containing only the encoding stuff.
        if (line_len >= policy - OCTETS_NO)
            add_new_line(line);

        for (int i = sextets_counter; i < OCTETS_NO; i++)
            octets[i] = '\0';

        sextets[0] = (octets[0] & 0xfc) >> 2;
        sextets[1] = ((octets[0] & 0x03) << 4) + ((octets[1] & 0xf0) >> 4);
        sextets[2] = ((octets[1] & 0x0f) << 2) + ((octets[2] & 0xc0) >> 6);
        sextets[3] = octets[2] & 0x3f;

        for (int i = 0; i < sextets_counter + 1; i++)
        {
            if (line_len >= policy)
                add_new_line(line);
            line += CHARSET[sextets[i]];
            line_len++;
        }

        while (sextets_counter++ < OCTETS_NO)
        {
            if (line_len >= policy)
                add_new_line(line);
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
    unsigned char sextets[SEXTETS_NO];
    unsigned char octets[OCTETS_NO];
    int count_4_chars = 0;

    for (const auto& line : text)
    {
        if (line.length() > lines_policy_)
            throw codec_error("Bad line policy.");

        for (string::size_type ch = 0; ch < line.length() && line[ch] != EQUAL_CHAR; ch++)
        {
            if (!is_allowed(line[ch]))
                throw codec_error("Bad character `" + string(1, line[ch]) + "`.");

            sextets[count_4_chars++] = line[ch];
            if (count_4_chars == SEXTETS_NO)
            {
                for (int i = 0; i < SEXTETS_NO; i++)
                    sextets[i] = static_cast<unsigned char>(CHARSET.find(sextets[i]));

                octets[0] = (sextets[0] << 2) + ((sextets[1] & 0x30) >> 4);
                octets[1] = ((sextets[1] & 0xf) << 4) + ((sextets[2] & 0x3c) >> 2);
                octets[2] = ((sextets[2] & 0x3) << 6) + sextets[3];

                for (int i = 0; i < OCTETS_NO; i++)
                    dec_text += octets[i];
                count_4_chars = 0;
            }
        }

        // decode remaining characters if any

        if (count_4_chars > 0)
        {
            for (int i = count_4_chars; i < SEXTETS_NO; i++)
                sextets[i] = '\0';

            for (int i = 0; i < SEXTETS_NO; i++)
                sextets[i] = static_cast<unsigned char>(CHARSET.find(sextets[i]));

            octets[0] = (sextets[0] << 2) + ((sextets[1] & 0x30) >> 4);
            octets[1] = ((sextets[1] & 0xf) << 4) + ((sextets[2] & 0x3c) >> 2);
            octets[2] = ((sextets[2] & 0x3) << 6) + sextets[3];

            for (int i = 0; i < count_4_chars - 1; i++)
                dec_text += octets[i];
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
