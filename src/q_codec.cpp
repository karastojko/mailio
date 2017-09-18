/*

q_codec.cpp
-----------

Copyright (C) 2017, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <boost/algorithm/string.hpp>
#include <base64.hpp>
#include <quoted_printable.hpp>
#include <q_codec.hpp>


using boost::iequals;
using std::string;
using std::vector;
using boost::to_upper_copy;


namespace mailio
{


const string q_codec::BASE64_CODEC_STR = "B";

const string q_codec::QP_CODEC_STR = "Q";


q_codec::q_codec(codec::line_len_policy_t line_policy, codec_method_t codec_method) :
    codec(line_policy), _codec_method(codec_method)
{
}


vector<string> q_codec::encode(const string& text) const
{
    vector<string> enc_text, text_c;
    string codec_flag;
    if (_codec_method == codec_method_t::BASE64)
    {
        codec_flag = BASE64_CODEC_STR;
        base64 b64(_line_policy);
        text_c = b64.encode(text);
    }
    else
    {
        codec_flag = QP_CODEC_STR;
        quoted_printable qp(_line_policy);
        qp.q_codec_mode(true);
        text_c = qp.encode(text);
    }

    for (auto s = text_c.begin(); s != text_c.end(); s++)
        enc_text.push_back("=?UTF-8?" + codec_flag + "?" + *s + "?=");

    return enc_text;
}


// TODO: returning charset info?
string q_codec::decode(const string& text) const
{
    string::size_type charset_pos = text.find(QUESTION_MARK_CHAR);
    if (charset_pos == string::npos)
        throw codec_error("Missing Q codec separator for charset.");
    string::size_type method_pos = text.find(QUESTION_MARK_CHAR, charset_pos + 1);
    if (method_pos == string::npos)
        throw codec_error("Missing Q codec separator for codec type.");
    string charset = text.substr(charset_pos + 1, method_pos - charset_pos - 1);
    if (charset.empty())
        throw codec_error("Missing Q codec charset.");
    string::size_type content_pos = text.find(QUESTION_MARK_CHAR, method_pos + 1);
    if (content_pos == string::npos)
        throw codec_error("Missing last Q codec separator.");
    string method = text.substr(method_pos + 1, content_pos - method_pos - 1);
    string text_c = text.substr(content_pos + 1);

    string dec_text;
    if (iequals(method, BASE64_CODEC_STR))
    {
        base64 b64(_line_policy);
        dec_text = b64.decode(text_c);
    }
    else if (iequals(method, QP_CODEC_STR))
    {
        dec_text = decode_qp(text_c);
    }
    else
        throw codec_error("Bad encoding method.");

    return dec_text;
}


string q_codec::decode_qp(const string& text) const
{
    quoted_printable qp(_line_policy);
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
