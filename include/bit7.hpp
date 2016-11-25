/*

bit7.hpp
--------

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
Seven bit codec.
**/
class bit7 : public codec
{
public:

    /**
    Calls `codec::codec(line_len_policy_t)`.

    @param line_policy Line policy to set.
    **/
    bit7(codec::line_len_policy_t line_policy = codec::line_len_policy_t::NONE);

    bit7(const bit7&) = delete;

    bit7(bit7&&) = delete;

    /**
    Calls parent destructor.
    **/
    ~bit7() = default;

    void operator=(const bit7&) = delete;

    void operator=(bit7&&) = delete;

    /**
    Encodes string into vector of seven bit encoded strings by applying the line policy.

    @param text        String to encode.
    @return            Vector of seven bit encoded strings.
    @throw codec_error Bad character.
    **/
    std::vector<std::string> encode(const std::string& text) const;

    /**
    Decodes vector of seven bit encoded strings to string by applying the line policy.

    @param text        Vector of seven bit encoded strings.
    @return            Decoded string.
    @throw codec_error Bad line length.
    @throw codec_error Bad character.
    **/
    std::string decode(const std::vector<std::string>& text) const;

private:

    /**
    Checks if the character is in the seven bit character set.

    @param ch Character to check.
    @return   True if it is, false if not.
    **/
    bool is_allowed(char ch) const;
};


} // namespace mailio
