/*

q_codec.hpp
-----------

Copyright (C) 2017, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include <string>
#include <vector>
#include <tuple>
#include "codec.hpp"
#include "export.hpp"


namespace mailio
{


/**
Q codec.

ASCII and UTF-8 charsets are recognized.
**/
class MAILIO_EXPORT q_codec : public codec
{
public:

    /**
    Setting the encoder and decoder line policies.

    @param line1_policy First line policy to set.
    @param lines_policy Other lines policy than the first one to set.
    @param codec_method Method for encoding/decoding.
    @throw codec_error  Bad encoding method.
    **/
    q_codec(std::string::size_type line1_policy, std::string::size_type lines_policy);

    q_codec(const q_codec&) = delete;

    q_codec(q_codec&&) = delete;

    /**
    Default destructor.
    **/
    ~q_codec() = default;

    void operator=(const q_codec&) = delete;

    void operator=(q_codec&&) = delete;

    /**
    Encoding a text by applying the given method.

    @param text    String to encode.
    @param charset Charset used by the string.
    @param method  Allowed encoding methods.
    @return        Encoded string.
    @todo          Merge text and charset into a single parameter of type `string_t`.
    @todo          It must take another parameter for the header name length in order to remove the hardcoded constant.
    **/
    std::vector<std::string> encode(const std::string& text, const std::string& charset, codec_t method) const;

    /**
    Decoding a string.

    @param text        String to decode.
    @return            Decoded string, its charset and its codec method.
    @throw codec_error Missing Q codec separator for charset.
    @throw codec_error Missing Q codec separator for codec type.
    @throw codec_error Missing last Q codec separator.
    @throw codec_error Bad encoding method.
    @throw *           `decode_qp(const string&)`, `base64::decode(const string&)`.
    **/
    std::tuple<std::string, std::string, codec_t> decode(const std::string& text) const;

    /**
    Checking if a string is Q encoded and decodes it.

    @param text        String to decode.
    @return            Decoded string, its charset and its codec method.
    @throw codec_error Bad Q codec format.
    @todo              Returning value to hold `string_t` instead of two `std::string`.
    **/
    std::tuple<std::string, std::string, codec_t> check_decode(const std::string& text) const;

private:

    /**
    String representation of Base64 method.
    **/
    static const std::string BASE64_CODEC_STR;

    /**
    String representation of Quoted Printable method.
    **/
    static const std::string QP_CODEC_STR;

    /**
    Decoding by using variation of the Quoted Printable method.

    @param text String to decode.
    @return     Decoded string.
    @throw *    `quoted_printable::decode(const vector<string>&)`
    **/
    std::string decode_qp(const std::string& text) const;

    /**
    Checking if a character is allowed.

    @param ch Character to check.
    @return   True if allowed, false if not.
    **/
    bool is_q_allowed(char ch) const;
};


} // namespace mailio


#ifdef _MSC_VER
#pragma warning(pop)
#endif
