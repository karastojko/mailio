/*

binary.hpp
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
Binary codec.
**/
class MAILIO_EXPORT binary : public codec
{
public:

    /**
    Setting the encoder and decoder line policy.

    @param encoder_line_policy Encoder line length policy to set.
    @param decoder_line_policy Decoder line length policy to set.
    **/
    binary(codec::line_len_policy_t encoder_line_policy = codec::line_len_policy_t::NONE,
           codec::line_len_policy_t decoder_line_policy = codec::line_len_policy_t::NONE);

    binary(const binary&) = delete;

    binary(binary&&) = delete;

    /**
    Default destructor.
    **/
    ~binary() = default;

    void operator=(const binary&) = delete;

    void operator=(binary&&) = delete;

    /**
    Encoding a string into vector of binary encoded strings.

    @param text String to encode.
    @return     Vector with binary encoded strings.
    **/
    std::vector<std::string> encode(const std::string& text) const;

    /**
    Decoding a vector of binary encoded strings.

    @param text Vector of binary encoded strings.
    @return     Decoded string.
    @todo       Line policy to be verified.
    **/
    std::string decode(const std::vector<std::string>& text) const;

private:
};


} // namespace mailio
