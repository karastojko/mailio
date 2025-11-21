/*

base64.hpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include <string>
#include <vector>
#include "codec.hpp"
#include "export.hpp"


namespace mailio
{


/**
Base64 codec.

@todo Add static method `string encode(string)` to be used by `smtp`?
@todo Does it need the first line policy?
**/
class MAILIO_EXPORT base64 : public codec
{
public:

    /**
    Base64 character set.
    **/
    static const std::string CHARSET;

    /**
    Setting the encoder and decoder line policies.

    Since Base64 encodes three characters into four, the split is made after each fourth character. It seems that email clients do not merge properly
    many lines of encoded text if the split is not grouped by four characters. For that reason, the constructor sets line policies to be divisible by
    the number four.

    @param line1_policy First line policy to set.
    @param lines_policy Other lines policy than the first one to set.
    **/
    base64(std::string::size_type line1_policy, std::string::size_type lines_policy);

    base64(const base64&) = delete;

    base64(base64&&) = delete;

    /**
    Default destructor.
    **/
    ~base64() = default;

    void operator=(const base64&) = delete;

    void operator=(base64&&) = delete;

    /**
    Encoding a string into vector of Base64 encoded strings by applying the line policy.

    @param text     String to encode.
    @return         Vector of Base64 encoded strings.
    **/
    std::vector<std::string> encode(const std::string& text) const;

    /**
    Decoding a vector of Base64 encoded strings to string by applying the line policy.

    @param text        Vector of Base64 encoded strings.
    @return            Decoded string.
    @throw codec_error Bad character.
    @todo              Line policy not verified.
    **/
    std::string decode(const std::vector<std::string>& text) const;

    /**
    Decoding a Base64 string to a string.

    @param text Base64 encoded string.
    @return     Encoded string.
    @throw *    `decode(const std::vector<std::string>&)`.
    **/
    std::string decode(const std::string& text) const;

private:

    /**
    Checking if the given character is in the base64 character set.

    @param ch Character to check.
    @return   True if it is, false if not.
    **/
    bool is_allowed(char ch) const;

	/**
	Number of six bit chunks.
	**/
	static constexpr unsigned short SEXTETS_NO = 4;

	/**
	Number of eight bit characters.
	**/
	static constexpr unsigned short OCTETS_NO = SEXTETS_NO - 1;
};


} // namespace mailio


#ifdef _MSC_VER
#pragma warning(pop)
#endif
