/*

bit7.hpp
--------

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
Seven bit codec.
**/
class MAILIO_EXPORT bit7 : public codec
{
public:

    /**
    Setting the encoder and decoder line policies.

    @param line1_policy First line policy to set.
    @param lines_policy Other lines policy than the first one to set.
    **/
    bit7(std::string::size_type line1_policy, std::string::size_type lines_policy);

    bit7(const bit7&) = delete;

    bit7(bit7&&) = delete;

    /**
    Default destructor.
    **/
    ~bit7() = default;

    void operator=(const bit7&) = delete;

    void operator=(bit7&&) = delete;

    /**
    Encoding a string into vector of 7bit encoded strings by applying the line policy.

    @param text        String to encode.
    @return            Vector of seven bit encoded strings.
    @throw codec_error Bad character.
    **/
    std::vector<std::string> encode(const std::string& text) const;

    /**
    Decoding a vector of 7bit encoded strings to string by applying the line policy.

    @param text        Vector of 7bit encoded strings.
    @return            Decoded string.
    @throw codec_error Line policy overflow.
    @throw codec_error Bad character.
    **/
    std::string decode(const std::vector<std::string>& text) const;

private:

    /**
    Checking if a character is in the 7bit character set.

    @param ch Character to check.
    @return   True if it is, false if not.
    **/
    bool is_allowed(char ch) const;
};


} // namespace mailio
