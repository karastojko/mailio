/*

base64.hpp
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
Base64 codec.

@todo Add static method `string encode(string)` to be used by `smtp`?
**/
class base64 : public codec
{
public:

    /**
    Base64 character set.
    **/
    static const std::string CHARSET;
    
    /**
    Calls `codec::codec(line_len_policy_t)`.

    @param line_policy Line length policy to set.
    **/
    base64(codec::line_len_policy_t line_policy = codec::line_len_policy_t::NONE);

    base64(const base64&) = delete;

    base64(base64&&) = delete;

    /**
    Calls parent destructor.
    **/
    ~base64() = default;

    void operator=(const base64&) = delete;

    void operator=(base64&&) = delete;

    /**
    Encodes string into vector of base64 encoded strings by applying the line policy.

    @param text String to encode.
    @return     Vector of base64 encoded strings.
    **/
    std::vector<std::string> encode(const std::string& text) const;

    /**
    Decodes vector of Base64 encoded strings to string by applying the line policy.

    @param text        Vector of base64 encoded strings.
    @return            Decoded string.
    @throw codec_error Bad character.
    **/
    std::string decode(const std::vector<std::string>& text) const;

    /**
    Decodes Base64 string to a string.

    @param text Base64 encoded string.
    @return     Encoded string.
    **/
    std::string decode(const std::string& text) const;

private:

    /**
    Checks if the given character is in the base64 character set.

    @param ch Character to check.
    @return   True if it is, false if not.
    **/
    bool is_allowed(char ch) const;
};


} // namespace mailio
