/*

percent.hpp
-----------

Copyright (C) 2024, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
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
    **/
    std::string encode(const std::string& txt) const;

    /**
    Decoding a percent encoded string.

    @param txt String to decode.
    @return    Decoded string.
    @todo      Implement the line policies.
    **/
    std::string decode(const std::string& txt) const;

private:

    std::string::size_type line1_policy_;

    std::string::size_type lines_policy_;
};


} // namespace mailio
