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
using mailio::mail_address;
using mailio::mail_group;
using mailio::mime;
using mailio::message;


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
