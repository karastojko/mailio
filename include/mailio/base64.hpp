/*

base64.hpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
#include <vector>
#include "codec.hpp"
#include "export.hpp"


namespace mailio
{


/**
Base64 codec.

@todo Add static method `string encode(string)` to be used by `smtp`?
**/
class MAILIO_EXPORT base64 : public codec
{
public:

    /**
    Base64 character set.
    **/
    static const std::string CHARSET;
    
    /**
    Setting the encoder and decoder line policy.

    @param encoder_line_policy Encoder line length policy to set.
    @param decoder_line_policy Decoder line length policy to set.
    **/
    base64(codec::line_len_policy_t encoder_line_policy = codec::line_len_policy_t::NONE,
           codec::line_len_policy_t decoder_line_policy = codec::line_len_policy_t::NONE);

    base64(const base64&) = delete;

    base64(base64&&) = delete;

    /**
    Default destructor.
    **/
    ~base64() = default;

    void operator=(const base64&) = delete;

    void operator=(base64&&) = delete;

    /**
    Encoding a string into vector of Base64 encoded strings by applying the line policy.

    @param text     String to encode.
    @param reserved Number of characters to subtract from the line policy.
    @return         Vector of Base64 encoded strings.
    **/
    std::vector<std::string> encode(const std::string& text, std::string::size_type reserved = 0) const;

    /**
    Decoding a vector of Base64 encoded strings to string by applying the line policy.

    @param text        Vector of Base64 encoded strings.
    @return            Decoded string.
    @throw codec_error Bad character.
    @todo              Line policy not verified.
    **/
    std::string decode(const std::vector<std::string>& text) const;

    /**
    Decoding a Base64 string to a string.

    @param text Base64 encoded string.
    @return     Encoded string.
    @throw *    `decode(const std::vector<std::string>&)`.
    **/
    std::string decode(const std::string& text) const;

private:

    /**
    Checking if the given character is in the base64 character set.

    @param ch Character to check.
    @return   True if it is, false if not.
    **/
    bool is_allowed(char ch) const;
};


} // namespace mailio
