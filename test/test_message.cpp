/*

test_message.cpp
----------------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


//#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE message_test

#include <string>
#include <utility>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <mailio/mailboxes.hpp>
#include <mailio/message.hpp>


using std::string;
using boost::posix_time::ptime;
using boost::posix_time::time_from_string;
using boost::local_time::time_zone_ptr;
using boost::local_time::local_date_time;
using boost::local_time::posix_time_zone;
using mailio::string_t;
using mailio::codec;
using mailio::mail_address;
using mailio::mail_group;
using mailio::mime;
using mailio::message;
using mailio::mime_error;


#ifdef __cpp_char8_t
#define utf8_string std::u8string
#else
#define utf8_string std::string
#endif


/**
Verifying setters and getters for sender/reply/recipient addresses, message date, subject, content type, transfer encoding and short ASCII content.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_addresses)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev"));
    msg.add_recipient(mail_address("kontakt", "kontakt@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_group("all", {mail_address("Tomislav", "qwertyuiop@hotmail.com")}));
    msg.add_cc_recipient(mail_group("mailio", {mail_address("", "karas@mailio.dev"), mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev")}));
    msg.add_cc_recipient(mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev"));
    msg.add_cc_recipient(mail_address("Tomislav @ Karastojkovic", "qwertyuiop@gmail.com"));
    msg.add_cc_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_cc_recipient(mail_group("all", {mail_address("", "qwertyuiop@hotmail.com"), mail_address("Tomislav", "qwertyuiop@gmail.com"),
        mail_address("Tomislav @ Karastojkovic", "qwertyuiop@zoho.com")}));
    msg.add_bcc_recipient(mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev"));
    msg.add_bcc_recipient(mail_address("Tomislav @ Karastojkovic", "qwertyuiop@gmail.com"));
    msg.add_bcc_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("Hello, World!");
    msg.content("Hello, World!");
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);

    BOOST_CHECK(msg.from_to_string() == "mailio <adresa@mailio.dev>");
    BOOST_CHECK(msg.reply_address_to_string() == "Tomislav Karastojkovic <kontakt@mailio.dev>");
    BOOST_CHECK(msg.recipients_to_string() == "kontakt <kontakt@mailio.dev>,\r\n"
        "  mailio <adresa@mailio.dev>,\r\n"
        "  all: Tomislav <qwertyuiop@hotmail.com>;");
    BOOST_CHECK(msg.cc_recipients_to_string() == "Tomislav Karastojkovic <kontakt@mailio.dev>,\r\n"
        "  \"Tomislav @ Karastojkovic\" <qwertyuiop@gmail.com>,\r\n"
        "  mailio <adresa@mailio.dev>,\r\n"
        "  mailio: <karas@mailio.dev>,\r\n"
        "  Tomislav Karastojkovic <kontakt@mailio.dev>;\r\n"
        "  all: <qwertyuiop@hotmail.com>,\r\n"
        "  Tomislav <qwertyuiop@gmail.com>,\r\n"
        "  \"Tomislav @ Karastojkovic\" <qwertyuiop@zoho.com>;");
    BOOST_CHECK(msg.bcc_recipients_to_string() == "Tomislav Karastojkovic <kontakt@mailio.dev>,\r\n"
        "  \"Tomislav @ Karastojkovic\" <qwertyuiop@gmail.com>,\r\n"
        "  mailio <adresa@mailio.dev>");
    BOOST_CHECK(msg.date_time() == ldt);
    BOOST_CHECK(msg.content_type().type == mime::media_type_t::NONE && msg.content_type().subtype.empty() && msg.content_type().charset.empty());
    BOOST_CHECK(msg.content_transfer_encoding() == mime::content_transfer_encoding_t::NONE);
    BOOST_CHECK(msg.content() == "Hello, World!");
}


/**
Formatting other headers.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_other_headers)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("Hello, World!");
    msg.content("Hello, World!");
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.add_header("User-Agent", "mailio");
    msg.add_header("Content-Language", "en-US");
    string msg_str;
    msg.format(msg_str);

    BOOST_CHECK(msg_str == "Content-Language: en-US\r\n"
        "User-Agent: mailio\r\n"
        "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: Hello, World!\r\n"
        "\r\n"
        "Hello, World!\r\n");
    BOOST_CHECK(msg.headers().size() == 2);

    msg.remove_header("User-Agent");
    msg_str.clear();
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "Content-Language: en-US\r\n"
        "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: Hello, World!\r\n"
        "\r\n"
        "Hello, World!\r\n");
    BOOST_CHECK(msg.headers().size() == 1);
}


/**
Formatting multiline content with lines containing leading dots, with the escaping dot flag off.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_dotted_no_escape)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format dotted no escape");
    msg.content(".Hello, World!\r\n"
        "opa bato\r\n"
        "..proba\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "yaba.daba.doo.\r\n"
        "\r\n"
        "..\r\n"
        "\r\n");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: format dotted no escape\r\n"
        "\r\n"
        ".Hello, World!\r\n"
        "opa bato\r\n"
        "..proba\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "yaba.daba.doo.\r\n"
        "\r\n"
        "..\r\n");
}


/**
Formatting multiline content with lines containing leading dots, with the escaping dot flag on.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_dotted_escape)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format dotted escape");
    msg.content(".Hello, World!\r\n"
        "opa bato\r\n"
        "..proba\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "yaba.daba.doo.\r\n"
        "\r\n"
        "..\r\n"
        "\r\n");

    string msg_str;
    msg.format(msg_str, true);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: format dotted escape\r\n"
        "\r\n"
        "..Hello, World!\r\n"
        "opa bato\r\n"
        "...proba\r\n"
        "\r\n"
        "..\r\n"
        "\r\n"
        "yaba.daba.doo.\r\n"
        "\r\n"
        "...\r\n");
}


/**
Formatting long default content (which is text with ASCII charset) default encoded (which is Seven Bit) to lines with the recommended length.

Since content type and transfer encoding are default, no such headers are created.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_long_text_default_default)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format long text default default");
    msg.content("Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. Nije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n"
        "\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe, jer libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n"
        "\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 karaktera odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da je nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije. Ovaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici formatiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n\r\n\r\n");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: format long text default default\r\n"
        "\r\n"
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. N\r\n"
        "ije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe, je\r\n"
        "r libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n"
        "\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 karakte\r\n"
        "ra odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da je\r\n"
        " nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije. O\r\n"
        "vaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici formatiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n");
}


/**
Formatting long text default charset (which is ASCII) Base64 encoded to lines with the recommended length.

Since charset is default, it is not set in the content type header.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_long_text_default_base64)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format long text default base64");
    msg.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    msg.content_type(message::media_type_t::TEXT, "plain");
    msg.content("Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. Nije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n"
        "\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe, jer libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n"
        "\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 karaktera odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da je nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije. Ovaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici formatiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n\r\n\r\n");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "Subject: format long text default base64\r\n"
        "\r\n"
        "T3ZvIGplIGpha28gZHVnYWNoa2EgcG9ydWthIGtvamEgaW1hIGkgcHJhem5paCBsaW5pamEgaSBw\r\n"
        "cmVkdWdhY2hraWggbGluaWphLiBOaWplIGphc25vIGtha28gY2Ugc2UgdGVrc3QgcHJlbG9taXRp\r\n"
        "DQpwYSBzZSBuYWRhbSBkYSBjY2UgdG8gb3ZhaiB0ZXN0IHBva2F6YXRpLg0KDQpUcmViYSB2aWRl\r\n"
        "dGkga2FrbyBwb3puYXRpIG1lamwga2xpamVudGkgbG9tZSB0ZWtzdCwgcGEgbmENCm9zbm92dSB0\r\n"
        "b2dhIGRvcmFkaXRpIGZvcm1hdGlyYW5qZSBzYWRyenphamEgbWVqbGEuIEEgbW96emRhIGkgbmVt\r\n"
        "YSBwb3RyZWJlLCBqZXIgbGlibWFpbGlvIG5pamUgemFtaXNobGplbiBkYSBzZQ0KYmF2aSBmb3Jt\r\n"
        "YXRpcmFuamVtIHRla3N0YS4NCg0KDQpVIHN2YWtvbSBzbHVjaGFqdSwgcG9zbGUgcHJvdmVyZSBs\r\n"
        "YXRpbmljZSB0cmViYSB1cmFkaXRpIGkgcHJvdmVydSB1dGY4IGthcmFrdGVyYSBvZG4uIGNjaXJp\r\n"
        "bGljZQ0KaSB2aWRldGkga2FrbyBzZSBwcmVsYW1hIHRla3N0IGthZGEgc3Uga2FyYWt0ZXJpIHZp\r\n"
        "c2hlYmFqdG5pLiBUcmViYWxvIGJpIGRhIGplIG5lYml0bm8gZGEgbGkgamUgZW5rb2RpbmcNCmJh\r\n"
        "c2U2NCBpbGkgcXVvdGVkIHByaW50YWJsZSwgamVyIHNlIGFzY2lpIGthcmFrdGVyaSBwcmVsYW1h\r\n"
        "anUgdSBub3ZlIGxpbmlqZS4gT3ZhaiB0ZXN0IGJpIHRyZWJhbG8gZGENCnBva2F6emUgaW1hIGxp\r\n"
        "IGJhZ292YSB1IGxvZ2ljaSBmb3JtYXRpcmFuamEsDQogYSBpc3RvIHRvIHRyZWJhIHByb3Zlcml0\r\n"
        "aSBzYSBwYXJzaXJhbmplbS4NCg0KDQoNCg0KT3ZkZSBqZSBpIHByb3ZlcmEgemEgbml6IHByYXpu\r\n"
        "aWggbGluaWphLg0KDQoNCg==\r\n");
}


/**
Parsing simple message.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_simple)
{
    string msg_str = "From: mail io <adre.sa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: parse simple\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "\r\n"
        "hello\r\n"
        "\r\n"
        "world\r\n"
        "\r\n"
        "\r\n"
        "opa bato\r\n";
    message msg;

    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mail io" && msg.from().addresses.at(0).address == "adre.sa@mailio.dev" && msg.date_time() == ldt &&
        msg.recipients_to_string() == "mailio <adresa@mailio.dev>" && msg.subject() == "parse simple" &&
        msg.content() == "hello\r\n\r\nworld\r\n\r\n\r\nopa bato");
}


/**
Parsing custom headers.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_custom_header)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    string msg_str = "From: mail io <adre.sa@mailio.dev>\r\n"
        "To: mailio <adre.sa@mailio.dev>\r\n"
        "Subject: parse custom header\r\n"
        "User-Agent: mailio\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Content-Language: en-US\r\n"
        "\r\n"
        "Hello, world!\r\n";
    msg.parse(msg_str);
    BOOST_CHECK(msg.headers().size() == 2 && msg.headers().find("User-Agent")->second == "mailio");
    msg.remove_header("User-Agent");
    BOOST_CHECK(msg.headers().size() == 1);
}


/**
Parsing a header with a non-allowed character in it's name.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_bad_header_name)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    string msg_str = "From: mail io <adre.sa@mailio.dev>\r\n"
        "To: mailio <adre.sa@mailio.dev>\r\n"
        "Subject: parse bad header name\r\n"
        "User-Ag€nt: mailio\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Content-Language: en-US\r\n"
        "\r\n"
        "Hello, world!\r\n";
    BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
}


/**
Parsing simple message with lines matching the recommended length.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_line_len)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    string msg_str = "From: adre.sa@mailio.dev\r\n"
        "To: adre.sa@mailio.dev\r\n"
        "Subject: parse line len\r\n"
        "\r\n"
        "01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n"
        "01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n"
        "01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n";

    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).address == "adre.sa@mailio.dev" && msg.content().size() == 244);
}


/**
Parsing a message with lines violating the recommended length.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_wrong_line_len)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::RECOMMENDED);
    string msg_str = "From: adre.sa@mailio.dev\r\n"
        "To: adre.sa@mailio.dev\r\n"
        "Subject: parse wrong line len\r\n"
        "\r\n"
        "01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n"
        "01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n"
        "01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n";

    BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
}
