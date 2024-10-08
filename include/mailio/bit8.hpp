/*

bit8.hpp
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
Eight bit codec.
**/
class MAILIO_EXPORT bit8 : public codec
{
public:

    /**
    Setting the encoder and decoder line policies.

    @param line1_policy First line policy to set.
    @param lines_policy Other lines policy than the first one to set.
    **/
    bit8(std::string::size_type line1_policy, std::string::size_type lines_policy);

    bit8(const bit8&) = delete;

    bit8(bit8&&) = delete;

    /**
    Default destructor.
    **/
    ~bit8() = default;

    void operator=(const bit8&) = delete;

    void operator=(bit8&&) = delete;

    /**
    Encoding a string into vector of 8bit encoded strings by applying the line policy.

    @param text        String to encode.
    @return            Vector of eight bit encoded strings.
    @throw codec_error Bad character.
    **/
    std::vector<std::string> encode(const std::string& text) const;

    /**
    Decoding a vector of 8bit strings to string by applying the line policy.

    @param text        Vector of eight bit encoded strings.
    @return            Decoded string.
    @throw codec_error Line policy overflow.
    @throw codec_error Bad character.
    **/
    std::string decode(const std::vector<std::string>& text) const;

private:

    /**
    Checking if a character is in the 8bit character set.

    @param ch Character to check.
    @return   True if it is, false if not.
    **/
    bool is_allowed(char ch) const;
};


} // namespace mailio
