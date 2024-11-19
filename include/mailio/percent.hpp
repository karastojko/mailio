/*

percent.hpp
-----------

Copyright (C) 2024, Tomislav Karastojkovic (http://www.alepho.com).

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
Percent encoding and decoding as described in RFC 2231 section 4.

@todo Line policies not implemented.
**/
class MAILIO_EXPORT percent : public codec
{
public:

    /**
    Setting the encoder and decoder line policies.

    @param line1_policy First line policy to set.
    @param lines_policy Other lines policy than the first one to set.
    **/
    percent(std::string::size_type line1_policy, std::string::size_type lines_policy);

    percent(const percent&) = delete;

    percent(percent&&) = delete;

    /**
    Default destructor.
    **/
    ~percent() = default;

    void operator=(const percent&) = delete;

    void operator=(percent&&) = delete;

    /**
    Encoding a string.

    @param txt String to encode.
    @return    Encoded string.
    @todo      Implement the line policies.
    @todo      Replace `txt` to be `string_t`, then no need for the charset parameter.
    **/
    std::vector<std::string> encode(const std::string& txt, const std::string& charset) const;

    /**
    Decoding a percent encoded string.

    @param txt String to decode.
    @return    Decoded string.
    @todo      Implement the line policies.
    **/
    std::string decode(const std::string& txt) const;
};


} // namespace mailio
