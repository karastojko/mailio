/*

binary.hpp
----------

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
Binary codec.
**/
class binary : public codec
{
public:

    /**
    Does nothing.

    @param line_policy Not used.
    **/
    binary(codec::line_len_policy_t line_policy = codec::line_len_policy_t::NONE);

    binary(const binary&) = delete;

    binary(binary&&) = delete;

    /**
    Calls parent destructor.
    **/
    ~binary() = default;

    void operator=(const binary&) = delete;

    void operator=(binary&&) = delete;

    /**
    Encodes string into vector of one binary encoded string.

    @param text String to encode.
    @return     Vector with one binary encoded string.
    **/
    std::vector<std::string> encode(const std::string& text) const;

    /**
    Decodes vector of binary encoded strings.

    @param text Vector of binary encoded strings.
    @return     Decoded string.
    **/
    std::string decode(const std::vector<std::string>& text) const;

private:
};


} // namespace mailio
