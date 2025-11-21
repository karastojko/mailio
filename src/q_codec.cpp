/*

q_codec.cpp
-----------

Copyright (C) 2017, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <boost/algorithm/string.hpp>
#include <mailio/base64.hpp>
#include <mailio/quoted_printable.hpp>
#include <mailio/q_codec.hpp>


using boost::iequals;
using std::string;
using std::vector;
using std::tuple;
using std::make_tuple;
using std::get;
using boost::to_upper_copy;


namespace mailio
{


const string q_codec::BASE64_CODEC_STR = "B";
const string q_codec::QP_CODEC_STR = "Q";


q_codec::q_codec(string::size_type line1_policy, string::size_type lines_policy) :
    codec(line1_policy, lines_policy)
{
}


vector<string> q_codec::encode(const string& text, const string& charset, codec_t method) const
{
    // TODO: The constant has to depend of the header length.
    const string::size_type Q_FLAGS_LEN = 12;
    vector<string> enc_text, text_c;
    string codec_flag;
    if (method == codec_t::BASE64)
    {
        codec_flag = BASE64_CODEC_STR;
        base64 b64(line1_policy_ - Q_FLAGS_LEN, lines_policy_ - Q_FLAGS_LEN);
        text_c = b64.encode(text);
    }
    else if (method == codec_t::QUOTED_PRINTABLE)
    {
        codec_flag = QP_CODEC_STR;
        quoted_printable qp(line1_policy_ - Q_FLAGS_LEN, lines_policy_ - Q_FLAGS_LEN);
        qp.q_codec_mode(true);
        text_c = qp.encode(text);
    }
    else
        throw codec_error("Bad encoding method.");

    // TODO: Name the magic constant for Q delimiters.
    for (auto s = text_c.begin(); s != text_c.end(); s++)
        enc_text.push_back("=?" + to_upper_copy(charset) + "?" + codec_flag + "?" + *s + "?=");

    return enc_text;
}


tuple<string, string, codec::codec_t> q_codec::decode(const string& text) const
{
    string::size_type charset_pos = text.find(QUESTION_MARK_CHAR);
    if (charset_pos == string::npos)
        throw codec_error("Missing Q codec separator for charset.");
    string::size_type method_pos = text.find(QUESTION_MARK_CHAR, charset_pos + 1);
    if (method_pos == string::npos)
        throw codec_error("Missing Q codec separator for codec type.");
    string charset = to_upper_copy(text.substr(charset_pos + 1, method_pos - charset_pos - 1));
    if (charset.empty())
        throw codec_error("Missing Q codec charset.");
    string::size_type content_pos = text.find(QUESTION_MARK_CHAR, method_pos + 1);
    if (content_pos == string::npos)
        throw codec_error("Missing last Q codec separator.");
    string method = text.substr(method_pos + 1, content_pos - method_pos - 1);
    codec_t method_type;
    string text_c = text.substr(content_pos + 1);

    string dec_text;
    if (iequals(method, BASE64_CODEC_STR))
    {
        base64 b64(line1_policy_, lines_policy_);
        dec_text = b64.decode(text_c);
        method_type = codec_t::BASE64;
    }
    else if (iequals(method, QP_CODEC_STR))
    {
        dec_text = decode_qp(text_c);
        method_type = codec_t::QUOTED_PRINTABLE;
    }
    else
        throw codec_error("Bad encoding method.");

    return make_tuple(dec_text, charset, method_type);
}


tuple<string, string, codec::codec_t> q_codec::check_decode(const string& text) const
{
    string::size_type question_mark_counter = 0;
    const string::size_type QUESTION_MARKS_NO = 4;
    bool is_encoded = false;
    string dec_text, encoded_part;
    string charset = CHARSET_ASCII;
    // If there is no q encoding, then it's ascii or utf8.
    codec_t method_type = codec_t::ASCII;

    for (auto ch = text.begin(); ch != text.end(); ch++)
    {
        if (*ch == codec::QUESTION_MARK_CHAR)
            ++question_mark_counter;

        if (*ch == codec::EQUAL_CHAR && ch + 1 != text.end() && *(ch + 1) == codec::QUESTION_MARK_CHAR && !is_encoded)
            is_encoded = true;
        else if (*ch == codec::QUESTION_MARK_CHAR && ch + 1 != text.end() && *(ch + 1) == codec::EQUAL_CHAR && question_mark_counter == QUESTION_MARKS_NO)
        {
            is_encoded = false;
            question_mark_counter = 0;
            auto text_charset = decode(encoded_part);
            dec_text += get<0>(text_charset);
            charset = get<1>(text_charset);
            method_type = get<2>(text_charset);

            encoded_part.clear();
            ch++;
        }
        else if (is_encoded == true)
            encoded_part.append(1, *ch);
        else
            dec_text.append(1, *ch);
    }

    if (is_encoded && question_mark_counter < QUESTION_MARKS_NO)
        throw codec_error("Bad Q codec format.");

    return make_tuple(dec_text, charset, method_type);
}


string q_codec::decode_qp(const string& text) const
{
    quoted_printable qp(line1_policy_, lines_policy_);
    qp.q_codec_mode(true);
    vector<string> lines;
    lines.push_back(text);
    return qp.decode(lines);
}


bool q_codec::is_q_allowed(char ch) const
{
    return (ch > SPACE_CHAR && ch <= TILDE_CHAR && ch != QUESTION_MARK_CHAR);
}


} // namespace mailio
