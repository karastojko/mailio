/*

quoted_printable.hpp
--------------------

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
Quoted Printable codec.
**/
class MAILIO_EXPORT quoted_printable : public codec
{
public:

    /**
    Setting the encoder and decoder line policy.

    @param encoder_line_policy Encoder line length policy to set.
    @param decoder_line_policy Decoder line length policy to set.
    **/
    quoted_printable(codec::line_len_policy_t encoder_line_policy = codec::line_len_policy_t::NONE,
                     codec::line_len_policy_t decoder_line_policy = codec::line_len_policy_t::NONE);

    quoted_printable(const quoted_printable&) = delete;

    quoted_printable(quoted_printable&&) = delete;

    /**
    Default destructor.
    **/
    ~quoted_printable() = default;

    void operator=(const quoted_printable&) = delete;

    void operator=(quoted_printable&&) = delete;

    /**
    Encoding a string into vector of quoted printable encoded strings by applying the line policy.

    @param text        String to encode.
    @param reserved    Number of characters to subtract from the line policy.
    @return            Vector of quoted printable strings.
    @throw codec_error Bad character.
    @throw codec_error Bad CRLF sequence.
    **/
    std::vector<std::string> encode(const std::string& text, std::string::size_type reserved = 0) const;

    /**
    Decoding a vector of quoted printable strings to string by applying the line policy.

    @param text        Vector of quoted printable encoded strings.
    @return            Decoded string.
    @throw codec_error Bad line policy.
    @throw codec_error Bad character.
    @throw codec_error Bad hexadecimal digit.
    **/
    std::string decode(const std::vector<std::string>& text) const;

    /**
    Setting Q codec mode.

    @param mode True to set, false to unset.
    **/
    void q_codec_mode(bool mode);

private:

    /**
    Check if a character is in the Quoted Printable character set.

    @param ch Character to check.
    @return   True if it is, false if not.
    **/
    bool is_allowed(char ch) const;

    /**
    Flag for Q codec mode.
    **/
    bool _q_codec_mode;
};


} // namespace mailio
