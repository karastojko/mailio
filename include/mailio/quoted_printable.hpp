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

@todo Remove the Q codec flag.
**/
class MAILIO_EXPORT quoted_printable : public codec
{
public:

    /**
    Setting the encoder and decoder line policies.

    @param line1_policy First line policy to set.
    @param lines_policy Other lines policy than the first one to set.
    **/
    quoted_printable(std::string::size_type line1_policy, std::string::size_type lines_policy);

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
    @return            Vector of quoted printable strings.
    @throw codec_error Bad character.
    @throw codec_error Bad CRLF sequence.
    **/
    std::vector<std::string> encode(const std::string& text) const;

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
    Flag for the Q codec mode.
    **/
    bool q_codec_mode_;
};


} // namespace mailio
