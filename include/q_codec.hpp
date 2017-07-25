/*

q_codec.hpp
-----------

Copyright (C) Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
#include <vector>
#include "codec.hpp"


namespace mailio
{


/**
Q codec.
Recognizes ASCII and UTF-8 charsets.
**/
class q_codec : public codec
{
public:

    /**
    Method used for encoding/decoding.
    **/
    enum class codec_method_t {BASE64, QUOTED_PRINTABLE};

    /**
    Sets line policy to recommended.

    @param codec_method Method for encoding/decoding.
    **/
    q_codec(codec_method_t codec_method);

    q_codec(const q_codec&) = delete;

    q_codec(q_codec&&) = delete;

    /**
    Default destructor.
    **/
    ~q_codec() = default;

    void operator=(const q_codec&) = delete;

    void operator=(q_codec&&) = delete;

    /**
    Encodes a text by applying the given method.

    @param text String to encode.
    @return     Encoded string.
    **/
    std::vector<std::string> encode(const std::string& text) const;

    /**
    Decodes a string.

    @param text        String to decode.
    @return            Decoded string.
    @throw codec_error Bad encoding.
    **/
    std::string decode(const std::string& text) const;

private:

    /**
    String representation of base64 method.
    **/
    static const std::string BASE64_CODEC_STR;

    /**
    String representation of quoted printable method.
    **/
    static const std::string QP_CODEC_STR;

    /**
    Decoding by using variation of the quoted printable method.

    @param text String to decode.
    @return     Decoded string.
    **/
    std::string decode_qp(const std::string& text) const;

    /**
    Checking if a character is allowed.

    @param ch Character to check.
    @return   True if allowed, false if not.
    **/
    bool is_q_allowed(char ch) const;

    /**
    Method used for encoding/decoding.
    **/
    codec_method_t _codec_method;
};


} // namespace mailio
