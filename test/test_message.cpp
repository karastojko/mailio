/*

test_message.cpp
----------------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#define BOOST_TEST_MODULE message_test

#include <string>
#include <istream>
#include <fstream>
#include <utility>
#include <list>
#include <tuple>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <mailio/mailboxes.hpp>
#include <mailio/message.hpp>


using std::string;
using std::ifstream;
using std::ofstream;
using std::list;
using std::tuple;
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
using mailio::message_error;
using mailio::codec_error;


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
Formatting a message without author.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_no_from)
{
    message msg;
    msg.sender(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("format no from");
    string msg_str;
    BOOST_CHECK_THROW(msg.format(msg_str), message_error);
}


/**
Formatting a message with two authors but no sender.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_no_sender_two_authors)
{
    message msg;
    msg.add_from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_from(mail_address("karas", "karas@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("format no sender two authors");
    string msg_str;
    BOOST_CHECK_THROW(msg.format(msg_str), message_error);
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
Formatting long text ASCII charset Quoted Printable encoded to lines with the recommended length.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_long_text_ascii_qp)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format long text ascii quoted printable");
    msg.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    msg.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
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
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "Subject: format long text ascii quoted printable\r\n"
        "\r\n"
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija=\r\n"
        ". Nije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe, =\r\n"
        "jer libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 kara=\r\n"
        "ktera odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da =\r\n"
        "je nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije=\r\n"
        ". Ovaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici formatiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n");
}


/**
Formatting long text UTF-8 charset Base64 encoded to lines with the recommended length.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_long_text_utf8_base64)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format long text utf8 base64");
    msg.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    msg.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    msg.content("Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
        "па се надам да ће то овај текст показати.\r\n"
        "\r\n"
        "Треба видети како познати мејл клијенти ломе текст, па на\r\n"
        "основу тога дорадити форматирање мејла. А можда и нема потребе, јер libmailio није замишљен да се\r\n"
        "бави форматирањем текста.\r\n"
        "\r\n\r\n"
        "У сваком случају, после провере латинице треба урадити и проверу utf8 карактера одн. ћирилице\r\n"
        "и видети како се прелама текст када су карактери вишебајтни. Требало би да је небитно да ли је енкодинг\r\n"
        "base64 или quoted printable, јер се ascii карактери преламају у нове линије. Овај тест би требало да\r\n"
        "покаже има ли багова у логици форматирања,\r\n"
        " а исто то треба проверити са парсирањем.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Овде је и провера за низ празних линија.\r\n\r\n\r\n");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "Subject: format long text utf8 base64\r\n"
        "\r\n"
        "0J7QstC+INGY0LUg0ZjQsNC60L4g0LTRg9Cz0LDRh9C60LAg0L/QvtGA0YPQutCwINC60L7RmNCw\r\n"
        "INC40LzQsCDQuCDQv9GA0LDQt9C90LjRhSDQu9C40L3QuNGY0LAg0Lgg0L/RgNC10LTRg9Cz0LDR\r\n"
        "h9C60LjRhSDQu9C40L3QuNGY0LAuINCd0LjRmNC1INGY0LDRgdC90L4g0LrQsNC60L4g0ZvQtSDR\r\n"
        "gdC1INGC0LXQutGB0YIg0L/RgNC10LvQvtC80LjRgtC4DQrQv9CwINGB0LUg0L3QsNC00LDQvCDQ\r\n"
        "tNCwINGb0LUg0YLQviDQvtCy0LDRmCDRgtC10LrRgdGCINC/0L7QutCw0LfQsNGC0LguDQoNCtCi\r\n"
        "0YDQtdCx0LAg0LLQuNC00LXRgtC4INC60LDQutC+INC/0L7Qt9C90LDRgtC4INC80LXRmNC7INC6\r\n"
        "0LvQuNGY0LXQvdGC0Lgg0LvQvtC80LUg0YLQtdC60YHRgiwg0L/QsCDQvdCwDQrQvtGB0L3QvtCy\r\n"
        "0YMg0YLQvtCz0LAg0LTQvtGA0LDQtNC40YLQuCDRhNC+0YDQvNCw0YLQuNGA0LDRmtC1INC80LXR\r\n"
        "mNC70LAuINCQINC80L7QttC00LAg0Lgg0L3QtdC80LAg0L/QvtGC0YDQtdCx0LUsINGY0LXRgCBs\r\n"
        "aWJtYWlsaW8g0L3QuNGY0LUg0LfQsNC80LjRiNGZ0LXQvSDQtNCwINGB0LUNCtCx0LDQstC4INGE\r\n"
        "0L7RgNC80LDRgtC40YDQsNGa0LXQvCDRgtC10LrRgdGC0LAuDQoNCg0K0KMg0YHQstCw0LrQvtC8\r\n"
        "INGB0LvRg9GH0LDRmNGDLCDQv9C+0YHQu9C1INC/0YDQvtCy0LXRgNC1INC70LDRgtC40L3QuNGG\r\n"
        "0LUg0YLRgNC10LHQsCDRg9GA0LDQtNC40YLQuCDQuCDQv9GA0L7QstC10YDRgyB1dGY4INC60LDR\r\n"
        "gNCw0LrRgtC10YDQsCDQvtC00L0uINGb0LjRgNC40LvQuNGG0LUNCtC4INCy0LjQtNC10YLQuCDQ\r\n"
        "utCw0LrQviDRgdC1INC/0YDQtdC70LDQvNCwINGC0LXQutGB0YIg0LrQsNC00LAg0YHRgyDQutCw\r\n"
        "0YDQsNC60YLQtdGA0Lgg0LLQuNGI0LXQsdCw0ZjRgtC90LguINCi0YDQtdCx0LDQu9C+INCx0Lgg\r\n"
        "0LTQsCDRmNC1INC90LXQsdC40YLQvdC+INC00LAg0LvQuCDRmNC1INC10L3QutC+0LTQuNC90LMN\r\n"
        "CmJhc2U2NCDQuNC70LggcXVvdGVkIHByaW50YWJsZSwg0ZjQtdGAINGB0LUgYXNjaWkg0LrQsNGA\r\n"
        "0LDQutGC0LXRgNC4INC/0YDQtdC70LDQvNCw0ZjRgyDRgyDQvdC+0LLQtSDQu9C40L3QuNGY0LUu\r\n"
        "INCe0LLQsNGYINGC0LXRgdGCINCx0Lgg0YLRgNC10LHQsNC70L4g0LTQsA0K0L/QvtC60LDQttC1\r\n"
        "INC40LzQsCDQu9C4INCx0LDQs9C+0LLQsCDRgyDQu9C+0LPQuNGG0Lgg0YTQvtGA0LzQsNGC0LjR\r\n"
        "gNCw0ZrQsCwNCiDQsCDQuNGB0YLQviDRgtC+INGC0YDQtdCx0LAg0L/RgNC+0LLQtdGA0LjRgtC4\r\n"
        "INGB0LAg0L/QsNGA0YHQuNGA0LDRmtC10LwuDQoNCg0KDQoNCtCe0LLQtNC1INGY0LUg0Lgg0L/R\r\n"
        "gNC+0LLQtdGA0LAg0LfQsCDQvdC40Lcg0L/RgNCw0LfQvdC40YUg0LvQuNC90LjRmNCwLg0KDQoN\r\n"
        "Cg==\r\n");
}


/**
Formatting long text UTF-8 cyrillic charset Quoted Printable encoded to lines with the recommended length.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_long_text_utf8_cyr_qp)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format long text utf8 cyrillic quoted printable");
    msg.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    msg.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    msg.content("Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
        "па се надам да ће то овај текст показати.\r\n"
        "\r\n"
        "Треба видети како познати мејл клијенти ломе текст, па на\r\n"
        "основу тога дорадити форматирање мејла. А можда и нема потребе, јер libmailio није замишљен да се\r\n"
        "бави форматирањем текста.\r\n"
        "\r\n\r\n"
        "У сваком случају, после провере латинице треба урадити и проверу utf8 карактера одн. ћирилице\r\n"
        "и видети како се прелама текст када су карактери вишебајтни. Требало би да је небитно да ли је енкодинг\r\n"
        "base64 или quoted printable, јер се ascii карактери преламају у нове линије. Овај тест би требало да\r\n"
        "покаже има ли багова у логици форматирања,\r\n"
        "а исто то треба проверити са парсирањем.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Овде је и провера за низ празних линија.\r\n\r\n\r\n");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "Subject: format long text utf8 cyrillic quoted printable\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=BE =D1=98=D0=B5 =D1=98=D0=B0=D0=BA=D0=BE =D0=B4=D1=83=D0=B3=\r\n"
        "=D0=B0=D1=87=D0=BA=D0=B0 =D0=BF=D0=BE=D1=80=D1=83=D0=BA=D0=B0 =D0=BA=D0=BE=\r\n"
        "=D1=98=D0=B0 =D0=B8=D0=BC=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=D0=B8=\r\n"
        "=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B5=D0=B4=\r\n"
        "=D1=83=D0=B3=D0=B0=D1=87=D0=BA=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=\r\n"
        "=D0=B0. =D0=9D=D0=B8=D1=98=D0=B5 =D1=98=D0=B0=D1=81=D0=BD=D0=BE =D0=BA=D0=\r\n"
        "=B0=D0=BA=D0=BE =D1=9B=D0=B5 =D1=81=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82 =\r\n"
        "=D0=BF=D1=80=D0=B5=D0=BB=D0=BE=D0=BC=D0=B8=D1=82=D0=B8\r\n"
        "=D0=BF=D0=B0 =D1=81=D0=B5 =D0=BD=D0=B0=D0=B4=D0=B0=D0=BC =D0=B4=D0=B0 =D1=\r\n"
        "=9B=D0=B5 =D1=82=D0=BE =D0=BE=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=D0=BA=D1=81=D1=\r\n"
        "=82 =D0=BF=D0=BE=D0=BA=D0=B0=D0=B7=D0=B0=D1=82=D0=B8.\r\n"
        "\r\n"
        "=D0=A2=D1=80=D0=B5=D0=B1=D0=B0 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=\r\n"
        "=D0=B0=D0=BA=D0=BE =D0=BF=D0=BE=D0=B7=D0=BD=D0=B0=D1=82=D0=B8 =D0=BC=D0=B5=\r\n"
        "=D1=98=D0=BB =D0=BA=D0=BB=D0=B8=D1=98=D0=B5=D0=BD=D1=82=D0=B8 =D0=BB=D0=BE=\r\n"
        "=D0=BC=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82, =D0=BF=D0=B0 =D0=BD=D0=B0\r\n"
        "=D0=BE=D1=81=D0=BD=D0=BE=D0=B2=D1=83 =D1=82=D0=BE=D0=B3=D0=B0 =D0=B4=D0=BE=\r\n"
        "=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=\r\n"
        "=D0=B8=D1=80=D0=B0=D1=9A=D0=B5 =D0=BC=D0=B5=D1=98=D0=BB=D0=B0. =D0=90 =D0=\r\n"
        "=BC=D0=BE=D0=B6=D0=B4=D0=B0 =D0=B8 =D0=BD=D0=B5=D0=BC=D0=B0 =D0=BF=D0=BE=D1=\r\n"
        "=82=D1=80=D0=B5=D0=B1=D0=B5, =D1=98=D0=B5=D1=80 libmailio =D0=BD=D0=B8=D1=\r\n"
        "=98=D0=B5 =D0=B7=D0=B0=D0=BC=D0=B8=D1=88=D1=99=D0=B5=D0=BD =D0=B4=D0=B0 =D1=\r\n"
        "=81=D0=B5\r\n"
        "=D0=B1=D0=B0=D0=B2=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=\r\n"
        "=D0=B0=D1=9A=D0=B5=D0=BC =D1=82=D0=B5=D0=BA=D1=81=D1=82=D0=B0.\r\n"
        "\r\n"
        "\r\n"
        "=D0=A3 =D1=81=D0=B2=D0=B0=D0=BA=D0=BE=D0=BC =D1=81=D0=BB=D1=83=D1=87=D0=B0=\r\n"
        "=D1=98=D1=83, =D0=BF=D0=BE=D1=81=D0=BB=D0=B5 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B5 =D0=BB=D0=B0=D1=82=D0=B8=D0=BD=D0=B8=D1=86=D0=B5 =D1=82=D1=80=\r\n"
        "=D0=B5=D0=B1=D0=B0 =D1=83=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D0=B8 =D0=BF=\r\n"
        "=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D1=83 utf8 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=\r\n"
        "=82=D0=B5=D1=80=D0=B0 =D0=BE=D0=B4=D0=BD. =D1=9B=D0=B8=D1=80=D0=B8=D0=BB=D0=\r\n"
        "=B8=D1=86=D0=B5\r\n"
        "=D0=B8 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=D0=B0=D0=BA=D0=BE =D1=81=\r\n"
        "=D0=B5 =D0=BF=D1=80=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0 =D1=82=D0=B5=D0=BA=D1=81=\r\n"
        "=D1=82 =D0=BA=D0=B0=D0=B4=D0=B0 =D1=81=D1=83 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=\r\n"
        "=D1=82=D0=B5=D1=80=D0=B8 =D0=B2=D0=B8=D1=88=D0=B5=D0=B1=D0=B0=D1=98=D1=82=\r\n"
        "=D0=BD=D0=B8. =D0=A2=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B1=D0=B8 =D0=\r\n"
        "=B4=D0=B0 =D1=98=D0=B5 =D0=BD=D0=B5=D0=B1=D0=B8=D1=82=D0=BD=D0=BE =D0=B4=D0=\r\n"
        "=B0 =D0=BB=D0=B8 =D1=98=D0=B5 =D0=B5=D0=BD=D0=BA=D0=BE=D0=B4=D0=B8=D0=BD=D0=\r\n"
        "=B3\r\n"
        "base64 =D0=B8=D0=BB=D0=B8 quoted printable, =D1=98=D0=B5=D1=80 =D1=81=D0=B5 =\r\n"
        "ascii =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=82=D0=B5=D1=80=D0=B8 =D0=BF=D1=80=\r\n"
        "=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0=D1=98=D1=83 =D1=83 =D0=BD=D0=BE=D0=B2=D0=B5 =\r\n"
        "=D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B5. =D0=9E=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=\r\n"
        "=D1=81=D1=82 =D0=B1=D0=B8 =D1=82=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B4=\r\n"
        "=D0=B0\r\n"
        "=D0=BF=D0=BE=D0=BA=D0=B0=D0=B6=D0=B5 =D0=B8=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=B1=\r\n"
        "=D0=B0=D0=B3=D0=BE=D0=B2=D0=B0 =D1=83 =D0=BB=D0=BE=D0=B3=D0=B8=D1=86=D0=B8 =\r\n"
        "=D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=D0=B0=D1=9A=D0=B0,\r\n"
        "=D0=B0 =D0=B8=D1=81=D1=82=D0=BE =D1=82=D0=BE =D1=82=D1=80=D0=B5=D0=B1=D0=B0 =\r\n"
        "=D0=BF=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D0=B8=D1=82=D0=B8 =D1=81=D0=B0 =D0=BF=\r\n"
        "=D0=B0=D1=80=D1=81=D0=B8=D1=80=D0=B0=D1=9A=D0=B5=D0=BC.\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=B4=D0=B5 =D1=98=D0=B5 =D0=B8 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B0 =D0=B7=D0=B0 =D0=BD=D0=B8=D0=B7 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=\r\n"
        "=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0.\r\n");
}


/**
Formatting long text UTF-8 latin charset Quoted Printable encoded to lines with the recommended length.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_long_text_utf8_lat_qp)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format long text utf8 latin quoted printable");
    msg.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    msg.content_type(message::media_type_t::TEXT, "plain", "utf-8");

    msg.content("Ovo je jako dugačka poruka koja ima i praznih linija i predugačkih linija. Nije jasno kako će se tekst prelomiti\r\n"
        "pa se nadam da će to ovaj test pokazati.\r\n"
        "\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadržaja mejla. A možda i nema potrebe, jer libmailio nije zamišljen da se\r\n"
        "bavi formatiranjem teksta.\r\n"
        "\r\n\r\n"
        "U svakom slučaju, posle provere latinice treba uraditi i proveru utf8 karaktera odn. ćirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri višebajtni. Trebalo bi da je nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije. Ovaj test bi trebalo da\r\n"
        "pokaže ima li bagova u logici formatiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n\r\n\r\n");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "Subject: format long text utf8 latin quoted printable\r\n"
        "\r\n"
        "Ovo je jako duga=C4=8Dka poruka koja ima i praznih linija i preduga=C4=8Dki=\r\n"
        "h linija. Nije jasno kako =C4=87e se tekst prelomiti\r\n"
        "pa se nadam da =C4=87e to ovaj test pokazati.\r\n"
        "\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadr=C5=BEaja mejla. A mo=C5=BEda i nema =\r\n"
        "potrebe, jer libmailio nije zami=C5=A1ljen da se\r\n"
        "bavi formatiranjem teksta.\r\n"
        "\r\n"
        "\r\n"
        "U svakom slu=C4=8Daju, posle provere latinice treba uraditi i proveru utf8 =\r\n"
        "karaktera odn. =C4=87irilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vi=C5=A1ebajtni. Trebalo b=\r\n"
        "i da je nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije=\r\n"
        ". Ovaj test bi trebalo da\r\n"
        "poka=C5=BEe ima li bagova u logici formatiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "Ovde je i provera za niz praznih linija.\r\n");
}


/**
Formatting a related multipart message with the first part HTML ASCII charset Bit7 encoded, the second part text ASCII charset Base64 encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_multipart_html_ascii_bit7_text_ascii_base64)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format multipart html ascii bit7 text ascii base64");
    msg.boundary("my_bound");
    msg.content_type(message::media_type_t::MULTIPART, "related");

    mime m1;
    m1.content_type(message::media_type_t::TEXT, "html", "us-ascii");
    m1.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_7);
    m1.content("<html><head></head><body><h1>Hello, World!</h1></body></html>");

    mime m2;
    m2.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
    m2.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    m2.content("Zdravo, Svete!");

    msg.add_part(m1);
    msg.add_part(m2);

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: format multipart html ascii bit7 text ascii base64\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "<html><head></head><body><h1>Hello, World!</h1></body></html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "\r\n"
        "WmRyYXZvLCBTdmV0ZSE=\r\n"
        "\r\n"
        "--my_bound--\r\n");
}


/**
Formatting an alternative multipart message with the first part HTML ASCII charset Quoted Printable encoded, the second part text ASCII charset Bit8 encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_multipart_html_ascii_qp_text_ascii_bit8)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format multipart html ascii qp text ascii bit8");
    msg.boundary("my_bound");
    msg.content_type(message::media_type_t::MULTIPART, "alternative");

    mime m1;
    m1.content_type(message::media_type_t::TEXT, "html", "us-ascii");
    m1.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    m1.content("<html><head></head><body><h1>Hello, World!</h1></body></html>");

    mime m2;
    m2.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
    m2.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_8);
    m2.content("Zdravo, Svete!");

    msg.add_part(m1);
    msg.add_part(m2);

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/alternative; boundary=\"my_bound\"\r\n"
        "Subject: format multipart html ascii qp text ascii bit8\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "<html><head></head><body><h1>Hello, World!</h1></body></html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 8bit\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n"
        "\r\n"
        "--my_bound--\r\n");
}


/**
Formatting a related multipart with the first part HTML default charset Base64 encoded, the second part text UTF-8 charset Quoted Printable encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_related_html_default_base64_text_utf8_qp)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format related html default base64 text utf8 qp");
    msg.boundary("my_bound");
    msg.content_type(message::media_type_t::MULTIPART, "related");

    mime m1;
    m1.content_type(message::media_type_t::TEXT, "html");
    m1.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    m1.content("<html><head></head><body><h1>Hello, World!</h1></body></html>");

    mime m2;
    m2.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    m2.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    m2.content("Здраво, Свете!");

    msg.add_part(m1);
    msg.add_part(m2);

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: format related html default base64 text utf8 qp\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "\r\n"
        "PGh0bWw+PGhlYWQ+PC9oZWFkPjxib2R5PjxoMT5IZWxsbywgV29ybGQhPC9oMT48L2JvZHk+PC9o\r\n"
        "dG1sPg==\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "=D0=97=D0=B4=D1=80=D0=B0=D0=B2=D0=BE, =D0=A1=D0=B2=D0=B5=D1=82=D0=B5!\r\n"
        "\r\n"
        "--my_bound--\r\n");
}


/**
Formatting an alternative multipart with the first part HTML ASCII charset Bit8 encoded, the second part text UTF-8 charset Base64 encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_alternative_html_ascii_bit8_text_utf8_base64)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format alternative html ascii bit8 text utf8 base64");
    msg.boundary("my_bound");
    msg.content_type(message::media_type_t::MULTIPART, "alternative");

    mime m1;
    m1.content_type(message::media_type_t::TEXT, "html", "us-ascii");
    m1.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_8);
    m1.content("<html><head></head><body><h1>Hello, World!</h1></body></html>");

    mime m2;
    m2.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    m2.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    m2.content("Здраво, Свете!");

    msg.add_part(m1);
    msg.add_part(m2);

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/alternative; boundary=\"my_bound\"\r\n"
        "Subject: format alternative html ascii bit8 text utf8 base64\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 8bit\r\n"
        "\r\n"
        "<html><head></head><body><h1>Hello, World!</h1></body></html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "\r\n"
        "0JfQtNGA0LDQstC+LCDQodCy0LXRgtC1IQ==\r\n"
        "\r\n"
        "--my_bound--\r\n");
}


/**
Formatting a multipart message with leading dots and the escaping flag turned off and on.

The first part is HTML ASCII charset Seven Bit encoded, the second part is text UTF-8 charset Quoted Printable encoded, the third part is text UTF-8
charset Quoted Printable encoded, the fourth part is HTML ASCII charset Base64 encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_dotted_multipart)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("Tomislav Karastojkovic", "qwerty@gmail.com"));
    msg.add_recipient(mail_address("Tomislav Karastojkovic", "asdfgh@zoho.com"));
    msg.add_recipient(mail_address("Tomislav Karastojkovic", "zxcvbn@hotmail.com"));
    ptime t = time_from_string("2016-03-15 13:13:32");
    time_zone_ptr tz(new posix_time_zone("-00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format dotted multipart");
    msg.boundary("my_bound");
    msg.content_type(message::media_type_t::MULTIPART, "related");

    mime m1;
    m1.content_type(message::media_type_t::TEXT, "html", "us-ascii");
    m1.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_7);
    m1.content("<html>\r\n"
        "\t<head>\r\n"
        "\t\t<title>.naslov</title>\r\n"
        "\t</head>\r\n"
        "..\r\n"
        "\t<body>\r\n"
        "\t\t<h1>\r\n"
        "\t\t\t..Zdravo, Sveteeeee!\r\n"
        "\t\t</h1>\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "\t.<p>Ima li koga?</p>\r\n"
        "\t</body>\r\n"
        "</html>");

    mime m2;
    m2.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    m2.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    m2.content(".Zdravo svete!\r\n"
        "..\r\n"
        "Ima li koga?\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "..yabadabadoo...\r\n");

    mime m3;
    m3.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    m3.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    m3.content(".Здраво, Свете!\r\n"
        "..\r\n"
        "Има ли кога?\r\n"
        "\r\n\r\n"
        ".\r\n"
        "\r\n\r\n"
        "..јабадабадуу...\r\n");

    mime m4;
    m4.content_type(message::media_type_t::TEXT, "html", "us-ascii");
    m4.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    m4.content("<html>\r\n"
        "\t<head>\r\n"
        "\t\t<title>.naslov</title>\r\n"
        "\t</head>\r\n"
        "..\r\n"
        "\t<body>\r\n"
        "\t\t<h1>\r\n"
        "\t\t\t..Zdravo, Sveteeeee!\r\n"
        "\t\t</h1>\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "\t.<p>Ima li koga?</p>\r\n"
        "\t</body>\r\n"
        "</html>");

    msg.add_part(m1);
    msg.add_part(m2);
    msg.add_part(m3);
    msg.add_part(m4);

    {
        string msg_str;
        msg.format(msg_str, false);
        BOOST_CHECK(msg_str ==
            "From: mailio <adresa@mailio.dev>\r\n"
            "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
            "To: mailio <adresa@mailio.dev>,\r\n"
            "  Tomislav Karastojkovic <qwerty@gmail.com>,\r\n"
            "  Tomislav Karastojkovic <asdfgh@zoho.com>,\r\n"
            "  Tomislav Karastojkovic <zxcvbn@hotmail.com>\r\n"
            "Date: Tue, 15 Mar 2016 13:13:32 +0000\r\n"
            "MIME-Version: 1.0\r\n"
            "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
            "Subject: format dotted multipart\r\n"
            "\r\n"
            "--my_bound\r\n"
            "Content-Type: text/html; charset=us-ascii\r\n"
            "Content-Transfer-Encoding: 7bit\r\n"
            "\r\n"
            "<html>\r\n"
            "\t<head>\r\n"
            "\t\t<title>.naslov</title>\r\n"
            "\t</head>\r\n"
            "..\r\n"
            "\t<body>\r\n"
            "\t\t<h1>\r\n"
            "\t\t\t..Zdravo, Sveteeeee!\r\n"
            "\t\t</h1>\r\n"
            "\r\n"
            "\r\n"
            ".\r\n"
            "\r\n\r\n"
            "\t.<p>Ima li koga?</p>\r\n"
            "\t</body>\r\n"
            "</html>\r\n"
            "\r\n"
            "--my_bound\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Content-Transfer-Encoding: Quoted-Printable\r\n"
            "\r\n"
            ".Zdravo svete!\r\n"
            "..\r\n"
            "Ima li koga?\r\n"
            "\r\n"
            "\r\n"
            ".\r\n"
            "\r\n"
            "\r\n"
            "..yabadabadoo...\r\n"
            "\r\n"
            "--my_bound\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Content-Transfer-Encoding: Quoted-Printable\r\n"
            "\r\n"
            ".=D0=97=D0=B4=D1=80=D0=B0=D0=B2=D0=BE, =D0=A1=D0=B2=D0=B5=D1=82=D0=B5!\r\n"
            "..\r\n"
            "=D0=98=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=BA=D0=BE=D0=B3=D0=B0?\r\n"
            "\r\n"
            "\r\n"
            ".\r\n"
            "\r\n"
            "\r\n"
            "..=D1=98=D0=B0=D0=B1=D0=B0=D0=B4=D0=B0=D0=B1=D0=B0=D0=B4=D1=83=D1=83...\r\n"
            "\r\n"
            "--my_bound\r\n"
            "Content-Type: text/html; charset=us-ascii\r\n"
            "Content-Transfer-Encoding: Base64\r\n"
            "\r\n"
            "PGh0bWw+DQoJPGhlYWQ+DQoJCTx0aXRsZT4ubmFzbG92PC90aXRsZT4NCgk8L2hlYWQ+DQouLg0K\r\n"
            "CTxib2R5Pg0KCQk8aDE+DQoJCQkuLlpkcmF2bywgU3ZldGVlZWVlIQ0KCQk8L2gxPg0KDQoNCi4N\r\n"
            "Cg0KDQoJLjxwPkltYSBsaSBrb2dhPzwvcD4NCgk8L2JvZHk+DQo8L2h0bWw+\r\n"
            "\r\n"
            "--my_bound--\r\n");
    }

    {
        string msg_str;
        msg.format(msg_str, true);

        BOOST_CHECK(msg_str ==
            "From: mailio <adresa@mailio.dev>\r\n"
            "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
            "To: mailio <adresa@mailio.dev>,\r\n"
            "  Tomislav Karastojkovic <qwerty@gmail.com>,\r\n"
            "  Tomislav Karastojkovic <asdfgh@zoho.com>,\r\n"
            "  Tomislav Karastojkovic <zxcvbn@hotmail.com>\r\n"
            "Date: Tue, 15 Mar 2016 13:13:32 +0000\r\n"
            "MIME-Version: 1.0\r\n"
            "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
            "Subject: format dotted multipart\r\n"
            "\r\n"
            "--my_bound\r\n"
            "Content-Type: text/html; charset=us-ascii\r\n"
            "Content-Transfer-Encoding: 7bit\r\n"
            "\r\n"
            "<html>\r\n"
            "\t<head>\r\n"
            "\t\t<title>.naslov</title>\r\n"
            "\t</head>\r\n"
            "...\r\n"
            "\t<body>\r\n"
            "\t\t<h1>\r\n"
            "\t\t\t..Zdravo, Sveteeeee!\r\n"
            "\t\t</h1>\r\n"
            "\r\n"
            "\r\n"
            "..\r\n"
            "\r\n"
            "\r\n"
            "\t.<p>Ima li koga?</p>\r\n"
            "\t</body>\r\n"
            "</html>\r\n"
            "\r\n"
            "--my_bound\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Content-Transfer-Encoding: Quoted-Printable\r\n"
            "\r\n"
            "..Zdravo svete!\r\n"
            "...\r\n"
            "Ima li koga?\r\n"
            "\r\n"
            "\r\n"
            "..\r\n"
            "\r\n"
            "\r\n"
            "...yabadabadoo...\r\n"
            "\r\n"
            "--my_bound\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Content-Transfer-Encoding: Quoted-Printable\r\n"
            "\r\n"
            "..=D0=97=D0=B4=D1=80=D0=B0=D0=B2=D0=BE, =D0=A1=D0=B2=D0=B5=D1=82=D0=B5!\r\n"
            "...\r\n"
            "=D0=98=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=BA=D0=BE=D0=B3=D0=B0?\r\n"
            "\r\n"
            "\r\n"
            "..\r\n"
            "\r\n"
            "\r\n"
            "...=D1=98=D0=B0=D0=B1=D0=B0=D0=B4=D0=B0=D0=B1=D0=B0=D0=B4=D1=83=D1=83...\r\n"
            "\r\n"
            "--my_bound\r\n"
            "Content-Type: text/html; charset=us-ascii\r\n"
            "Content-Transfer-Encoding: Base64\r\n"
            "\r\n"
            "PGh0bWw+DQoJPGhlYWQ+DQoJCTx0aXRsZT4ubmFzbG92PC90aXRsZT4NCgk8L2hlYWQ+DQouLg0K\r\n"
            "CTxib2R5Pg0KCQk8aDE+DQoJCQkuLlpkcmF2bywgU3ZldGVlZWVlIQ0KCQk8L2gxPg0KDQoNCi4N\r\n"
            "Cg0KDQoJLjxwPkltYSBsaSBrb2dhPzwvcD4NCgk8L2JvZHk+DQo8L2h0bWw+\r\n"
            "\r\n"
            "--my_bound--\r\n");
    }
}


/**
Formatting multipart with a long content in various combinations.

The message has four parts: the first is long HTML ASCII charset Seven Bit encoded, the second is long text ASCII charset Base64 encoded, the third is long
text ASCII charset Quoted Printable encoded, the fourth is long text UTF-8 charset Quoted Printable encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_long_multipart)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format long multipart");
    msg.boundary("my_bound");
    msg.content_type(message::media_type_t::MULTIPART, "related");

    mime m1;
    m1.content_type(message::media_type_t::TEXT, "html", "us-ascii");
    m1.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_7);
    m1.content("<html><head></head><body><h1>Hello, World!</h1><p>Zdravo Svete!</p><p>Opa Bato!</p><p>Shta ima?</p><p>Yaba Daba Doo!</p></body></html>");

    mime m2;
    m2.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
    m2.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    m2.content("Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. Nije jasno kako ce se tekst prelomiti\r\n"
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

    mime m3;
    m3.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
    m3.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    m3.content("Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. Nije jasno kako ce se tekst prelomiti\r\n"
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

    mime m4;
    m4.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    m4.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    m4.content("Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
        "па се надам да ће то овај текст показати.\r\n"
        "\r\n"
        "Треба видети како познати мејл клијенти ломе текст, па на\r\n"
        "основу тога дорадити форматирање мејла. А можда и нема потребе, јер libmailio није замишљен да се\r\n"
        "бави форматирањем текста.\r\n"
        "\r\n\r\n"
        "У сваком случају, после провере латинице треба урадити и проверу utf8 карактера одн. ћирилице\r\n"
        "и видети како се прелама текст када су карактери вишебајтни. Требало би да је небитно да ли је енкодинг\r\n"
        "base64 или quoted printable, јер се ascii карактери преламају у нове линије. Овај тест би требало да\r\n"
        "покаже има ли багова у логици форматирања,\r\n"
        "а исто то треба проверити са парсирањем.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Овде је и провера за низ празних линија.\r\n\r\n\r\n");

    msg.add_part(m1);
    msg.add_part(m2);
    msg.add_part(m3);
    msg.add_part(m4);

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: format long multipart\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "<html><head></head><body><h1>Hello, World!</h1><p>Zdravo Svete!</p><p>Opa Bato\r\n"
        "!</p><p>Shta ima?</p><p>Yaba Daba Doo!</p></body></html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
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
        "aWggbGluaWphLg0KDQoNCg==\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija=\r\n"
        ". Nije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe, =\r\n"
        "jer libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 kara=\r\n"
        "ktera odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da =\r\n"
        "je nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije=\r\n"
        ". Ovaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici formatiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=BE =D1=98=D0=B5 =D1=98=D0=B0=D0=BA=D0=BE =D0=B4=D1=83=D0=B3=\r\n"
        "=D0=B0=D1=87=D0=BA=D0=B0 =D0=BF=D0=BE=D1=80=D1=83=D0=BA=D0=B0 =D0=BA=D0=BE=\r\n"
        "=D1=98=D0=B0 =D0=B8=D0=BC=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=D0=B8=\r\n"
        "=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B5=D0=B4=\r\n"
        "=D1=83=D0=B3=D0=B0=D1=87=D0=BA=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=\r\n"
        "=D0=B0. =D0=9D=D0=B8=D1=98=D0=B5 =D1=98=D0=B0=D1=81=D0=BD=D0=BE =D0=BA=D0=\r\n"
        "=B0=D0=BA=D0=BE =D1=9B=D0=B5 =D1=81=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82 =\r\n"
        "=D0=BF=D1=80=D0=B5=D0=BB=D0=BE=D0=BC=D0=B8=D1=82=D0=B8\r\n"
        "=D0=BF=D0=B0 =D1=81=D0=B5 =D0=BD=D0=B0=D0=B4=D0=B0=D0=BC =D0=B4=D0=B0 =D1=\r\n"
        "=9B=D0=B5 =D1=82=D0=BE =D0=BE=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=D0=BA=D1=81=D1=\r\n"
        "=82 =D0=BF=D0=BE=D0=BA=D0=B0=D0=B7=D0=B0=D1=82=D0=B8.\r\n"
        "\r\n"
        "=D0=A2=D1=80=D0=B5=D0=B1=D0=B0 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=\r\n"
        "=D0=B0=D0=BA=D0=BE =D0=BF=D0=BE=D0=B7=D0=BD=D0=B0=D1=82=D0=B8 =D0=BC=D0=B5=\r\n"
        "=D1=98=D0=BB =D0=BA=D0=BB=D0=B8=D1=98=D0=B5=D0=BD=D1=82=D0=B8 =D0=BB=D0=BE=\r\n"
        "=D0=BC=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82, =D0=BF=D0=B0 =D0=BD=D0=B0\r\n"
        "=D0=BE=D1=81=D0=BD=D0=BE=D0=B2=D1=83 =D1=82=D0=BE=D0=B3=D0=B0 =D0=B4=D0=BE=\r\n"
        "=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=\r\n"
        "=D0=B8=D1=80=D0=B0=D1=9A=D0=B5 =D0=BC=D0=B5=D1=98=D0=BB=D0=B0. =D0=90 =D0=\r\n"
        "=BC=D0=BE=D0=B6=D0=B4=D0=B0 =D0=B8 =D0=BD=D0=B5=D0=BC=D0=B0 =D0=BF=D0=BE=D1=\r\n"
        "=82=D1=80=D0=B5=D0=B1=D0=B5, =D1=98=D0=B5=D1=80 libmailio =D0=BD=D0=B8=D1=\r\n"
        "=98=D0=B5 =D0=B7=D0=B0=D0=BC=D0=B8=D1=88=D1=99=D0=B5=D0=BD =D0=B4=D0=B0 =D1=\r\n"
        "=81=D0=B5\r\n"
        "=D0=B1=D0=B0=D0=B2=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=\r\n"
        "=D0=B0=D1=9A=D0=B5=D0=BC =D1=82=D0=B5=D0=BA=D1=81=D1=82=D0=B0.\r\n"
        "\r\n"
        "\r\n"
        "=D0=A3 =D1=81=D0=B2=D0=B0=D0=BA=D0=BE=D0=BC =D1=81=D0=BB=D1=83=D1=87=D0=B0=\r\n"
        "=D1=98=D1=83, =D0=BF=D0=BE=D1=81=D0=BB=D0=B5 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B5 =D0=BB=D0=B0=D1=82=D0=B8=D0=BD=D0=B8=D1=86=D0=B5 =D1=82=D1=80=\r\n"
        "=D0=B5=D0=B1=D0=B0 =D1=83=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D0=B8 =D0=BF=\r\n"
        "=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D1=83 utf8 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=\r\n"
        "=82=D0=B5=D1=80=D0=B0 =D0=BE=D0=B4=D0=BD. =D1=9B=D0=B8=D1=80=D0=B8=D0=BB=D0=\r\n"
        "=B8=D1=86=D0=B5\r\n"
        "=D0=B8 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=D0=B0=D0=BA=D0=BE =D1=81=\r\n"
        "=D0=B5 =D0=BF=D1=80=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0 =D1=82=D0=B5=D0=BA=D1=81=\r\n"
        "=D1=82 =D0=BA=D0=B0=D0=B4=D0=B0 =D1=81=D1=83 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=\r\n"
        "=D1=82=D0=B5=D1=80=D0=B8 =D0=B2=D0=B8=D1=88=D0=B5=D0=B1=D0=B0=D1=98=D1=82=\r\n"
        "=D0=BD=D0=B8. =D0=A2=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B1=D0=B8 =D0=\r\n"
        "=B4=D0=B0 =D1=98=D0=B5 =D0=BD=D0=B5=D0=B1=D0=B8=D1=82=D0=BD=D0=BE =D0=B4=D0=\r\n"
        "=B0 =D0=BB=D0=B8 =D1=98=D0=B5 =D0=B5=D0=BD=D0=BA=D0=BE=D0=B4=D0=B8=D0=BD=D0=\r\n"
        "=B3\r\n"
        "base64 =D0=B8=D0=BB=D0=B8 quoted printable, =D1=98=D0=B5=D1=80 =D1=81=D0=B5 =\r\n"
        "ascii =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=82=D0=B5=D1=80=D0=B8 =D0=BF=D1=80=\r\n"
        "=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0=D1=98=D1=83 =D1=83 =D0=BD=D0=BE=D0=B2=D0=B5 =\r\n"
        "=D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B5. =D0=9E=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=\r\n"
        "=D1=81=D1=82 =D0=B1=D0=B8 =D1=82=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B4=\r\n"
        "=D0=B0\r\n"
        "=D0=BF=D0=BE=D0=BA=D0=B0=D0=B6=D0=B5 =D0=B8=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=B1=\r\n"
        "=D0=B0=D0=B3=D0=BE=D0=B2=D0=B0 =D1=83 =D0=BB=D0=BE=D0=B3=D0=B8=D1=86=D0=B8 =\r\n"
        "=D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=D0=B0=D1=9A=D0=B0,\r\n"
        "=D0=B0 =D0=B8=D1=81=D1=82=D0=BE =D1=82=D0=BE =D1=82=D1=80=D0=B5=D0=B1=D0=B0 =\r\n"
        "=D0=BF=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D0=B8=D1=82=D0=B8 =D1=81=D0=B0 =D0=BF=\r\n"
        "=D0=B0=D1=80=D1=81=D0=B8=D1=80=D0=B0=D1=9A=D0=B5=D0=BC.\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=B4=D0=B5 =D1=98=D0=B5 =D0=B8 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B0 =D0=B7=D0=B0 =D0=BD=D0=B8=D0=B7 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=\r\n"
        "=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0.\r\n"
        "\r\n"
        "--my_bound--\r\n");
}


/**
Formatting a multipart message which contains themselves more multipart messages.

The message is created, formatted and parsed back again, so resulting strings could be compared.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_parse_nested_multipart)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format nested multipart");
    msg.content_type(message::media_type_t::MULTIPART, "related");
    msg.boundary("my_global_bound");
    msg.content("global content");

    mime m1;
    m1.content_type(message::media_type_t::MULTIPART, "related");
    m1.boundary("my_first_boundary");

    mime m11;
    m11.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    m11.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    m11.content("мимe парт 1.1");

    mime m12;
    m12.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
    m12.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_8);
    m12.content("mime 1.2");

    mime m13;
    m13.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
    m13.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_7);
    m13.content("mime 1.3");

    m1.add_part(m11);
    m1.add_part(m12);
    m1.add_part(m13);

    mime m2;
    m2.content_type(message::media_type_t::MULTIPART, "related");
    m2.boundary("my_second_boundary");

    mime m21;
    m21.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    m21.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    m21.content("мимe парт 2.1");

    mime m22;
    m22.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    m22.content_transfer_encoding(mime::content_transfer_encoding_t::BASE_64);
    m22.content("mime 2.2");

    m2.add_part(m21);
    m2.add_part(m22);

    msg.add_part(m1);
    msg.add_part(m2);
    string msg_str;
    msg.format(msg_str);

    message msg_msg;
    msg_msg.parse(msg_str);
    string msg_msg_str;
    msg_msg.format(msg_msg_str);
    BOOST_CHECK(msg_str == msg_msg_str);
}


/**
Formatting multipart message with both content and parts.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_multipart_content)
{
    message msg;
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("format multipart content");
    msg.boundary("my_bound");
    msg.content_type(message::media_type_t::MULTIPART, "related");
    msg.content("This is a multipart message.");

    mime m1;
    m1.content_type(message::media_type_t::TEXT, "html", "us-ascii");
    m1.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_7);
    m1.content("<html><head></head><body><h1>Hello, World!</h1></body></html>");

    mime m2;
    m2.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
    m2.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_7);
    m2.content("Zdravo, Svete!");

    msg.add_part(m1);
    msg.add_part(m2);
    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: format multipart content\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "This is a multipart message.\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "<html><head></head><body><h1>Hello, World!</h1></body></html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n"
        "\r\n"
        "--my_bound--\r\n");
}


/**
Attaching two files to a message.

@pre  Files `cv.txt` and `aleph0.png` in the current directory.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_attachment)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("format attachment");
    std::ifstream ifs1("cv.txt");
    msg.attach(ifs1, "TomislavKarastojkovic_CV.txt", message::media_type_t::APPLICATION, "txt");
    std::ifstream ifs2("aleph0.png");
    msg.attach(ifs2, "logo.png", message::media_type_t::IMAGE, "png");

    BOOST_CHECK(msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "mixed" && msg.attachments_size() == 2);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::APPLICATION && msg.parts().at(0).content_type().subtype == "txt" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 && msg.parts().at(0).content_disposition() ==
        mime::content_disposition_t::ATTACHMENT);
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::IMAGE && msg.parts().at(1).content_type().subtype == "png" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 && msg.parts().at(1).content_disposition() ==
        mime::content_disposition_t::ATTACHMENT);
}


/**
Attaching a file with UTF-8 name.

@pre  File `cv.txt` in the current directory.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_attachment_utf8)
{
    message msg;
    msg.header_codec(message::header_codec_t::BASE64);
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("format attachment utf8");
    msg.boundary("mybnd");
    std::ifstream ifs1("cv.txt");
    msg.attach(ifs1, "TomislavKarastojković_CV.txt", message::media_type_t::TEXT, "plain");
    string msg_str;
    msg.format(msg_str);

    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/mixed; boundary=\"mybnd\"\r\n"
        "Subject: format attachment utf8\r\n"
        "\r\n"
        "--mybnd\r\n"
        "Content-Type: text/plain; \r\n"
        "  name=\"=?UTF-8?B?VG9taXNsYXZLYXJhc3RvamtvdmnEh19DVi50eHQ=?=\"\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "Content-Disposition: attachment; \r\n"
        "  filename=\"=?UTF-8?B?VG9taXNsYXZLYXJhc3RvamtvdmnEh19DVi50eHQ=?=\"\r\n"
        "\r\n"
        "VG9taXNsYXYgS2FyYXN0b2prb3ZpxIcgQ1YK\r\n"
        "\r\n"
        "--mybnd--\r\n");
}


/**
Attaching a file with long UTF-8 message content.

@pre  File `cv.txt` in the current directory.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_msg_att)
{
    message msg;
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("format message attachment");
    msg.boundary("mybnd");
    msg.content_type(message::media_type_t::TEXT, "plain", "utf-8");
    msg.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    msg.content("Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
        "па се надам да ће то овај текст показати.\r\n"
        "\r\n"
        "Треба видети како познати мејл клијенти ломе текст, па на\r\n"
        "основу тога дорадити форматирање мејла. А можда и нема потребе, јер libmailio није замишљен да се\r\n"
        "бави форматирањем текста.\r\n"
        "\r\n\r\n"
        "У сваком случају, после провере латинице треба урадити и проверу utf8 карактера одн. ћирилице\r\n"
        "и видети како се прелама текст када су карактери вишебајтни. Требало би да је небитно да ли је енкодинг\r\n"
        "base64 или quoted printable, јер се ascii карактери преламају у нове линије. Овај тест би требало да\r\n"
        "покаже има ли багова у логици форматирања,\r\n"
        " а исто то треба проверити са парсирањем.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Овде је и провера за низ празних линија.\r\n\r\n\r\n");
    std::ifstream ifs1("cv.txt");
    msg.attach(ifs1, "TomislavKarastojkovic_CV.txt", message::media_type_t::TEXT, "plain");
    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str ==
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/mixed; charset=utf-8; boundary=\"mybnd\"\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "Subject: format message attachment\r\n"
        "\r\n"
        "--mybnd\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=BE =D1=98=D0=B5 =D1=98=D0=B0=D0=BA=D0=BE =D0=B4=D1=83=D0=B3=\r\n"
        "=D0=B0=D1=87=D0=BA=D0=B0 =D0=BF=D0=BE=D1=80=D1=83=D0=BA=D0=B0 =D0=BA=D0=BE=\r\n"
        "=D1=98=D0=B0 =D0=B8=D0=BC=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=D0=B8=\r\n"
        "=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B5=D0=B4=\r\n"
        "=D1=83=D0=B3=D0=B0=D1=87=D0=BA=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=\r\n"
        "=D0=B0. =D0=9D=D0=B8=D1=98=D0=B5 =D1=98=D0=B0=D1=81=D0=BD=D0=BE =D0=BA=D0=\r\n"
        "=B0=D0=BA=D0=BE =D1=9B=D0=B5 =D1=81=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82 =\r\n"
        "=D0=BF=D1=80=D0=B5=D0=BB=D0=BE=D0=BC=D0=B8=D1=82=D0=B8\r\n"
        "=D0=BF=D0=B0 =D1=81=D0=B5 =D0=BD=D0=B0=D0=B4=D0=B0=D0=BC =D0=B4=D0=B0 =D1=\r\n"
        "=9B=D0=B5 =D1=82=D0=BE =D0=BE=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=D0=BA=D1=81=D1=\r\n"
        "=82 =D0=BF=D0=BE=D0=BA=D0=B0=D0=B7=D0=B0=D1=82=D0=B8.\r\n"
        "\r\n"
        "=D0=A2=D1=80=D0=B5=D0=B1=D0=B0 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=\r\n"
        "=D0=B0=D0=BA=D0=BE =D0=BF=D0=BE=D0=B7=D0=BD=D0=B0=D1=82=D0=B8 =D0=BC=D0=B5=\r\n"
        "=D1=98=D0=BB =D0=BA=D0=BB=D0=B8=D1=98=D0=B5=D0=BD=D1=82=D0=B8 =D0=BB=D0=BE=\r\n"
        "=D0=BC=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82, =D0=BF=D0=B0 =D0=BD=D0=B0\r\n"
        "=D0=BE=D1=81=D0=BD=D0=BE=D0=B2=D1=83 =D1=82=D0=BE=D0=B3=D0=B0 =D0=B4=D0=BE=\r\n"
        "=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=\r\n"
        "=D0=B8=D1=80=D0=B0=D1=9A=D0=B5 =D0=BC=D0=B5=D1=98=D0=BB=D0=B0. =D0=90 =D0=\r\n"
        "=BC=D0=BE=D0=B6=D0=B4=D0=B0 =D0=B8 =D0=BD=D0=B5=D0=BC=D0=B0 =D0=BF=D0=BE=D1=\r\n"
        "=82=D1=80=D0=B5=D0=B1=D0=B5, =D1=98=D0=B5=D1=80 libmailio =D0=BD=D0=B8=D1=\r\n"
        "=98=D0=B5 =D0=B7=D0=B0=D0=BC=D0=B8=D1=88=D1=99=D0=B5=D0=BD =D0=B4=D0=B0 =D1=\r\n"
        "=81=D0=B5\r\n"
        "=D0=B1=D0=B0=D0=B2=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=\r\n"
        "=D0=B0=D1=9A=D0=B5=D0=BC =D1=82=D0=B5=D0=BA=D1=81=D1=82=D0=B0.\r\n"
        "\r\n"
        "\r\n"
        "=D0=A3 =D1=81=D0=B2=D0=B0=D0=BA=D0=BE=D0=BC =D1=81=D0=BB=D1=83=D1=87=D0=B0=\r\n"
        "=D1=98=D1=83, =D0=BF=D0=BE=D1=81=D0=BB=D0=B5 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B5 =D0=BB=D0=B0=D1=82=D0=B8=D0=BD=D0=B8=D1=86=D0=B5 =D1=82=D1=80=\r\n"
        "=D0=B5=D0=B1=D0=B0 =D1=83=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D0=B8 =D0=BF=\r\n"
        "=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D1=83 utf8 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=\r\n"
        "=82=D0=B5=D1=80=D0=B0 =D0=BE=D0=B4=D0=BD. =D1=9B=D0=B8=D1=80=D0=B8=D0=BB=D0=\r\n"
        "=B8=D1=86=D0=B5\r\n"
        "=D0=B8 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=D0=B0=D0=BA=D0=BE =D1=81=\r\n"
        "=D0=B5 =D0=BF=D1=80=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0 =D1=82=D0=B5=D0=BA=D1=81=\r\n"
        "=D1=82 =D0=BA=D0=B0=D0=B4=D0=B0 =D1=81=D1=83 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=\r\n"
        "=D1=82=D0=B5=D1=80=D0=B8 =D0=B2=D0=B8=D1=88=D0=B5=D0=B1=D0=B0=D1=98=D1=82=\r\n"
        "=D0=BD=D0=B8. =D0=A2=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B1=D0=B8 =D0=\r\n"
        "=B4=D0=B0 =D1=98=D0=B5 =D0=BD=D0=B5=D0=B1=D0=B8=D1=82=D0=BD=D0=BE =D0=B4=D0=\r\n"
        "=B0 =D0=BB=D0=B8 =D1=98=D0=B5 =D0=B5=D0=BD=D0=BA=D0=BE=D0=B4=D0=B8=D0=BD=D0=\r\n"
        "=B3\r\n"
        "base64 =D0=B8=D0=BB=D0=B8 quoted printable, =D1=98=D0=B5=D1=80 =D1=81=D0=B5 =\r\n"
        "ascii =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=82=D0=B5=D1=80=D0=B8 =D0=BF=D1=80=\r\n"
        "=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0=D1=98=D1=83 =D1=83 =D0=BD=D0=BE=D0=B2=D0=B5 =\r\n"
        "=D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B5. =D0=9E=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=\r\n"
        "=D1=81=D1=82 =D0=B1=D0=B8 =D1=82=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B4=\r\n"
        "=D0=B0\r\n"
        "=D0=BF=D0=BE=D0=BA=D0=B0=D0=B6=D0=B5 =D0=B8=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=B1=\r\n"
        "=D0=B0=D0=B3=D0=BE=D0=B2=D0=B0 =D1=83 =D0=BB=D0=BE=D0=B3=D0=B8=D1=86=D0=B8 =\r\n"
        "=D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=D0=B0=D1=9A=D0=B0,\r\n"
        " =D0=B0 =D0=B8=D1=81=D1=82=D0=BE =D1=82=D0=BE =D1=82=D1=80=D0=B5=D0=B1=D0=\r\n"
        "=B0 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D0=B8=D1=82=D0=B8 =D1=81=D0=B0 =D0=\r\n"
        "=BF=D0=B0=D1=80=D1=81=D0=B8=D1=80=D0=B0=D1=9A=D0=B5=D0=BC.\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=B4=D0=B5 =D1=98=D0=B5 =D0=B8 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B0 =D0=B7=D0=B0 =D0=BD=D0=B8=D0=B7 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=\r\n"
        "=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0.\r\n"
        "\r\n"
        "--mybnd\r\n"
        "Content-Type: text/plain; name=\"TomislavKarastojkovic_CV.txt\"\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "Content-Disposition: attachment; filename=\"TomislavKarastojkovic_CV.txt\"\r\n"
        "\r\n"
        "VG9taXNsYXYgS2FyYXN0b2prb3ZpxIcgQ1YK\r\n"
        "\r\n"
        "--mybnd--\r\n");
}


/**
Attaching a text file together with an HTML message content.

@pre  File `cv.txt` in the current directory.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_html_att)
{
    message msg;
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("format html attachment");
    msg.boundary("mybnd");
    msg.content_type(message::media_type_t::TEXT, "html", "utf-8");
    msg.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    msg.content("<h1>Naslov</h1><p>Ovo je poruka.</p>");

    std::ifstream ifs1("cv.txt");
    std::list<std::tuple<std::istream&, string, message::content_type_t>> atts;
    message::content_type_t ct(message::media_type_t::TEXT, "plain");
    auto tp = std::tie(ifs1, "TomislavKarastojkovic_CV.txt", ct);
    atts.push_back(tp);
    msg.attach(atts);
    string msg_str;
    msg.format(msg_str);

    BOOST_CHECK(msg.parts().size() == 2);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/mixed; charset=utf-8; boundary=\"mybnd\"\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "Subject: format html attachment\r\n"
        "\r\n"
        "--mybnd\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "<h1>Naslov</h1><p>Ovo je poruka.</p>\r\n"
        "\r\n"
        "--mybnd\r\n"
        "Content-Type: text/plain; name=\"TomislavKarastojkovic_CV.txt\"\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "Content-Disposition: attachment; filename=\"TomislavKarastojkovic_CV.txt\"\r\n"
        "\r\n"
        "VG9taXNsYXYgS2FyYXN0b2prb3ZpxIcgQ1YK\r\n"
        "\r\n"
        "--mybnd--\r\n");
}


/**
Formatting a message without content type.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_mime_no_content_type)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("mime no content type");
    msg.boundary("my_bound");

    mime m1;
    m1.content_type(message::media_type_t::TEXT, "html", "us-ascii");
    m1.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_7);
    m1.content("<html><head></head><body><h1>Hello, World!</h1></body></html>");

    mime m2;
    m2.content_type(message::media_type_t::TEXT, "plain", "us-ascii");
    m2.content_transfer_encoding(mime::content_transfer_encoding_t::BIT_7);
    m2.content("Zdravo, Svete!");

    msg.add_part(m1);
    msg.add_part(m2);
    string msg_str;
    BOOST_CHECK_THROW(msg.format(msg_str), message_error);
}


/**
Formatting a message with the disposition notification.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_notification)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.header_codec(message::header_codec_t::BASE64);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.disposition_notification(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format notification");
    msg.content("Hello, World!");
    string msg_str;
    msg.format(msg_str);

    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Disposition-Notification-To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: format notification\r\n"
        "\r\n"
        "Hello, World!\r\n");
}


/**
Formatting a message with UTF-8 addresses by using Base64 Q codec.

The line policy has to be mandatory because of the third recipient.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_qb_sender)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.header_codec(message::header_codec_t::BASE64);
    msg.from(mail_address("маилио", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("Tomislav Karastojković", "qwerty@gmail.com"));
    msg.add_recipient(mail_address("Томислав Карастојковић", "asdfg@zoho.com"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("proba");
    msg.content("test");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: =?UTF-8?B?0LzQsNC40LvQuNC+?= <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>,\r\n"
        "  =?UTF-8?B?VG9taXNsYXYgS2FyYXN0b2prb3ZpxIc=?= <qwerty@gmail.com>,\r\n"
        "  =?UTF-8?B?0KLQvtC80LjRgdC70LDQsiDQmtCw0YDQsNGB0YLQvtGY0LrQvtCy0LjRmw==?= <asdfg@zoho.com>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: proba\r\n"
        "\r\n"
        "test\r\n");
}


/**
Formatting a message with UTF-8 addresses by using Quoted Printable Q codec.

The line policy has to be mandatory because of the third recipient.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_qq_sender)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("маилио", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("Tomislav Karastojković", "qwerty@gmail.com"));
    msg.add_recipient(mail_address("Томислав Карастојковић", "asdfg@zoho.com"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("proba");
    msg.content("test");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: =?UTF-8?Q?=D0=BC=D0=B0=D0=B8=D0=BB=D0=B8=D0=BE?= <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>,\r\n"
        "  =?UTF-8?Q?Tomislav_Karastojkovi=C4=87?= <qwerty@gmail.com>,\r\n"
        "  =?UTF-8?Q?=D0=A2=D0=BE=D0=BC=D0=B8=D1=81=D0=BB=D0=B0=D0=B2_=D0=9A=D0=B0=D1=80=D0=B0=D1=81=D1=82=D0=BE=D1=98=D0=BA=D0=BE=D0=B2=D0=B8=D1=9B?= <asdfg@zoho.com>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: proba\r\n"
        "\r\n"
        "test\r\n");
}


/**
Formatting a message with UTF-8 subject by using Base64 Q codec.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_qb_long_subject)
{
    message msg;
    msg.header_codec(message::header_codec_t::BASE64);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("Re: Σχετ: Request from GrckaInfo visitor - Eleni Beach Apartments");
    msg.content("Hello, Sithonia!");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: =?UTF-8?B?UmU6IM6jz4fOtc+EOiBSZXF1ZXN0IGZyb20gR3Jja2FJbmZvIHZpc2l0b3IgLSBF?=\r\n"
        " =?UTF-8?B?bGVuaSBCZWFjaCBBcGFydG1lbnRz?=\r\n"
        "\r\n"
        "Hello, Sithonia!\r\n");
}


/**
Formatting a message with UTF-8 subject by using Quoted Printable Q codec.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_qq_long_subject)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("Re: Σχετ: Request from GrckaInfo visitor - Eleni Beach Apartments");
    msg.content("Hello, Sithonia!");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: =?UTF-8?Q?Re:_=CE=A3=CF=87=CE=B5=CF=84:_Request_from_GrckaInfo_visitor_-_E?=\r\n"
        " =?UTF-8?Q?leni_Beach_Apartments?=\r\n"
        "\r\n"
        "Hello, Sithonia!\r\n");
}


/**
Formatting a message with UTF-8 subject containing the long dash character.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_qq_subject_dash)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject(u8"C++ Annotated: Sep \u2013 Dec 2017");
    msg.content("test");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: =?UTF-8?Q?C++_Annotated:_Sep_=E2=80=93_Dec_2017?=\r\n"
        "\r\n"
        "test\r\n");
}


/**
Formatting a message with UTF-8 subject containing an emoji character.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_qq_subject_emoji)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject(u8"\U0001F381\u017Divi godinu dana na ra\u010Dun Super Kartice");
    msg.content("test");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: =?UTF-8?Q?=F0=9F=8E=81=C5=BDivi_godinu_dana_na_ra=C4=8Dun_Super_Kartice?=\r\n"
        "\r\n"
        "test\r\n");
}


/**
Formatting UTF8 subject in 8bit encoding.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_utf8_subject)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.from(mail_address("Tomislav Karastojković", "qwerty@hotmail.com"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("Здраво, Свете!");
    msg.content("Hello, World!");
    msg.header_codec(mime::header_codec_t::UTF8);
    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: Tomislav Karastojković <qwerty@hotmail.com>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Здраво, Свете!\r\n"
        "\r\n"
        "Hello, World!\r\n");
}


/**
Formatting a message with UTF-8 raw subject by using Base64 Q codec.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_qb_utf8_subject_raw)
{
    message msg;
    msg.header_codec(message::header_codec_t::BASE64);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject_raw(string_t("Re: Σχετ: Request from GrckaInfo visitor - Eleni Beach Apartments", "utf-8"));
    msg.content("Hello, Sithonia!");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: =?UTF-8?B?UmU6IM6jz4fOtc+EOiBSZXF1ZXN0IGZyb20gR3Jja2FJbmZvIHZpc2l0b3IgLSBF?=\r\n"
        " =?UTF-8?B?bGVuaSBCZWFjaCBBcGFydG1lbnRz?=\r\n"
        "\r\n"
        "Hello, Sithonia!\r\n");
}


/**
Formatting a message with the message ID in the strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_message_id)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.strict_mode(true);
    msg.date_time(ldt);
    msg.subject("Proba");
    msg.content("Zdravo, Svete!");
    msg.message_id("1234567890@mailio.dev");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID: <1234567890@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n");
}


/**
Formatting the message ID without the monkey character in the strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_message_id_no_monkey_strict)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.strict_mode(true);
    msg.date_time(ldt);
    msg.subject("Proba");
    msg.content("Zdravo, Svete!");
    BOOST_CHECK_THROW(msg.message_id("1234567890mailio.dev"), message_error);
}


/**
Formatting the message ID without the monkey character in the non-strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_message_id_no_monkey_non_strict)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("Proba");
    msg.content("Zdravo, Svete!");
    msg.message_id("1234567890mailio.dev");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID: <1234567890mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n");
}


/**
Formatting the message ID with the space character in the strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_message_id_with_space_strict)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.strict_mode(true);
    msg.date_time(ldt);
    msg.subject("Proba");
    msg.content("Zdravo, Svete!");
    BOOST_CHECK_THROW(msg.message_id("1234567890@ mailio.dev"), message_error);
}


/**
Formatting the message ID with the space character in the non-strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_message_id_with_space_non_strict)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("Proba");
    msg.content("Zdravo, Svete!");
    msg.message_id("1234567890@ mailio.dev");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID: <1234567890@ mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n");
}


/**
Formatting a message with the in-reply-to and references IDs.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_in_reply_to)
{
    message msg;
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("Proba");
    msg.content("Zdravo, Svete!");
    msg.add_in_reply_to("1@mailio.dev");
    msg.add_in_reply_to("22@mailio.dev");
    msg.add_in_reply_to("333@mailio.dev");
    msg.add_references("4444@mailio.dev");
    msg.add_references("55555@mailio.dev");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "In-Reply-To: <1@mailio.dev> <22@mailio.dev> <333@mailio.dev>\r\n"
        "References: <4444@mailio.dev> <55555@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n");
}


/**
Formatting oversized recipient with the recommended line policy.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_recommended_recipient)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::RECOMMENDED);
    msg.header_codec(message::header_codec_t::BASE64);
    msg.from(mail_address("маилио", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("Tomislav Karastojković", "qwerty@gmail.com"));
    msg.add_recipient(mail_address("Томислав Карастојковић", "asdfg@zoho.com"));
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("proba");
    msg.content("test");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "From: =?UTF-8?B?0LzQsNC40LvQuNC+?= <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>,\r\n"
        "  =?UTF-8?B?VG9taXNsYXYgS2FyYXN0b2prb3ZpxIc=?= <qwerty@gmail.com>,\r\n"
        "  =?UTF-8?B?0KLQvtC80LjRgdC70LDQsiDQmtCw0YDQsNGB0YLQvtGY0LrQvtCy0LjRmw==?=\r\n"
        "  <asdfg@zoho.com>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: proba\r\n"
        "\r\n"
        "test\r\n");
}


/**
Formatting a message with a long header to be folded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_long_header)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev"));
    msg.add_recipient(mail_address("contact", "kontakt@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.add_recipient(mail_group("all", {mail_address("Tomislav", "qwerty@hotmail.com")}));
    msg.subject("Hello, World!");
    msg.content("Hello, World!");
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.add_header("Proba", "1234567890123456789 01234567890123456789012345678901234567890123456789012345678901234567890 12345678901234567890@mailio.dev");

    string msg_str;
    msg.format(msg_str);
    BOOST_CHECK(msg_str == "Proba: 1234567890123456789\r\n"
        "  01234567890123456789012345678901234567890123456789012345678901234567890\r\n"
        "  12345678901234567890@mailio.dev\r\n"
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <kontakt@mailio.dev>\r\n"
        "To: contact <kontakt@mailio.dev>,\r\n"
        "  mailio <adresa@mailio.dev>,\r\n"
        "  all: Tomislav <qwerty@hotmail.com>;\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: Hello, World!\r\n"
        "\r\n"
        "Hello, World!\r\n");
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


/**
Parsing by lines an oversized line.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_by_line_oversized)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::RECOMMENDED);
    msg.parse_by_line("From: mailio <adresa@mailio.dev>");
    msg.parse_by_line("To: mailio");
    msg.parse_by_line("Subject: parse by line oversized");
    msg.parse_by_line("");
    BOOST_CHECK_THROW(msg.parse_by_line("01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n"), mime_error);
}


/**
Parsing by lines an oversized line Base64 encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_base64_line_oversized)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::RECOMMENDED);
    msg.parse_by_line("From: mailio <adresa@mailio.dev>");
    msg.parse_by_line("To: mailio <adresa@mailio.dev>");
    msg.parse_by_line("Date: Fri, 17 Jan 2014 05:39:22 -0730");
    msg.parse_by_line("Content-Type: text/plain");
    msg.parse_by_line("Content-Transfer-Encoding: Base64");
    msg.parse_by_line("Subject: parse base64 line oversized");
    msg.parse_by_line("");
    BOOST_CHECK_THROW(msg.parse_by_line("T3ZvIGplIGpha28gZHVnYWNoa2EgcG9ydWthIGtvamEgaW1hIGkgcHJhem5paCBsaW5pamEgaSBwcmVkdWdhY2hraWggbGluaWphLg==\r\n"), mime_error);
}


/**
Parsing addresses and groups from the header.

Multiple addresses in a header are in separated lines, some of them are contain additional spaces.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_addresses)
{
    string msg_str = "From: mail io <adresa@mailio.dev>\r\n"
        "To: info,\r\n"
        "  kontakt@mailio.dev,\r\n"
        "  all, \r\n"
        "  mail io <adresa@mailio.dev>\r\n"
        "Cc: all: Tomislav <qwerty@mailio.dev>, \r\n"
        "  \"Tomislav Karastojkovic\" <asdfgh@mailio.dev>; \r\n"
        "  adresa@mailio.dev,\r\n"
        "  undisclosed-recipients:;\r\n"
        "  qwerty@gmail.com,\r\n"
        "  \"Tomislav\" <qwerty@hotmail.com>,\r\n"
        "  <qwerty@zoho.com>, \r\n"
        "  mailio: qwerty@outlook.com;\r\n"
        "Subject: parse addresses\r\n"
        "\r\n"
        "Hello, World!\r\n";
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mail io" && msg.from().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.recipients().addresses.size() == 4 &&
        msg.recipients_to_string() == "info,\r\n  <kontakt@mailio.dev>,\r\n  all,\r\n  mail io <adresa@mailio.dev>" &&
        msg.recipients().addresses.at(0).name == "info" && msg.recipients().addresses.at(1).address == "kontakt@mailio.dev" &&
        msg.recipients().addresses.at(2).name == "all" &&
        msg.recipients().addresses.at(3).name == "mail io" && msg.recipients().addresses.at(3).address == "adresa@mailio.dev" &&
        msg.cc_recipients().addresses.size() == 4 && msg.cc_recipients().groups.size() == 3 &&
        msg.cc_recipients().groups.at(0).name == "all" && msg.cc_recipients().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.cc_recipients().groups.at(1).name == "undisclosed-recipients" &&
        msg.cc_recipients().addresses.at(2).name == "Tomislav" && msg.cc_recipients().addresses.at(2).address == "qwerty@hotmail.com" &&
        msg.cc_recipients().groups.at(2).name == "mailio" && msg.cc_recipients().groups.at(2).members.size() == 1 &&
        msg.subject() == "parse addresses" && msg.content() == "Hello, World!");
}


/**
Parsing address not separated by space from name.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_address_no_space)
{
    string msg_str = "From: mail io<adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: proba\r\n"
        "\r\n"
        "test\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mail io" && msg.from().addresses.at(0).address == "adresa@mailio.dev");
}


/**
Parsing the name and address without brackets.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_bad_author_address)
{
    string msg_str = "From: mailio adresa@mailio.dev\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: parse bad author address\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "\r\n"
        "hello\r\n";
    message msg;

    BOOST_CHECK_THROW(msg.parse(msg_str), message_error);
}


/**
Parsing a message without the author address.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_no_author_address)
{
    string msg_str = "Sender: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: parse no author address\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "\r\n"
        "hello\r\n";
    message msg;

    BOOST_CHECK_THROW(msg.parse(msg_str), message_error);
}


/**
Parsing a message with a wrong recipient mail group.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_bad_mail_group)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: all: karas@mailio.dev\r\n"
        "Subject: parse bad mail group\r\n"
        "\r\n"
        "hello\r\n";
    message msg;

    BOOST_CHECK_THROW(msg.parse(msg_str), message_error);
}


/**
Parsing a wrong recipient address.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_bad_recipient_address)
{
    string msg_str = "From: maill.io@mailio.dev\r\n"
        "To: <mailio>\r\n"
        "Subject: parse bad recipient address\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "\r\n"
        "hello\r\n";
    message msg;

    BOOST_CHECK_THROW(msg.parse(msg_str), message_error);
}


/**
Parsing oversized recipients with the recommended line policy.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_recommended_address)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <kontakt@mailio.dev>\r\n"
        "To: contact <kontakt@mailio.dev>, Tomislav Karastojkovic <karas@mailio.dev>, Tomislav Karastojkovic <qwerty@gmail.com>, "
        "Tomislav Karastojkovic <asdfg@zoho.com>\r\n"
        "Cc: mail.io <adresa@mailio.dev>, Tomislav Karastojkovic <zxcvb@yahoo.com>\r\n"
        "Date: Wed, 23 Aug 2017 22:16:45 +0000\r\n"
        "Subject: parse recommended address\r\n"
        "\r\n"
        "Hello, World!\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::RECOMMENDED);
    BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
}


/**
Parsing quoted address not separated by space from name.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_quoted_address_no_space)
{
    string msg_str = "From: \"mail io\"<adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: proba\r\n"
        "\r\n"
        "test\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mail io" && msg.from().addresses.at(0).address == "adresa@mailio.dev");
}


/**
Parsing addresses in a single line which contains trailing comment.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_address_comment)
{
    string msg_str = "From: mailio <adresa@mailio.dev> (Mail Delivery System)\r\n"
        "To: adresa@mailio.dev, all: qwerty@gmail.com, Karas <asdfgh@mailio.dev>; Tomislav <qwerty@hotmail.com> (The comment)\r\n"
        "Subject: parse address comment\r\n"
        "\r\n"
        "Hello, World!";
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);

    BOOST_CHECK(msg.from().addresses.at(0).name == "mailio" && msg.from().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.recipients().addresses.size() == 2 && msg.recipients().groups.size() == 1 && msg.recipients().groups.at(0).members.size() == 2);
}


/**
Parsing address as name in the strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_double_address_strict)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: aaa@mailio.dev <bbb@mailio.dev>\r\n"
        "Subject: parse double address strict\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.strict_mode(true);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    BOOST_CHECK_THROW(msg.parse(msg_str), message_error);
}


/**
Parsing address as name in the non-strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_double_address_non_strict)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: aaa@mailio.dev <bbb@mailio.dev>\r\n"
        "Subject: parse double address non strict\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.strict_mode(false);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.recipients().addresses.at(0).name == "aaa@mailio.dev" && msg.recipients().addresses.at(0).address == "bbb@mailio.dev");
}


/**
Parsing the address without the monkey.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_address_without_monkey)
{
    string msg_str =
        "From: recipients \"undisclosed recipients: ;\"\r\n"
        "To: recipients \"undisclosed recipients: ;\"\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: parse address without monkey\r\n"
        "\r\n"
        "test\r\n";
    message msg;
    msg.strict_mode(false);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    auto from = msg.from().addresses.at(0);
    auto rcpt = msg.recipients().addresses.at(0);
    BOOST_CHECK(from.name == "recipients undisclosed recipients: ;" && rcpt.name == "recipients undisclosed recipients: ;");
}


/**
Parsing the content type which follows the specification.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_content_type)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
        "To: adresa@mailio.dev\r\n"
        "Subject: parse content type\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.content_type().type == mailio::mime::media_type_t::TEXT && msg.content_type().subtype == "plain" && msg.content_type().charset == "utf-8");
}


/**
Parsing the content type which does not follow the specification, in both strict and non-strict modes.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_malformed_content_type)
{
    const string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Content-Type: text/plain; charset       =   \"UTF-8\"\r\n"
        "To: adresa@mailio.dev\r\n"
        "Subject: parse strict content type\r\n"
        "\r\n"
        "Hello, World!";

    {
        message msg;
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        msg.strict_mode(true);
        BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
    }

    {
        message msg;
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        msg.strict_mode(false);
        msg.parse(msg_str);
        BOOST_CHECK(msg.content_type().type == mailio::mime::media_type_t::TEXT && msg.content_type().subtype == "plain" && msg.content_type().charset == "utf-8");
    }
}


/**
Parsing the content type with an attribute containing the backslash in the non-strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_attribute_backslash_non_strict)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Content-Type: application/octet-stream; name=217093469\\container_0_LOGO\r\n"
        "To: adresa@mailio.dev\r\n"
        "Subject: parse attribute backslash non strict\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.strict_mode(false);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.content_type().type == mailio::mime::media_type_t::APPLICATION && msg.content_type().subtype == "octet-stream");
}


/**
Parsing the content type with an attribute containing the backslash in the strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_attribute_backslash_strict)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Content-Type: application/octet-stream; name=217093469\\container_0_LOGO\r\n"
        "To: adresa@mailio.dev\r\n"
        "Subject: parse attribute backslash strict\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.strict_mode(true);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
}


/**
Parsing the content disposition with an attribute with the quoted value containing the backslash.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_quoted_attribute_backslash)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Disposition: attachment; \r\n"
        "  filename=\"C:\\Windows\\mailio.ini\"\r\n"
        "To: adresa@mailio.dev\r\n"
        "Subject: parse quoted attribute backslash\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.strict_mode(false);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.content_type().type == mailio::mime::media_type_t::TEXT && msg.content_type().subtype == "plain");
}


/**
Parsing the filename as a continued attribute.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_continued_filename)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Disposition: attachment; \r\n"
        "  filename*0=\"C:\\Program Files\\AlephoLtd\"; \r\n"
        "  filename*1=\"\\mailio\\configuration.ini\"\r\n"
        "To: adresa@mailio.dev\r\n"
        "Subject: parse address comment\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.strict_mode(false);
    msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::RECOMMENDED);
    msg.parse(msg_str);
    BOOST_CHECK(msg.name() == "C:\\Program Files\\AlephoLtd\\mailio\\configuration.ini");
}


/**
Parsing a header split into two lines.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_multiline_header)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID: <1234567890123456789012345678901234567890\r\n"
        " 12345678901234567890@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: parse multiline header\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    message msg;
    msg.strict_mode(false);
    msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::RECOMMENDED);
    msg.parse(msg_str);
    BOOST_CHECK(msg.message_id() == "<123456789012345678901234567890123456789012345678901234567890@mailio.dev>");
}


/**
Parsing a header exceeding the line policy.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_long_header)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID: <123456789012345678901234567890123456789012345678901234567890@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: parse long header\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    message msg;
    msg.strict_mode(false);
    msg.line_policy(codec::line_len_policy_t::RECOMMENDED, codec::line_len_policy_t::RECOMMENDED);
    BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
}


/**
Parsing multiline content with lines containing leading dots, with the escaping dot flag on.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_dotted_esc)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: parse dotted escape\r\n"
        "\r\n"
        "..Hello, World!\r\n"
        "opa bato\r\n"
        "...proba\r\n"
        "\r\n"
        "..\r\n"
        "\r\n"
        "yaba.daba.doo.\r\n"
        "\r\n"
        "...\r\n"
        "\r\n";

    message msg;
    msg.parse(msg_str, true);
    BOOST_CHECK(msg.content() == ".Hello, World!\r\n"
        "opa bato\r\n"
        "..proba\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "yaba.daba.doo.\r\n"
        "\r\n"
        "..");
}


/**
Parsing long plain text with default charset (ASCII) default encoded (Seven Bit) with the recommended length.

Except the trailing ones, CRLF characters are preserved.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_long_text_default_default)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>, <qwerty@gmail.com>\r\n"
        "Date: Fri, 12 Feb 2016 13:22:22 +0100\r\n"
        "Content-Type: text/plain\r\n"
        "Subject: parse long text default default\r\n"
        "\r\n"
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. N\r\n"
        "ije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe, je\r\n"
        "r libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n"
        "\r\n\r\n\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 karakte\r\n"
        "ra odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da je\r\n"
        " nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije. O\r\n"
        "vaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici parsiranja,\r\n"
        " a isto to treba proveriti sa formatiranjem.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n\r\n\r\n";

    message msg;
    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() == "parse long text default default" && msg.content_type().type == mime::media_type_t::TEXT &&
        msg.content_type().subtype == "plain" && msg.content_type().charset.empty() &&
        msg.content_transfer_encoding() == mime::content_transfer_encoding_t::NONE);
    BOOST_CHECK(msg.content() ==
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. N\r\n"
        "ije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe, je\r\n"
        "r libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n"
        "\r\n\r\n\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 karakte\r\n"
        "ra odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da je\r\n"
        " nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije. O\r\n"
        "vaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici parsiranja,\r\n"
        " a isto to treba proveriti sa formatiranjem.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.");
}


/**
Parsing long default content (plain text ASCII charset) Base64 encoded with the recommended length.

The encoded text contains CRLF characters which are present when text is decoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_long_text_default_base64)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: parse long text default base64\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "\r\n"
        "T3ZvIGplIGpha28gZHVnYWNoa2EgcG9ydWthIGtvamEgaW1hIGkgcHJhem5paCBsaW5pamEgaSBw\r\n"
        "cmVkdWdhY2hraWggbGluaWphLiBODQppamUgamFzbm8ga2FrbyBjZSBzZSB0ZWtzdCBwcmVsb21p\r\n"
        "dGkNCnBhIHNlIG5hZGFtIGRhIGNjZSB0byBvdmFqIHRlc3QgcG9rYXphdGkuDQoNClRyZWJhIHZp\r\n"
        "ZGV0aSBrYWtvIHBvem5hdGkgbWVqbCBrbGlqZW50aSBsb21lIHRla3N0LCBwYSBuYQ0Kb3Nub3Z1\r\n"
        "IHRvZ2EgZG9yYWRpdGkgZm9ybWF0aXJhbmplIHNhZHJ6emFqYSBtZWpsYS4gQSBtb3p6ZGEgaSBu\r\n"
        "ZW1hIHBvdHJlYmUsIGplDQpyIGxpYm1haWxpbyBuaWplIHphbWlzaGxqZW4gZGEgc2UNCmJhdmkg\r\n"
        "Zm9ybWF0aXJhbmplbSB0ZWtzdGEuDQoNCg0KDQoNClUgc3Zha29tIHNsdWNoYWp1LCBwb3NsZSBw\r\n"
        "cm92ZXJlIGxhdGluaWNlIHRyZWJhIHVyYWRpdGkgaSBwcm92ZXJ1IHV0Zjgga2FyYWt0ZQ0KcmEg\r\n"
        "b2RuLiBjY2lyaWxpY2UNCmkgdmlkZXRpIGtha28gc2UgcHJlbGFtYSB0ZWtzdCBrYWRhIHN1IGth\r\n"
        "cmFrdGVyaSB2aXNoZWJhanRuaS4gVHJlYmFsbyBiaSBkYSBqZQ0KIG5lYml0bm8gZGEgbGkgamUg\r\n"
        "ZW5rb2RpbmcNCmJhc2U2NCBpbGkgcXVvdGVkIHByaW50YWJsZSwgamVyIHNlIGFzY2lpIGthcmFr\r\n"
        "dGVyaSBwcmVsYW1hanUgdSBub3ZlIGxpbmlqZS4gTw0KdmFqIHRlc3QgYmkgdHJlYmFsbyBkYQ0K\r\n"
        "cG9rYXp6ZSBpbWEgbGkgYmFnb3ZhIHUgbG9naWNpIHBhcnNpcmFuamEsDQogYSBpc3RvIHRvIHRy\r\n"
        "ZWJhIHByb3Zlcml0aSBzYSBmb3JtYXRpcmFuamVtLg0KDQoNCg0KDQpPdmRlIGplIGkgcHJvdmVy\r\n"
        "YSB6YSBuaXogcHJhem5paCBsaW5pamEuDQoNCg0K";

    message msg;
    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() == "parse long text default base64" && msg.content_type().type == mime::media_type_t::NONE &&
        msg.content_type().subtype.empty() && msg.content_type().charset.empty() &&
        msg.content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64);
    BOOST_CHECK(msg.content() ==
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. N\r\n"
        "ije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe, je\r\n"
        "r libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n"
        "\r\n\r\n\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 karakte\r\n"
        "ra odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da je\r\n"
        " nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije. O\r\n"
        "vaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici parsiranja,\r\n"
        " a isto to treba proveriti sa formatiranjem.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n\r\n\r\n");
}


/**
Parsing long default content (plain text ASCII charset) Quoted Printable encoded with the recommended length.

Soft breaks are used to concatenate lines, other CRLF characters are preserved.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_long_text_default_qp)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: parse long text default quoted printable\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija=\r\n"
        ". Nije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj tes=\r\n"
        "t pokazati.\r\n"
        "\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, =\r\n"
        "pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i ne=\r\n"
        "ma potrebe, jer libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem tek=\r\n"
        "sta.\r\n"
        "\r\n"
        "\r\n"
        "U svakom sluchaju, posle provere latinice treba uradi=\r\n"
        "ti i proveru utf8 karaktera odn. ccirilice\r\n"
        "i videti kako se prelama te=\r\n"
        "kst kada su karakteri vishebajtni. Trebalo bi da je nebitno da li je enkodi=\r\n"
        "ng\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nov=\r\n"
        "e linije. Ovaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici forma=\r\n"
        "tiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "Ovde je i provera za niz praznih linija.\r\n"
        "\r\n"
        "\r\n";

    message msg;
    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() == "parse long text default quoted printable" && msg.content_type().type == mime::media_type_t::NONE &&
        msg.content_type().subtype.empty() && msg.content_type().charset.empty() &&
        msg.content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    BOOST_CHECK(msg.content() == "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. Nije jasno kako ce se tekst prelomiti\r\n"
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
        "Ovde je i provera za niz praznih linija.");
}


/**
Parsing long text with UTF-8 charset Base64 encoded with the recommended length.

The encoded text contains CRLF characters which are present when text is decoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_long_text_utf8_base64)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Subject: parse long text utf8 base64\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n"
        "0J7QstC+INGY0LUg0ZjQsNC60L4g0LTRg9Cz0LDRh9C60LAg0L/QvtGA0YPQutCwINC60L7RmNCw\r\n"
        "INC40LzQsCDQuCDQv9GA0LDQt9C90LjRhSDQu9C40L3QuNGY0LAg0Lgg0L/RgNC10LTRg9Cz0LDR\r\n"
        "h9C60LjRhSDQu9C40L3QuNGY0LAuINCd0LjRmNC1INGY0LDRgdC90L4g0LrQsNC60L4g0ZvQtSDR\r\n"
        "gdC1INGC0LXQutGB0YIg0L/RgNC10LvQvtC80LjRgtC4DQrQv9CwINGB0LUg0L3QsNC00LDQvCDQ\r\n"
        "tNCwINGb0LUg0YLQviDQvtCy0LDRmCDRgtC10LrRgdGCINC/0L7QutCw0LfQsNGC0LguDQoNCtCi\r\n"
        "0YDQtdCx0LAg0LLQuNC00LXRgtC4INC60LDQutC+INC/0L7Qt9C90LDRgtC4INC80LXRmNC7INC6\r\n"
        "0LvQuNGY0LXQvdGC0Lgg0LvQvtC80LUg0YLQtdC60YHRgiwg0L/QsCDQvdCwDQrQvtGB0L3QvtCy\r\n"
        "0YMg0YLQvtCz0LAg0LTQvtGA0LDQtNC40YLQuCDRhNC+0YDQvNCw0YLQuNGA0LDRmtC1INC80LXR\r\n"
        "mNC70LAuINCQINC80L7QttC00LAg0Lgg0L3QtdC80LAg0L/QvtGC0YDQtdCx0LUsINGY0LXRgCBs\r\n"
        "aWJtYWlsaW8g0L3QuNGY0LUg0LfQsNC80LjRiNGZ0LXQvSDQtNCwINGB0LUNCtCx0LDQstC4INGE\r\n"
        "0L7RgNC80LDRgtC40YDQsNGa0LXQvCDRgtC10LrRgdGC0LAuDQoNCg0K0KMg0YHQstCw0LrQvtC8\r\n"
        "INGB0LvRg9GH0LDRmNGDLCDQv9C+0YHQu9C1INC/0YDQvtCy0LXRgNC1INC70LDRgtC40L3QuNGG\r\n"
        "0LUg0YLRgNC10LHQsCDRg9GA0LDQtNC40YLQuCDQuCDQv9GA0L7QstC10YDRgyB1dGY4INC60LDR\r\n"
        "gNCw0LrRgtC10YDQsCDQvtC00L0uINGb0LjRgNC40LvQuNGG0LUNCtC4INCy0LjQtNC10YLQuCDQ\r\n"
        "utCw0LrQviDRgdC1INC/0YDQtdC70LDQvNCwINGC0LXQutGB0YIg0LrQsNC00LAg0YHRgyDQutCw\r\n"
        "0YDQsNC60YLQtdGA0Lgg0LLQuNGI0LXQsdCw0ZjRgtC90LguINCi0YDQtdCx0LDQu9C+INCx0Lgg\r\n"
        "0LTQsCDRmNC1INC90LXQsdC40YLQvdC+INC00LAg0LvQuCDRmNC1INC10L3QutC+0LTQuNC90LMN\r\n"
        "CmJhc2U2NCDQuNC70LggcXVvdGVkIHByaW50YWJsZSwg0ZjQtdGAINGB0LUgYXNjaWkg0LrQsNGA\r\n"
        "0LDQutGC0LXRgNC4INC/0YDQtdC70LDQvNCw0ZjRgyDRgyDQvdC+0LLQtSDQu9C40L3QuNGY0LUu\r\n"
        "INCe0LLQsNGYINGC0LXRgdGCINCx0Lgg0YLRgNC10LHQsNC70L4g0LTQsA0K0L/QvtC60LDQttC1\r\n"
        "INC40LzQsCDQu9C4INCx0LDQs9C+0LLQsCDRgyDQu9C+0LPQuNGG0Lgg0YTQvtGA0LzQsNGC0LjR\r\n"
        "gNCw0ZrQsCwNCiDQsCDQuNGB0YLQviDRgtC+INGC0YDQtdCx0LAg0L/RgNC+0LLQtdGA0LjRgtC4\r\n"
        "INGB0LAg0L/QsNGA0YHQuNGA0LDRmtC10LwuDQoNCg0KDQoNCtCe0LLQtNC1INGY0LUg0Lgg0L/R\r\n"
        "gNC+0LLQtdGA0LAg0LfQsCDQvdC40Lcg0L/RgNCw0LfQvdC40YUg0LvQuNC90LjRmNCwLg0KDQoN\r\n"
        "Cg==\r\n";

    message msg;
    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() == "parse long text utf8 base64" && msg.content_type().type == mime::media_type_t::TEXT &&
        msg.content_type().subtype == "plain" && msg.content_type().charset == "utf-8" &&
        msg.content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64);
    BOOST_CHECK(msg.content() == "Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
        "па се надам да ће то овај текст показати.\r\n"
        "\r\n"
        "Треба видети како познати мејл клијенти ломе текст, па на\r\n"
        "основу тога дорадити форматирање мејла. А можда и нема потребе, јер libmailio није замишљен да се\r\n"
        "бави форматирањем текста.\r\n"
        "\r\n\r\n"
        "У сваком случају, после провере латинице треба урадити и проверу utf8 карактера одн. ћирилице\r\n"
        "и видети како се прелама текст када су карактери вишебајтни. Требало би да је небитно да ли је енкодинг\r\n"
        "base64 или quoted printable, јер се ascii карактери преламају у нове линије. Овај тест би требало да\r\n"
        "покаже има ли багова у логици форматирања,\r\n"
        " а исто то треба проверити са парсирањем.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Овде је и провера за низ празних линија.\r\n\r\n\r\n");
}


/**
Parsing long text with UTF-8 charset Quoted Printable encoded with the recommended length.

Soft breaks are used to concatenate lines, other CRLF characters are preserved.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_long_text_utf8_qp)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Subject: parse long text utf8 quoted printable\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=BE =D1=98=D0=B5 =D1=98=D0=B0=D0=BA=D0=BE =D0=B4=D1=83=D0=B3=\r\n"
        "=D0=B0=D1=87=D0=BA=D0=B0 =D0=BF=D0=BE=D1=80=D1=83=D0=BA=D0=B0 =D0=BA=D0=BE=\r\n"
        "=D1=98=D0=B0 =D0=B8=D0=BC=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=D0=B8=\r\n"
        "=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B5=D0=B4=\r\n"
        "=D1=83=D0=B3=D0=B0=D1=87=D0=BA=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=\r\n"
        "=D0=B0. =D0=9D=D0=B8=D1=98=D0=B5 =D1=98=D0=B0=D1=81=D0=BD=D0=BE =D0=BA=D0=\r\n"
        "=B0=D0=BA=D0=BE =D1=9B=D0=B5 =D1=81=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82 =\r\n"
        "=D0=BF=D1=80=D0=B5=D0=BB=D0=BE=D0=BC=D0=B8=D1=82=D0=B8\r\n"
        "=D0=BF=D0=B0 =D1=81=D0=B5 =D0=BD=D0=B0=D0=B4=D0=B0=D0=BC =D0=B4=D0=B0 =D1=\r\n"
        "=9B=D0=B5 =D1=82=D0=BE =D0=BE=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=D0=BA=D1=81=D1=\r\n"
        "=82 =D0=BF=D0=BE=D0=BA=D0=B0=D0=B7=D0=B0=D1=82=D0=B8.\r\n"
        "\r\n"
        "=D0=A2=D1=80=D0=B5=D0=B1=D0=B0 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=\r\n"
        "=D0=B0=D0=BA=D0=BE =D0=BF=D0=BE=D0=B7=D0=BD=D0=B0=D1=82=D0=B8 =D0=BC=D0=B5=\r\n"
        "=D1=98=D0=BB =D0=BA=D0=BB=D0=B8=D1=98=D0=B5=D0=BD=D1=82=D0=B8 =D0=BB=D0=BE=\r\n"
        "=D0=BC=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82, =D0=BF=D0=B0 =D0=BD=D0=B0\r\n"
        "=D0=BE=D1=81=D0=BD=D0=BE=D0=B2=D1=83 =D1=82=D0=BE=D0=B3=D0=B0 =D0=B4=D0=BE=\r\n"
        "=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=\r\n"
        "=D0=B8=D1=80=D0=B0=D1=9A=D0=B5 =D0=BC=D0=B5=D1=98=D0=BB=D0=B0. =D0=90 =D0=\r\n"
        "=BC=D0=BE=D0=B6=D0=B4=D0=B0 =D0=B8 =D0=BD=D0=B5=D0=BC=D0=B0 =D0=BF=D0=BE=D1=\r\n"
        "=82=D1=80=D0=B5=D0=B1=D0=B5, =D1=98=D0=B5=D1=80 libmailio =D0=BD=D0=B8=D1=\r\n"
        "=98=D0=B5 =D0=B7=D0=B0=D0=BC=D0=B8=D1=88=D1=99=D0=B5=D0=BD =D0=B4=D0=B0 =D1=\r\n"
        "=81=D0=B5\r\n"
        "=D0=B1=D0=B0=D0=B2=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=\r\n"
        "=D0=B0=D1=9A=D0=B5=D0=BC =D1=82=D0=B5=D0=BA=D1=81=D1=82=D0=B0.\r\n"
        "\r\n"
        "\r\n"
        "=D0=A3 =D1=81=D0=B2=D0=B0=D0=BA=D0=BE=D0=BC =D1=81=D0=BB=D1=83=D1=87=D0=B0=\r\n"
        "=D1=98=D1=83, =D0=BF=D0=BE=D1=81=D0=BB=D0=B5 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B5 =D0=BB=D0=B0=D1=82=D0=B8=D0=BD=D0=B8=D1=86=D0=B5 =D1=82=D1=80=\r\n"
        "=D0=B5=D0=B1=D0=B0 =D1=83=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D0=B8 =D0=BF=\r\n"
        "=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D1=83 utf8 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=\r\n"
        "=82=D0=B5=D1=80=D0=B0 =D0=BE=D0=B4=D0=BD. =D1=9B=D0=B8=D1=80=D0=B8=D0=BB=D0=\r\n"
        "=B8=D1=86=D0=B5\r\n"
        "=D0=B8 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=D0=B0=D0=BA=D0=BE =D1=81=\r\n"
        "=D0=B5 =D0=BF=D1=80=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0 =D1=82=D0=B5=D0=BA=D1=81=\r\n"
        "=D1=82 =D0=BA=D0=B0=D0=B4=D0=B0 =D1=81=D1=83 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=\r\n"
        "=D1=82=D0=B5=D1=80=D0=B8 =D0=B2=D0=B8=D1=88=D0=B5=D0=B1=D0=B0=D1=98=D1=82=\r\n"
        "=D0=BD=D0=B8. =D0=A2=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B1=D0=B8 =D0=\r\n"
        "=B4=D0=B0 =D1=98=D0=B5 =D0=BD=D0=B5=D0=B1=D0=B8=D1=82=D0=BD=D0=BE =D0=B4=D0=\r\n"
        "=B0 =D0=BB=D0=B8 =D1=98=D0=B5 =D0=B5=D0=BD=D0=BA=D0=BE=D0=B4=D0=B8=D0=BD=D0=\r\n"
        "=B3\r\n"
        "base64 =D0=B8=D0=BB=D0=B8 quoted printable, =D1=98=D0=B5=D1=80 =D1=81=D0=B5=\r\n"
        " ascii =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=82=D0=B5=D1=80=D0=B8 =D0=BF=D1=80=\r\n"
        "=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0=D1=98=D1=83 =D1=83 =D0=BD=D0=BE=D0=B2=D0=B5 =\r\n"
        "=D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B5. =D0=9E=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=\r\n"
        "=D1=81=D1=82 =D0=B1=D0=B8 =D1=82=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B4=\r\n"
        "=D0=B0\r\n"
        "=D0=BF=D0=BE=D0=BA=D0=B0=D0=B6=D0=B5 =D0=B8=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=B1=\r\n"
        "=D0=B0=D0=B3=D0=BE=D0=B2=D0=B0 =D1=83 =D0=BB=D0=BE=D0=B3=D0=B8=D1=86=D0=B8 =\r\n"
        "=D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=D0=B0=D1=9A=D0=B0,\r\n"
        "=D0=B0 =D0=B8=D1=81=D1=82=D0=BE =D1=82=D0=BE =D1=82=D1=80=D0=B5=D0=B1=D0=B0=\r\n"
        " =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D0=B8=D1=82=D0=B8 =D1=81=D0=B0 =D0=BF=\r\n"
        "=D0=B0=D1=80=D1=81=D0=B8=D1=80=D0=B0=D1=9A=D0=B5=D0=BC.\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=B4=D0=B5 =D1=98=D0=B5 =D0=B8 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B0 =D0=B7=D0=B0 =D0=BD=D0=B8=D0=B7 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=\r\n"
        "=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0.\r\n";

    message msg;
    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() == "parse long text utf8 quoted printable" && msg.content_type().type == mime::media_type_t::TEXT &&
        msg.content_type().subtype == "plain" && msg.content_type().charset == "utf-8" &&
        msg.content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    BOOST_CHECK(msg.content() == "Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
        "па се надам да ће то овај текст показати.\r\n"
        "\r\n"
        "Треба видети како познати мејл клијенти ломе текст, па на\r\n"
        "основу тога дорадити форматирање мејла. А можда и нема потребе, јер libmailio није замишљен да се\r\n"
        "бави форматирањем текста.\r\n"
        "\r\n\r\n"
        "У сваком случају, после провере латинице треба урадити и проверу utf8 карактера одн. ћирилице\r\n"
        "и видети како се прелама текст када су карактери вишебајтни. Требало би да је небитно да ли је енкодинг\r\n"
        "base64 или quoted printable, јер се ascii карактери преламају у нове линије. Овај тест би требало да\r\n"
        "покаже има ли багова у логици форматирања,\r\n"
        "а исто то треба проверити са парсирањем.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Овде је и провера за низ празних линија.");
}


/**
Parsing alternative multipart with the first part HTML with ASCII charset Seven Bit encoded, the second part plain text with UTF-8 charset Base64 encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_multipart_html_ascii_bit7_plain_utf8_base64)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/alternative; boundary=\"my_bound\"\r\n"
        "Subject: parse multipart html ascii bit7 plain utf8 base64\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "<html><head></head><body><h1>Hello, World!</h1></body></html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "0JfQtNGA0LDQstC+LCDQodCy0LXRgtC1IQ==\r\n"
        "\r\n"
        "--my_bound--\r\n";
    msg.parse(msg_str);

    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    BOOST_CHECK(msg.boundary() == "my_bound" && msg.subject() == "parse multipart html ascii bit7 plain utf8 base64" && msg.date_time() == ldt &&
        msg.from_to_string() == "mailio <adresa@mailio.dev>" && msg.recipients().addresses.size() == 1 &&
        msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "alternative" && msg.parts().size() == 2);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::TEXT && msg.parts().at(0).content_type().subtype == "html" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::BIT_7 &&
        msg.parts().at(0).content_type().charset == "us-ascii" &&
        msg.parts().at(0).content() == "<html><head></head><body><h1>Hello, World!</h1></body></html>");
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::TEXT && msg.parts().at(1).content_type().subtype == "plain" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 &&
        msg.parts().at(1).content_type().charset == "utf-8" && msg.parts().at(1).content() == "Здраво, Свете!");
}


/**
Parsing alternative multipart with the first part HTML with ASCII charset Quoted Printable encoded, the second part text with ASCII charset Bit8
encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_multipart_html_ascii_qp_plain_ascii_bit8)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);

    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/alternative; boundary=\"my_bound\"\r\n"
        "Subject: parse multipart html ascii qp plain ascii bit8\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "<html><head></head><body><h1>Hello, World!</h1></body></html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 8bit\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n"
        "\r\n"
        "--my_bound--\r\n";
    msg.parse(msg_str);

    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    BOOST_CHECK(msg.subject() == "parse multipart html ascii qp plain ascii bit8" &&  msg.boundary() == "my_bound" && msg.date_time() == ldt &&
        msg.from_to_string() == "mailio <adresa@mailio.dev>" && msg.recipients().addresses.size() == 1 &&
        msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "alternative" && msg.parts().size() == 2);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::TEXT && msg.parts().at(0).content_type().subtype == "html" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(0).content_type().charset == "us-ascii" && msg.parts().at(0).content() ==
        "<html><head></head><body><h1>Hello, World!</h1></body></html>");
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::TEXT && msg.parts().at(1).content_type().subtype == "plain" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::BIT_8 &&
        msg.parts().at(1).content_type().charset == "us-ascii" && msg.parts().at(1).content() == "Zdravo, Svete!");
}


/**
Parsing related multipart with the first part HTML default charset Base64 encoded, the second part text UTF-8 charset Quoted Printable encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_multipart_html_default_base64_text_utf8_qp)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);

    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: parse multipart html default base64 text utf8 qp\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "\r\n"
        "PGh0bWw+PGhlYWQ+PC9oZWFkPjxib2R5PjxoMT5IZWxsbywgV29ybGQhPC9oMT48L2JvZHk+PC9o\r\n"
        "dG1sPg==\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "=D0=97=D0=B4=D1=80=D0=B0=D0=B2=D0=BE, =D0=A1=D0=B2=D0=B5=D1=82=D0=B5!\r\n"
        "\r\n"
        "--my_bound--\r\n";
    msg.parse(msg_str);

    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    BOOST_CHECK(msg.subject() == "parse multipart html default base64 text utf8 qp" &&  msg.boundary() == "my_bound" && msg.date_time() == ldt &&
        msg.from_to_string() == "mailio <adresa@mailio.dev>" && msg.recipients().addresses.size() == 1 &&
        msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "related" && msg.parts().size() == 2);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::TEXT && msg.parts().at(0).content_type().subtype == "html" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 &&
        msg.parts().at(0).content_type().charset.empty() && msg.parts().at(0).content() == "<html><head></head><body><h1>Hello, World!</h1></body></html>");
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::TEXT && msg.parts().at(1).content_type().subtype == "plain" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(1).content_type().charset == "utf-8" && msg.parts().at(1).content() == "Здраво, Свете!");
}


/**
Parsing alternative multipart with the first part HTML with ASCII charset Base64 encoded, the second part plain text with ASCII charset Bit7 encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_multipart_html_ascii_base64_plain_ascii_bit7)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);

    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>, <qwerty@gmail.com>\r\n"
        "Date: Fri, 12 Feb 2016 13:22:22 +0100\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: parse multipart html ascii base64 plain ascii bit7\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "PGh0bWw+PGhlYWQ+PC9oZWFkPjxib2R5PkhlbGxvLCBXb3JsZCE8L2JvZHk+PC9odG1sPg==\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n"
        "\r\n"
        "--my_bound--\r\n";
    msg.parse(msg_str);

    ptime t = time_from_string("2016-02-12 12:22:22");
    time_zone_ptr tz(new posix_time_zone("+01:00"));
    local_date_time ldt(t, tz);
    BOOST_CHECK(msg.subject() == "parse multipart html ascii base64 plain ascii bit7" &&  msg.boundary() == "my_bound" && msg.date_time() == ldt &&
        msg.from_to_string() == "mailio <adresa@mailio.dev>" && msg.recipients().addresses.size() == 2 &&
        msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "related" && msg.parts().size() == 2);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::TEXT && msg.parts().at(0).content_type().subtype == "html" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 &&
        msg.parts().at(0).content_type().charset == "us-ascii" && msg.parts().at(0).content() == "<html><head></head><body>Hello, World!</body></html>");
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::TEXT && msg.parts().at(1).content_type().subtype == "plain" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::BIT_7 &&
        msg.parts().at(1).content_type().charset == "us-ascii" && msg.parts().at(1).content() == "Zdravo, Svete!");
}


/**
Parsing multipart with leading dots and escaping flag turned off.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_dotted_multipart_no_esc)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    string msg_str =
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>, Tomislav Karastojkovic <qwerty@gmail.com>, Tomislav Karastojkovic <asdfgh@zoho.com>, Tomislav Karastojkovic <zxcvbn@hotmail.com>\r\n"
        "Date: Tue, 15 Mar 2016 13:13:32 +0000\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: parse dotted multipart no esc\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "<html>\r\n"
        "\t<head>\r\n"
        "\t\t<title>.naslov</title>\r\n"
        "\t</head>\r\n"
        "..\r\n"
        "\t<body>\r\n"
        "\t\t<h1>\r\n"
        "\t\t\t..Zdravo, Sveteeeee!\r\n"
        "\t\t</h1>\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n\r\n"
        "\t.<p>Ima li koga?</p>\r\n"
        "\t</body>\r\n"
        "</html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        ".Zdravo svete!\r\n"
        "..\r\n"
        "Ima li koga?\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "..yabadabadoo...\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        ".=D0=97=D0=B4=D1=80=D0=B0=D0=B2=D0=BE, =D0=A1=D0=B2=D0=B5=D1=82=D0=B5!\r\n"
        "..\r\n"
        "=D0=98=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=BA=D0=BE=D0=B3=D0=B0?\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "..=D1=98=D0=B0=D0=B1=D0=B0=D0=B4=D0=B0=D0=B1=D0=B0=D0=B4=D1=83=D1=83...\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "\r\n"
        "PGh0bWw+DQoJPGhlYWQ+DQoJCTx0aXRsZT4ubmFzbG92PC90aXRsZT4NCgk8L2hlYWQ+DQouLg0K\r\n"
        "CTxib2R5Pg0KCQk8aDE+DQoJCQkuLlpkcmF2bywgU3ZldGVlZWVlIQ0KCQk8L2gxPg0KDQoNCi4N\r\n"
        "Cg0KDQoJLjxwPkltYSBsaSBrb2dhPzwvcD4NCgk8L2JvZHk+DQo8L2h0bWw+\r\n"
        "\r\n"
        "--my_bound--\r\n";
    msg.parse(msg_str, false);

    ptime t = time_from_string("2016-03-15 13:13:32");
    time_zone_ptr tz(new posix_time_zone("-00:00"));
    local_date_time ldt(t, tz);
    BOOST_CHECK(msg.subject() == "parse dotted multipart no esc" && msg.boundary() == "my_bound" &&
        msg.date_time() == ldt && msg.from_to_string() == "mailio <adresa@mailio.dev>" && msg.recipients().addresses.size() == 4 &&
        msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "related" && msg.parts().size() == 4);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::TEXT && msg.parts().at(0).content_type().subtype == "html" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::BIT_7 &&
        msg.parts().at(0).content_type().charset == "us-ascii" && msg.parts().at(0).content() == "<html>\r\n"
        "\t<head>\r\n"
        "\t\t<title>.naslov</title>\r\n"
        "\t</head>\r\n"
        "..\r\n"
        "\t<body>\r\n"
        "\t\t<h1>\r\n"
        "\t\t\t..Zdravo, Sveteeeee!\r\n"
        "\t\t</h1>\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n\r\n"
        "\t.<p>Ima li koga?</p>\r\n"
        "\t</body>\r\n"
        "</html>");
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::TEXT && msg.parts().at(1).content_type().subtype == "plain" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(1).content_type().charset == "utf-8" && msg.parts().at(1).content() == ".Zdravo svete!\r\n"
        "..\r\n"
        "Ima li koga?\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "..yabadabadoo...");
    BOOST_CHECK(msg.parts().at(2).content_type().type == mime::media_type_t::TEXT && msg.parts().at(2).content_type().subtype == "plain" &&
        msg.parts().at(2).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(2).content_type().charset == "utf-8" && msg.parts().at(2).content() == ".Здраво, Свете!\r\n"
        "..\r\n"
        "Има ли кога?\r\n"
        "\r\n\r\n"
        ".\r\n"
        "\r\n\r\n"
        "..јабадабадуу...");
    BOOST_CHECK(msg.parts().at(3).content_type().type == mime::media_type_t::TEXT && msg.parts().at(3).content_type().subtype == "html" &&
        msg.parts().at(3).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 &&
        msg.parts().at(3).content_type().charset == "us-ascii" && msg.parts().at(3).content() == "<html>\r\n"
        "\t<head>\r\n"
        "\t\t<title>.naslov</title>\r\n"
        "\t</head>\r\n"
        "..\r\n"
        "\t<body>\r\n"
        "\t\t<h1>\r\n"
        "\t\t\t..Zdravo, Sveteeeee!\r\n"
        "\t\t</h1>\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "\t.<p>Ima li koga?</p>\r\n"
        "\t</body>\r\n"
        "</html>");
}


/**
Parsing multipart with leading dots and escaping flag turned on.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_dotted_multipart_esc)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    string msg_str =
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>, Tomislav Karastojkovic <qwerty@gmail.com>, Tomislav Karastojkovic <asdfgh@zoho.com>, Tomislav Karastojkovic <zxcvbn@hotmail.com>\r\n"
        "Date: Tue, 15 Mar 2016 13:13:32 +0000\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: parse dotted multipart esc\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "<html>\r\n"
        "\t<head>\r\n"
        "\t\t<title>.naslov</title>\r\n"
        "\t</head>\r\n"
        "...\r\n"
        "\t<body>\r\n"
        "\t\t<h1>\r\n"
        "\t\t\t..Zdravo, Sveteeeee!\r\n"
        "\t\t</h1>\r\n"
        "\r\n"
        "\r\n"
        "..\r\n"
        "\r\n"
        "\r\n"
        "\t.<p>Ima li koga?</p>\r\n"
        "\t</body>\r\n"
        "</html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "..Zdravo svete!\r\n"
        "...\r\n"
        "Ima li koga?\r\n"
        "\r\n"
        "\r\n"
        "..\r\n"
        "\r\n"
        "\r\n"
        "...yabadabadoo...\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "..=D0=97=D0=B4=D1=80=D0=B0=D0=B2=D0=BE, =D0=A1=D0=B2=D0=B5=D1=82=D0=B5!\r\n"
        "...\r\n"
        "=D0=98=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=BA=D0=BE=D0=B3=D0=B0?\r\n"
        "\r\n"
        "\r\n"
        "..\r\n"
        "\r\n"
        "\r\n"
        "...=D1=98=D0=B0=D0=B1=D0=B0=D0=B4=D0=B0=D0=B1=D0=B0=D0=B4=D1=83=D1=83...\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "\r\n"
        "PGh0bWw+DQoJPGhlYWQ+DQoJCTx0aXRsZT4ubmFzbG92PC90aXRsZT4NCgk8L2hlYWQ+DQouLg0K\r\n"
        "CTxib2R5Pg0KCQk8aDE+DQoJCQkuLlpkcmF2bywgU3ZldGVlZWVlIQ0KCQk8L2gxPg0KDQoNCi4N\r\n"
        "Cg0KDQoJLjxwPkltYSBsaSBrb2dhPzwvcD4NCgk8L2JvZHk+DQo8L2h0bWw+\r\n"
        "\r\n"
        "--my_bound--\r\n";
    msg.parse(msg_str, true);

    ptime t = time_from_string("2016-03-15 13:13:32");
    time_zone_ptr tz(new posix_time_zone("-00:00"));
    local_date_time ldt(t, tz);
    BOOST_CHECK(msg.subject() == "parse dotted multipart esc" && msg.boundary() == "my_bound" &&
        msg.date_time() == ldt && msg.from_to_string() == "mailio <adresa@mailio.dev>" && msg.recipients().addresses.size() == 4 &&
        msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "related" && msg.parts().size() == 4);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::TEXT && msg.parts().at(0).content_type().subtype == "html" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::BIT_7 &&
        msg.parts().at(0).content_type().charset == "us-ascii" && msg.parts().at(0).content() == "<html>\r\n"
        "\t<head>\r\n"
        "\t\t<title>.naslov</title>\r\n"
        "\t</head>\r\n"
        "..\r\n"
        "\t<body>\r\n"
        "\t\t<h1>\r\n"
        "\t\t\t..Zdravo, Sveteeeee!\r\n"
        "\t\t</h1>\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n\r\n"
        "\t.<p>Ima li koga?</p>\r\n"
        "\t</body>\r\n"
        "</html>");
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::TEXT && msg.parts().at(1).content_type().subtype == "plain" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(1).content_type().charset == "utf-8" && msg.parts().at(1).content() == ".Zdravo svete!\r\n"
        "..\r\n"
        "Ima li koga?\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "..yabadabadoo...");
    BOOST_CHECK(msg.parts().at(2).content_type().type == mime::media_type_t::TEXT && msg.parts().at(2).content_type().subtype == "plain" &&
        msg.parts().at(2).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(2).content_type().charset == "utf-8" && msg.parts().at(2).content() == ".Здраво, Свете!\r\n"
        "..\r\n"
        "Има ли кога?\r\n"
        "\r\n\r\n"
        ".\r\n"
        "\r\n\r\n"
        "..јабадабадуу...");
    BOOST_CHECK(msg.parts().at(3).content_type().type == mime::media_type_t::TEXT && msg.parts().at(3).content_type().subtype == "html" &&
        msg.parts().at(3).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 &&
        msg.parts().at(3).content_type().charset == "us-ascii" && msg.parts().at(3).content() == "<html>\r\n"
        "\t<head>\r\n"
        "\t\t<title>.naslov</title>\r\n"
        "\t</head>\r\n"
        "..\r\n"
        "\t<body>\r\n"
        "\t\t<h1>\r\n"
        "\t\t\t..Zdravo, Sveteeeee!\r\n"
        "\t\t</h1>\r\n"
        "\r\n"
        "\r\n"
        ".\r\n"
        "\r\n"
        "\r\n"
        "\t.<p>Ima li koga?</p>\r\n"
        "\t</body>\r\n"
        "</html>");
}


/**
Parsing multipart with long content in various combinations.

The message has four parts: the first is long HTML ASCII charset Seven Bit encoded, the second is long text ASCII charset Base64 encoded, the third is
long text ASCII charset Quoted Printable encoded, the fourth is long text UTF-8 charset Quoted printable encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_long_multipart)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    string msg_str =
        "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/related; boundary=\"my_bound\"\r\n"
        "Subject: parse long multipart\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: 7bit\r\n"
        "\r\n"
        "<html><head></head><body><h1>Hello, World!</h1><p>Zdravo Svete!</p><p>Opa Bato\r\n"
         "!</p><p>Shta ima?</p><p>Yaba Daba Doo!</p></body></html>\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
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
        "aWggbGluaWphLg0KDQoNCg==\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=us-ascii\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija=\r\n"
        ". Nije jasno kako ce se tekst prelomiti\r\n"
        "pa se nadam da cce to ovaj test pokazati.\r\n\r\n"
        "Treba videti kako poznati mejl klijenti lome tekst, pa na\r\n"
        "osnovu toga doraditi formatiranje sadrzzaja mejla. A mozzda i nema potrebe,=\r\n"
        " jer libmailio nije zamishljen da se\r\n"
        "bavi formatiranjem teksta.\r\n\r\n\r\n"
        "U svakom sluchaju, posle provere latinice treba uraditi i proveru utf8 kara=\r\n"
        "ktera odn. ccirilice\r\n"
        "i videti kako se prelama tekst kada su karakteri vishebajtni. Trebalo bi da=\r\n"
        " je nebitno da li je enkoding\r\n"
        "base64 ili quoted printable, jer se ascii karakteri prelamaju u nove linije=\r\n"
        ". Ovaj test bi trebalo da\r\n"
        "pokazze ima li bagova u logici formatiranja,\r\n"
        " a isto to treba proveriti sa parsiranjem.\r\n\r\n\r\n\r\n\r\n"
        "Ovde je i provera za niz praznih linija.\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=BE =D1=98=D0=B5 =D1=98=D0=B0=D0=BA=D0=BE =D0=B4=D1=83=D0=B3=\r\n"
        "=D0=B0=D1=87=D0=BA=D0=B0 =D0=BF=D0=BE=D1=80=D1=83=D0=BA=D0=B0 =D0=BA=D0=BE=\r\n"
        "=D1=98=D0=B0 =D0=B8=D0=BC=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=D0=B8=\r\n"
        "=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0 =D0=B8 =D0=BF=D1=80=D0=B5=D0=B4=\r\n"
        "=D1=83=D0=B3=D0=B0=D1=87=D0=BA=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=\r\n"
        "=D0=B0. =D0=9D=D0=B8=D1=98=D0=B5 =D1=98=D0=B0=D1=81=D0=BD=D0=BE =D0=BA=D0=\r\n"
        "=B0=D0=BA=D0=BE =D1=9B=D0=B5 =D1=81=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82 =\r\n"
        "=D0=BF=D1=80=D0=B5=D0=BB=D0=BE=D0=BC=D0=B8=D1=82=D0=B8\r\n"
        "=D0=BF=D0=B0 =D1=81=D0=B5 =D0=BD=D0=B0=D0=B4=D0=B0=D0=BC =D0=B4=D0=B0 =D1=\r\n"
        "=9B=D0=B5 =D1=82=D0=BE =D0=BE=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=D0=BA=D1=81=D1=\r\n"
        "=82 =D0=BF=D0=BE=D0=BA=D0=B0=D0=B7=D0=B0=D1=82=D0=B8.\r\n"
        "\r\n"
        "=D0=A2=D1=80=D0=B5=D0=B1=D0=B0 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=\r\n"
        "=D0=B0=D0=BA=D0=BE =D0=BF=D0=BE=D0=B7=D0=BD=D0=B0=D1=82=D0=B8 =D0=BC=D0=B5=\r\n"
        "=D1=98=D0=BB =D0=BA=D0=BB=D0=B8=D1=98=D0=B5=D0=BD=D1=82=D0=B8 =D0=BB=D0=BE=\r\n"
        "=D0=BC=D0=B5 =D1=82=D0=B5=D0=BA=D1=81=D1=82, =D0=BF=D0=B0 =D0=BD=D0=B0\r\n"
        "=D0=BE=D1=81=D0=BD=D0=BE=D0=B2=D1=83 =D1=82=D0=BE=D0=B3=D0=B0 =D0=B4=D0=BE=\r\n"
        "=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=\r\n"
        "=D0=B8=D1=80=D0=B0=D1=9A=D0=B5 =D0=BC=D0=B5=D1=98=D0=BB=D0=B0. =D0=90 =D0=\r\n"
        "=BC=D0=BE=D0=B6=D0=B4=D0=B0 =D0=B8 =D0=BD=D0=B5=D0=BC=D0=B0 =D0=BF=D0=BE=D1=\r\n"
        "=82=D1=80=D0=B5=D0=B1=D0=B5, =D1=98=D0=B5=D1=80 libmailio =D0=BD=D0=B8=D1=\r\n"
        "=98=D0=B5 =D0=B7=D0=B0=D0=BC=D0=B8=D1=88=D1=99=D0=B5=D0=BD =D0=B4=D0=B0 =D1=\r\n"
        "=81=D0=B5\r\n"
        "=D0=B1=D0=B0=D0=B2=D0=B8 =D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=\r\n"
        "=D0=B0=D1=9A=D0=B5=D0=BC =D1=82=D0=B5=D0=BA=D1=81=D1=82=D0=B0.\r\n"
        "\r\n"
        "\r\n"
        "=D0=A3 =D1=81=D0=B2=D0=B0=D0=BA=D0=BE=D0=BC =D1=81=D0=BB=D1=83=D1=87=D0=B0=\r\n"
        "=D1=98=D1=83, =D0=BF=D0=BE=D1=81=D0=BB=D0=B5 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B5 =D0=BB=D0=B0=D1=82=D0=B8=D0=BD=D0=B8=D1=86=D0=B5 =D1=82=D1=80=\r\n"
        "=D0=B5=D0=B1=D0=B0 =D1=83=D1=80=D0=B0=D0=B4=D0=B8=D1=82=D0=B8 =D0=B8 =D0=BF=\r\n"
        "=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D1=83 utf8 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=\r\n"
        "=82=D0=B5=D1=80=D0=B0 =D0=BE=D0=B4=D0=BD. =D1=9B=D0=B8=D1=80=D0=B8=D0=BB=D0=\r\n"
        "=B8=D1=86=D0=B5\r\n"
        "=D0=B8 =D0=B2=D0=B8=D0=B4=D0=B5=D1=82=D0=B8 =D0=BA=D0=B0=D0=BA=D0=BE =D1=81=\r\n"
        "=D0=B5 =D0=BF=D1=80=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0 =D1=82=D0=B5=D0=BA=D1=81=\r\n"
        "=D1=82 =D0=BA=D0=B0=D0=B4=D0=B0 =D1=81=D1=83 =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=\r\n"
        "=D1=82=D0=B5=D1=80=D0=B8 =D0=B2=D0=B8=D1=88=D0=B5=D0=B1=D0=B0=D1=98=D1=82=\r\n"
        "=D0=BD=D0=B8. =D0=A2=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B1=D0=B8 =D0=\r\n"
        "=B4=D0=B0 =D1=98=D0=B5 =D0=BD=D0=B5=D0=B1=D0=B8=D1=82=D0=BD=D0=BE =D0=B4=D0=\r\n"
        "=B0 =D0=BB=D0=B8 =D1=98=D0=B5 =D0=B5=D0=BD=D0=BA=D0=BE=D0=B4=D0=B8=D0=BD=D0=\r\n"
        "=B3\r\n"
        "base64 =D0=B8=D0=BB=D0=B8 quoted printable, =D1=98=D0=B5=D1=80 =D1=81=D0=B5=\r\n"
        " ascii =D0=BA=D0=B0=D1=80=D0=B0=D0=BA=D1=82=D0=B5=D1=80=D0=B8 =D0=BF=D1=80=\r\n"
        "=D0=B5=D0=BB=D0=B0=D0=BC=D0=B0=D1=98=D1=83 =D1=83 =D0=BD=D0=BE=D0=B2=D0=B5 =\r\n"
        "=D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B5. =D0=9E=D0=B2=D0=B0=D1=98 =D1=82=D0=B5=\r\n"
        "=D1=81=D1=82 =D0=B1=D0=B8 =D1=82=D1=80=D0=B5=D0=B1=D0=B0=D0=BB=D0=BE =D0=B4=\r\n"
        "=D0=B0\r\n"
        "=D0=BF=D0=BE=D0=BA=D0=B0=D0=B6=D0=B5 =D0=B8=D0=BC=D0=B0 =D0=BB=D0=B8 =D0=B1=\r\n"
        "=D0=B0=D0=B3=D0=BE=D0=B2=D0=B0 =D1=83 =D0=BB=D0=BE=D0=B3=D0=B8=D1=86=D0=B8 =\r\n"
        "=D1=84=D0=BE=D1=80=D0=BC=D0=B0=D1=82=D0=B8=D1=80=D0=B0=D1=9A=D0=B0,\r\n"
        "=D0=B0 =D0=B8=D1=81=D1=82=D0=BE =D1=82=D0=BE =D1=82=D1=80=D0=B5=D0=B1=D0=B0=\r\n"
        " =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=D1=80=D0=B8=D1=82=D0=B8 =D1=81=D0=B0 =D0=BF=\r\n"
        "=D0=B0=D1=80=D1=81=D0=B8=D1=80=D0=B0=D1=9A=D0=B5=D0=BC.\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "\r\n"
        "=D0=9E=D0=B2=D0=B4=D0=B5 =D1=98=D0=B5 =D0=B8 =D0=BF=D1=80=D0=BE=D0=B2=D0=B5=\r\n"
        "=D1=80=D0=B0 =D0=B7=D0=B0 =D0=BD=D0=B8=D0=B7 =D0=BF=D1=80=D0=B0=D0=B7=D0=BD=\r\n"
        "=D0=B8=D1=85 =D0=BB=D0=B8=D0=BD=D0=B8=D1=98=D0=B0.\r\n"
        "\r\n"
        "--my_bound--\r\n";
    msg.parse(msg_str);

    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    BOOST_CHECK(msg.subject() == "parse long multipart" &&  msg.boundary() == "my_bound" && msg.date_time() == ldt &&
        msg.from_to_string() == "mailio <adresa@mailio.dev>" && msg.recipients().addresses.size() == 1 &&
        msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "related" && msg.parts().size() == 4);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::TEXT && msg.parts().at(0).content_type().subtype == "html" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::BIT_7 && msg.parts().at(0).content_type().charset == "us-ascii" &&
        msg.parts().at(0).content() == "<html><head></head><body><h1>Hello, World!</h1><p>Zdravo Svete!</p><p>Opa Bato\r\n"
        "!</p><p>Shta ima?</p><p>Yaba Daba Doo!</p></body></html>");
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::TEXT && msg.parts().at(1).content_type().subtype == "plain" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 && msg.parts().at(1).content_type().charset == "us-ascii"
        && msg.parts().at(1).content() ==
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. Nije jasno kako ce se tekst prelomiti\r\n"
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
    BOOST_CHECK(msg.parts().at(2).content_type().type == mime::media_type_t::TEXT && msg.parts().at(2).content_type().subtype == "plain" &&
        msg.parts().at(2).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(2).content_type().charset == "us-ascii" && msg.parts().at(2).content() ==
        "Ovo je jako dugachka poruka koja ima i praznih linija i predugachkih linija. Nije jasno kako ce se tekst prelomiti\r\n"
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
        "Ovde je i provera za niz praznih linija.");
    BOOST_CHECK(msg.parts().at(3).content_type().type == mime::media_type_t::TEXT && msg.parts().at(3).content_type().subtype == "plain" &&
        msg.parts().at(3).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(3).content_type().charset == "utf-8" && msg.parts().at(3).content() ==
        "Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
        "па се надам да ће то овај текст показати.\r\n"
        "\r\n"
        "Треба видети како познати мејл клијенти ломе текст, па на\r\n"
        "основу тога дорадити форматирање мејла. А можда и нема потребе, јер libmailio није замишљен да се\r\n"
        "бави форматирањем текста.\r\n"
        "\r\n\r\n"
        "У сваком случају, после провере латинице треба урадити и проверу utf8 карактера одн. ћирилице\r\n"
        "и видети како се прелама текст када су карактери вишебајтни. Требало би да је небитно да ли је енкодинг\r\n"
        "base64 или quoted printable, јер се ascii карактери преламају у нове линије. Овај тест би требало да\r\n"
        "покаже има ли багова у логици форматирања,\r\n"
        "а исто то треба проверити са парсирањем.\r\n"
        "\r\n\r\n\r\n\r\n"
        "Овде је и провера за низ празних линија.");
}


/**
Parsing multipart message with a content.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_multipart_content)
{
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>, <qwerty@gmail.com>, Tomislav Karastojkovic <asdfgh@outlook.com>\r\n"
        "Date: Fri, 17 Jan 2014 05:39:22 -0730\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/alternative; boundary=\"my_bound\"\r\n"
        "Subject: parse multipart content\r\n"
        "\r\n"
        "This is a multipart message.\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Base64\r\n"
        "\r\n"
        "PGh0bWw+PGhlYWQ+PC9oZWFkPjxib2R5PjxoMT7EhmFvLCBTdmV0ZSE8L2gxPjwvYm9keT48L2h0bWw+\r\n"
        "\r\n"
        "--my_bound\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Transfer-Encoding: Quoted-Printable\r\n"
        "\r\n"
        "=D0=97=D0=B4=D1=80=D0=B0=D0=B2=D0=BE, =D0=A1=D0=B2=D0=B5=D1=82=D0=B5!\r\n"
        "--my_bound--\r\n";
    msg.parse(msg_str);

    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    BOOST_CHECK(msg.subject() == "parse multipart content" && msg.content() == "This is a multipart message." && msg.boundary() == "my_bound" &&
        msg.date_time() == ldt && msg.from_to_string() == "mailio <adresa@mailio.dev>" && msg.recipients().addresses.size() == 3 &&
        msg.content_type().type == mime::media_type_t::MULTIPART && msg.content_type().subtype == "alternative" && msg.parts().size() == 2);
    BOOST_CHECK(msg.parts().at(0).content_type().type == mime::media_type_t::TEXT && msg.parts().at(0).content_type().subtype == "html" &&
        msg.parts().at(0).content_transfer_encoding() == mime::content_transfer_encoding_t::BASE_64 &&
        msg.parts().at(0).content_type().charset == "utf-8" &&
        msg.parts().at(0).content() == "<html><head></head><body><h1>Ćao, Svete!</h1></body></html>");
    BOOST_CHECK(msg.parts().at(1).content_type().type == mime::media_type_t::TEXT && msg.parts().at(1).content_type().subtype == "plain" &&
        msg.parts().at(1).content_transfer_encoding() == mime::content_transfer_encoding_t::QUOTED_PRINTABLE &&
        msg.parts().at(1).content_type().charset == "utf-8" && msg.parts().at(1).content() == "Здраво, Свете!");
}


/**
Parsing attachments of a message.

The message is formatted by the library itself.

@pre  Files `cv.txt` and `aleph0.png` used for attaching files.
@post Created files `tkcv.txt` and `a0.png` as copies of `cv.txt` and `aleph0.png`.
**/
BOOST_AUTO_TEST_CASE(parse_attachment)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("parse attachment");
    ifstream ifs1("cv.txt");
    msg.attach(ifs1, "tkcv.txt", message::media_type_t::APPLICATION, "txt");
    ifstream ifs2("aleph0.png");
    msg.attach(ifs2, "a0.png", message::media_type_t::IMAGE, "png");

    string msg_str;
    msg.format(msg_str);
    message msg_msg;
    msg_msg.parse(msg_str);
    BOOST_CHECK(msg_msg.content_type().type == mime::media_type_t::MULTIPART && msg_msg.content_type().subtype == "mixed" &&
        msg_msg.attachments_size() == 2);
    BOOST_CHECK(msg_msg.parts().at(0).name() == "tkcv.txt" && msg_msg.parts().at(0).content_type().type ==
        message::media_type_t::APPLICATION && msg_msg.parts().at(0).content_type().subtype == "txt");
    BOOST_CHECK(msg_msg.parts().at(1).name() == "a0.png" && msg_msg.parts().at(1).content_type().type == message::media_type_t::IMAGE &&
        msg_msg.parts().at(1).content_type().subtype == "png");

    ofstream ofs1("tkcv.txt");
    string ofs1_name;
    msg_msg.attachment(1, ofs1, ofs1_name);
    BOOST_CHECK(ofs1_name == "tkcv.txt");
    ofstream ofs2("a0.png");
    string ofs2_name;
    msg_msg.attachment(2, ofs2, ofs2_name);
    BOOST_CHECK(ofs2_name == "a0.png");
}


/**
Parsing attachments and an HTML content of a message.

@pre  Files `cv.txt` and `aleph0.png` used for attaching files.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_html_attachment)
{
    message msg;
    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg.subject("parse html attachment");
    msg.boundary("mybnd");
    msg.content_type(message::media_type_t::TEXT, "html", "utf-8");
    msg.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
    msg.content("<h1>Naslov</h1><p>Ovo je poruka.</p>");

    ifstream ifs1("cv.txt");
    message::content_type_t ct1(message::media_type_t::APPLICATION, "txt");
    auto tp1 = std::tie(ifs1, "tkcv.txt", ct1);
    ifstream ifs2("aleph0.png");
    message::content_type_t ct2(message::media_type_t::IMAGE, "png");
    auto tp2 = std::tie(ifs2, "a0.png", ct2);
    list<tuple<std::istream&, string, message::content_type_t>> atts;
    atts.push_back(tp1);
    atts.push_back(tp2);

    msg.attach(atts);
    string msg_str;
    msg.format(msg_str);

    message msg_msg;
    msg_msg.parse(msg_str);
    BOOST_CHECK(msg_msg.content_type().type == mime::media_type_t::MULTIPART && msg_msg.content_type().subtype == "mixed" && msg_msg.attachments_size() == 2);
    BOOST_CHECK(msg_msg.parts().at(0).content() == "<h1>Naslov</h1><p>Ovo je poruka.</p>" && msg_msg.parts().at(0).content_type().type ==
        mime::media_type_t::TEXT && msg_msg.parts().at(0).content_type().subtype == "html");
    BOOST_CHECK(msg_msg.parts().at(1).name() == "tkcv.txt" && msg_msg.parts().at(1).content_type().type ==
        message::media_type_t::APPLICATION && msg_msg.parts().at(1).content_type().subtype == "txt");
    BOOST_CHECK(msg_msg.parts().at(2).name() == "a0.png" && msg_msg.parts().at(2).content_type().type ==
        message::media_type_t::IMAGE && msg_msg.parts().at(2).content_type().subtype == "png");
}


/**
Parsing a message with the recipents and CC recipients in several lines.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_multilined_addresses)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <kontakt@mailio.dev>\r\n"
        "To: contact <kontakt@mailio.dev>,\r\n"
        "  Tomislav Karastojkovic <adresa@mailio.dev>,\r\n"
        "  Tomislav Karastojkovic <qwerty@gmail.com>,\r\n"
        "  Tomislav Karastojkovic <asdfg@zoho.com>,\r\n"
        "Cc: mail.io <adresa@mailio.dev>,\r\n"
        "  Tomislav Karastojkovic <zxcvb@yahoo.com>\r\n"
        "Date: Wed, 23 Aug 2017 22:16:45 +0000\r\n"
        "Subject: Hello, World!\r\n"
        "\r\n"
        "Hello, World!\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);

    BOOST_CHECK(msg.from().addresses.at(0).name == "mailio" && msg.from().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.recipients().addresses.at(0).name == "contact" && msg.recipients().addresses.at(0).address == "kontakt@mailio.dev" &&
        msg.recipients().addresses.at(1).name == "Tomislav Karastojkovic" && msg.recipients().addresses.at(1).address == "adresa@mailio.dev" &&
        msg.recipients().addresses.at(2).name == "Tomislav Karastojkovic" && msg.recipients().addresses.at(2).address == "qwerty@gmail.com" &&
        msg.recipients().addresses.at(3).name == "Tomislav Karastojkovic" && msg.recipients().addresses.at(3).address == "asdfg@zoho.com" &&
        msg.cc_recipients().addresses.at(0).name == "mail.io" && msg.cc_recipients().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.cc_recipients().addresses.at(1).name == "Tomislav Karastojkovic" && msg.cc_recipients().addresses.at(1).address == "zxcvb@yahoo.com");
}


/**
Parsing a message with the recipients in a long line.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_long_addresses)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Reply-To: Tomislav Karastojkovic <kontakt@mailio.dev>\r\n"
        "To: contact <kontakt@mailio.dev>, Tomislav Karastojkovic <adresa@mailio.dev>, Tomislav Karastojkovic <qwerty@gmail.com>, "
        "Tomislav Karastojkovic <asdfg@zoho.com>\r\n"
        "Cc: mail.io <adresa@mailio.dev>, Tomislav Karastojkovic <zxcvb@yahoo.com>\r\n"
        "Date: Wed, 23 Aug 2017 22:16:45 +0000\r\n"
        "Subject: Hello, World!\r\n"
        "\r\n"
        "Hello, World!\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mailio" && msg.from().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.recipients().addresses.at(0).name == "contact" && msg.recipients().addresses.at(0).address == "kontakt@mailio.dev" &&
        msg.recipients().addresses.at(1).name == "Tomislav Karastojkovic" && msg.recipients().addresses.at(1).address == "adresa@mailio.dev" &&
        msg.recipients().addresses.at(2).name == "Tomislav Karastojkovic" && msg.recipients().addresses.at(2).address == "qwerty@gmail.com" &&
        msg.recipients().addresses.at(3).name == "Tomislav Karastojkovic" && msg.recipients().addresses.at(3).address == "asdfg@zoho.com" &&
        msg.cc_recipients().addresses.at(0).name == "mail.io" && msg.cc_recipients().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.cc_recipients().addresses.at(1).name == "Tomislav Karastojkovic" && msg.cc_recipients().addresses.at(1).address == "zxcvb@yahoo.com");
}


/**
Parsing a message with the disposition notification.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_notification)
{
    string msg_str = "From: karastojko <qwerty@gmail.com>\r\n"
        "To: karastojko <asdfg@hotmail.com>\r\n"
        "Disposition-Notification-To: karastojko <zxcvb@zoho.com>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: parse notification\r\n"
        "\r\n"
        "Hello, World!\r\n";
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);

    BOOST_CHECK(msg.disposition_notification_to_string() == "karastojko <zxcvb@zoho.com>" && msg.subject() == "parse notification");
}


/**
Parsing a message with Q/Quoted Printable encoded sender.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qq_sender)
{
    string msg_str = "From: =?UTF-8?Q?=D0=BC=D0=B0=D0=B8=D0=BB=D0=B8=D0=BE?= <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>, \r\n"
        "    =?UTF-8?Q?Tomislav_Karastojkovi=C4=87?= <qwerty@gmail.com>, \r\n"
        "    =?UTF-8?Q?=D0=A2=D0=BE=D0=BC=D0=B8=D1=81=D0=BB=D0=B0=D0=B2_=D0=9A=D0=B0=D1=80=D0=B0=D1=81=D1=82=D0=BE=D1=98=D0=BA=D0=BE=D0=B2=D0=B8=D1=9B?= <asdfg@zoho.com>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: proba\r\n"
        "\r\n"
        "test\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "маилио" && msg.from().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.recipients().addresses.at(0).name == "mailio" && msg.recipients().addresses.at(0).address == "adresa@mailio.dev" &&
        msg.recipients().addresses.at(1).name == "Tomislav Karastojković" && msg.recipients().addresses.at(1).address == "qwerty@gmail.com" &&
        msg.recipients().addresses.at(2).name == "Томислав Карастојковић" && msg.recipients().addresses.at(2).address == "asdfg@zoho.com");
}


/**
Parsing a message with Q/Base64 encoded sender.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qb_sender)
{
    string msg_str = "From: =?UTF-8?B?0LzQsNC40LvQuNC+?= <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: proba\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "\r\n"
        "test\r\n";
    message msg;
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "маилио");
}


/**
Parsing a message with sender's name Q encoded not separated by space from the address.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qq_from_no_space)
{
    string msg_str = "From: =?windows-1252?Q?Action_fran=E7aise_?=<adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: examen\r\n"
        "\r\n"
        "test\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "Action fran" "\xE7" "aise" && msg.from().addresses.at(0).address == "adresa@mailio.dev");
}


/**
Parsing a message with Q/Base64 encoded subject with the UTF-8 charset.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qb_utf8_subject)
{
    string msg_str = "From: mail io <adre.sa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: =?UTF-8?B?UmU6IM6jz4fOtc+EOiBTdW1tZXIgMjAxNw==?=\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "\r\n"
        "hello world\r\n";
    message msg;

    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mail io" && msg.from().addresses.at(0).address == "adre.sa@mailio.dev" && msg.date_time() == ldt &&
        msg.recipients_to_string() == "mailio <adresa@mailio.dev>" && msg.subject() == "Re: Σχετ: Summer 2017" && msg.content() == "hello world");
}


/**
Parsing a message with Q/Base64 encoded subject in the ISO-8859-1 charset by using the mailio string type.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qq_latin1_subject_raw)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: =?iso-8859-1?Q?Comprobaci=F3n_CV?=\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "\r\n"
        "hello world\r\n";
    message msg;

    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.parse(msg_str);

    BOOST_CHECK(msg.from().addresses.at(0).name == "mailio" && msg.from().addresses.at(0).address == "adresa@mailio.dev" && msg.date_time() == ldt &&
        msg.recipients_to_string() == "mailio <adresa@mailio.dev>" &&
        msg.subject_raw().buffer == "Comprobaci\363n CV" && msg.subject_raw().charset == "ISO-8859-1" &&
        msg.content() == "hello world");
}


/**
Parsing a subject and checking the result against `string_t`.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qq_utf8_emoji_subject_raw)
{
    string msg_str = "From: \"Tomislav Karastojkovic\" <qwerty@gmail.com>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: =?utf-8?Q?=F0=9F=8E=81=C5=BDivi=20godinu=20dana=20na=20ra=C4=8Dun=20Super=20Kartice?=\r\n"
        "Date: Fri, 24 Dec 2021 15:15:38 +0000\r\n"
        "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "0JfQtNGA0LDQstC+LCDQodCy0LXRgtC1IQ0K";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.subject_raw() == string_t("🎁Živi godinu dana na račun Super Kartice", "utf-8"));
}


/**
Parsing a message with Q/Quoted Printable encoded long subject.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qq_long_subject)
{
    string msg_str = "From: mail io <adre.sa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: =?UTF-8?Q?TOMISLAV_KARASTOJKOVI=C4=86_PR_RA=C4=8CUNAR?=\r\n"
        "    =?UTF-8?Q?SKO_PROGRAMIRANJE_ALEPHO_BEOGRAD_?=\r\n"
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
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mail io" && msg.from().addresses.at(0).address == "adre.sa@mailio.dev" && msg.date_time() == ldt &&
        msg.recipients_to_string() == "mailio <adresa@mailio.dev>" &&
        msg.subject() == "TOMISLAV KARASTOJKOVIĆ PR RAČUNARSKO PROGRAMIRANJE ALEPHO BEOGRAD" &&
        msg.content() == "hello\r\n\r\nworld\r\n\r\n\r\nopa bato");
}


/**
Parsing a message with Q/Base64 encoded long subject.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qb_long_subject)
{
    string msg_str = "From: mail io <adre.sa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: =?UTF-8?B?UmU6IM6jz4fOtc+EOiBSZXF1ZXN0IGZyb20gR3Jja2FJbmZvIHZpc2l0b3Ig?=\r\n"
        "  =?UTF-8?B?LSBFbGVuaSBCZWFjaCBBcGFydG1lbnRz?=\r\n"
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
    msg.header_codec(message::header_codec_t::QUOTED_PRINTABLE);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mail io" && msg.from().addresses.at(0).address == "adre.sa@mailio.dev" && msg.date_time() == ldt &&
        msg.recipients_to_string() == "mailio <adresa@mailio.dev>" &&
        msg.subject() == "Re: Σχετ: Request from GrckaInfo visitor - Eleni Beach Apartments" &&
        msg.content() == "hello\r\n\r\nworld\r\n\r\n\r\nopa bato");
}


/**
Parsing a message with mixed Q/Quoted Printable and Q/Base64 long subject.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qbq_long_subject)
{
    string msg_str = "From: mail io <adre.sa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: =?UTF-8?B?UmU6IM6jz4fOtc+EOiBSZXF1ZXN0IGZyb20gR3Jja2FJbmZvIHZpc2l0?=\r\n"
        " =?UTF-8?Q?or_-_Eleni_Beach_Apartments?=\r\n"
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
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "mail io" && msg.from().addresses.at(0).address == "adre.sa@mailio.dev" && msg.date_time() == ldt &&
        msg.recipients_to_string() == "mailio <adresa@mailio.dev>" &&
        msg.subject() == "Re: Σχετ: Request from GrckaInfo visitor - Eleni Beach Apartments" &&
        msg.content() == "hello\r\n\r\nworld\r\n\r\n\r\nopa bato");
}


/**
Parsing a message with UTF-8 subject containing the long dash character.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qq_subject_dash)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: =?UTF-8?Q?C++_Annotated:_Sep_=E2=80=93_Dec_2017?=\r\n"
        "\r\n"
        "test\r\n";
    message msg;

    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);

    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() == "C++ Annotated: Sep – Dec 2017");
}


/**
Parsing a message with UTF-8 subject containing an emoji character.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qq_subject_emoji)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: =?utf-8?Q?=F0=9F=8E=81=C5=BDivi=20godinu=20dana=20na=20ra=C4=8Dun=20Super=20Kartice?=\r\n"
        "\r\n"
        "test\r\n";
    message msg;

    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);

    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() == "🎁Živi godinu dana na račun Super Kartice");
}


/**
Parsing a long subject.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_qq_subject_long)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: =?utf-8?Q?=F0=9F=8E=84=F0=9F=8E=81=F0=9F=8E=8A=C2=A0Sre=C4=87ni=20novogodi=C5=A1nji=20i=20bo=C5=BEi=C4=87ni=20praznici=C2=A0=F0=9F=8E=89=F0=9F=8E=85=F0=9F=92=9D?=\r\n"
        "\r\n"
        "test\r\n";
    message msg;

    ptime t = time_from_string("2016-02-11 22:56:22");
    time_zone_ptr tz(new posix_time_zone("+00:00"));
    local_date_time ldt(t, tz);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() ==
        u8"\U0001F384\U0001F381\U0001F38A\u00A0Sre\u0107ni novogodi\u0161nji i bo\u017Ei\u0107ni praznici\u00A0\U0001F389\U0001F385\U0001F49D");
}


/**
Parsing a UTF8 subject in the eight bit encoding.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_utf8_subject)
{
    string msg_str = "From: \"Tomislav Karastojkovic\" <qwerty@gmail.com>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: Здраво, Свете!\r\n"
        "Date: Fri, 24 Dec 2021 15:15:38 +0000\r\n"
        "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "0JfQtNGA0LDQstC+LCDQodCy0LXRgtC1IQ0K";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.subject() == "Здраво, Свете!");
}


/**
Parsing a UTF8 sender with the quoted name in the eight bit encoding.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_utf8_quoted_name)
{
    string msg_str = "From: \"Tomislav Karastojković\" <qwerty@gmail.com>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: Proba za UTF8\r\n"
        "Date: Fri, 24 Dec 2021 15:15:38 +0000\r\n"
        "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "0JfQtNGA0LDQstC+LCDQodCy0LXRgtC1IQ0K";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "Tomislav Karastojković" && msg.from().addresses.at(0).address == "qwerty@gmail.com");

    msg.header_codec(mime::header_codec_t::UTF8);
    BOOST_CHECK(msg.from_to_string() == "Tomislav Karastojković <qwerty@gmail.com>");
}


/**
Parsing a UTF8 recipient with the quoted name in the eight bit encoding.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_utf8_name)
{
    string msg_str = "From: Tomislav Karastojkovic <qwerty@gmail.com>\r\n"
        "To: \"Tomislav Karastojković\" <qwerty@gmail.com>\r\n"
        "Subject: Здраво, Свете!\r\n"
        "Date: Fri, 24 Dec 2021 15:15:38 +0000\r\n"
        "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "0JfQtNGA0LDQstC+LCDQodCy0LXRgtC1IQ0K";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.recipients().addresses.at(0).name == "Tomislav Karastojković" && msg.recipients().addresses.at(0).address == "qwerty@gmail.com");
    BOOST_CHECK(msg.subject() == "Здраво, Свете!");

    msg.header_codec(mime::header_codec_t::UTF8);
    BOOST_CHECK(msg.recipients_to_string() == "Tomislav Karastojković <qwerty@gmail.com>");
}


/**
Parsing UTF8 sender with the address in the eight bit encoding.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_utf8_address)
{
    string msg_str = "From: Tomislav Karastojkovic <karastojković@gmail.com>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Subject: Proba za UTF8\r\n"
        "Date: Fri, 24 Dec 2021 15:15:38 +0000\r\n"
        "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "0JfQtNGA0LDQstC+LCDQodCy0LXRgtC1IQ0K";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.from().addresses.at(0).name == "Tomislav Karastojkovic" && msg.from().addresses.at(0).address == "karastojković@gmail.com");

    msg.header_codec(mime::header_codec_t::UTF8);
    BOOST_CHECK(msg.from_to_string() == "Tomislav Karastojkovic <karastojković@gmail.com>" && msg.content() == "Здраво, Свете!\r\n");
}


/**
Parsing Q encoded recipient with the missing charset.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_q_subject_missing_charset)
{
    string msg_str = "From: =??Q?=D0=BC=D0=B0=D0=B8=D0=BB=D0=B8=D0=BE?= <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: proba\r\n"
        "\r\n"
        "test\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    BOOST_CHECK_THROW(msg.parse(msg_str), codec_error);
}


/**
Parsing Q encoded recipient with the missing codec type.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_q_subject_missing_codec)
{
    string msg_str = "From: =?UTF-8\?\?=D0=BC=D0=B0=D0=B8=D0=BB=D0=B8=D0=BE?= <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: proba\r\n"
        "\r\n"
        "test\r\n";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    BOOST_CHECK_THROW(msg.parse(msg_str), codec_error);
}


/**
Parsing the message ID.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_message_id)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID: <1234567890@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";

    {
        // strict mode
        message msg;
        msg.strict_mode(true);
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        msg.parse(msg_str);
        BOOST_CHECK(msg.message_id() == "1234567890@mailio.dev");
    }
    {
        // non-strict mode
        message msg;
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        msg.parse(msg_str);
        BOOST_CHECK(msg.message_id() == "<1234567890@mailio.dev>");
    }
}


/**
Parsing the message ID consisting only of spaces.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_whitespace_message_id)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID:    \r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    BOOST_CHECK(msg.message_id().empty() == true);
}


/**
Parsing the empty message ID.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_empty_message_id)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID:\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
}


/**
Parsing few message IDs.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_few_message_ids)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "Message-ID: <1@mailio.dev><2@mailio.dev>   <3@mailio.dev>    <4@mailio.dev>   \r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    {
        message msg;
        msg.strict_mode(true);
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        msg.parse(msg_str);
        BOOST_CHECK(msg.message_id() == "1@mailio.dev");
    }
    {
        message msg;
        msg.strict_mode(false);
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        msg.parse(msg_str);
        BOOST_CHECK(msg.message_id() == "<1@mailio.dev><2@mailio.dev>   <3@mailio.dev>    <4@mailio.dev>");
    }
}


/**
Parsing a message with the in-reply-to IDs.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_in_reply_to)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "In-Reply-To: <1@mailio.dev> <22@mailio.dev> <333@mailio.dev>\r\n"
        "References: <4444@mailio.dev> <55555@mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    {
        message msg;
        msg.strict_mode(true);
        msg.parse(msg_str);
        BOOST_CHECK(msg.in_reply_to().size() == 3 && msg.in_reply_to().at(0) == "1@mailio.dev" && msg.in_reply_to().at(1) == "22@mailio.dev" &&
            msg.in_reply_to().at(2) == "333@mailio.dev" && msg.references().at(0) == "4444@mailio.dev" && msg.references().at(1) == "55555@mailio.dev");
    }
    {
        message msg;
        msg.strict_mode(false);
        msg.parse(msg_str);

        BOOST_CHECK(msg.in_reply_to().size() == 1 && msg.in_reply_to().at(0) == "<1@mailio.dev> <22@mailio.dev> <333@mailio.dev>" &&
            msg.references().size() == 1 && msg.references().at(0) == "<4444@mailio.dev> <55555@mailio.dev>");
    }
}


/**
Parsing the message ID without the monkey character.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_in_reply_without_monkey)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "In-reply-To: <1@mailio.dev> <2 mailio.dev>\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    {
        message msg;
        msg.strict_mode(true);
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        BOOST_CHECK_THROW(msg.parse(msg_str), message_error);
    }
    {
        message msg;
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        msg.parse(msg_str);
        BOOST_CHECK(msg.in_reply_to().size() == 1 && msg.in_reply_to().at(0) == "<1@mailio.dev> <2 mailio.dev>");
    }
}


/**
Parsing the message ID without the angle brackets.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_references_without_brackets)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "References: <1@mailio.dev> 2@mailio.dev\r\n"
        "In-reply-To: <3@mailio.dev> <4@mailio.dev>"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    {
        message msg;
        msg.strict_mode(true);
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        BOOST_CHECK_THROW(msg.parse(msg_str), message_error);
    }
    {
        message msg;
        msg.strict_mode(false);
        msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
        msg.parse(msg_str);
        BOOST_CHECK(msg.references().size() == 1);
    }
}


/**
Parsing an empty header in the strict mode.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_empty_header_strict)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "User-Agent:\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    message msg;
    msg.strict_mode(true);
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
}


/**
Parsing the empty header in the non-strict mode.

@pre  None.
@post None.
@todo MSVC is not working well if headers are not copied, see below.
**/
BOOST_AUTO_TEST_CASE(parse_empty_header_relaxed)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "User-Agent:\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "Hello: World\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);

    // If the headers in tests are accessed without copying, then for some reason the multimap often does not read the individual headers
    // properly. Not sure what is the reason for this behavior, Gcc works fine.
    auto headers = msg.headers();
    BOOST_CHECK(headers.size() == 2);
    BOOST_CHECK(msg.subject() == "Proba");
    BOOST_CHECK(headers.count("User-Agent") == 1 && headers.count("Hello") == 1);
    auto user_agent = headers.find("User-Agent");
    BOOST_CHECK(user_agent->first == "User-Agent" && user_agent->second.empty());
    auto hello = headers.find("Hello");
    BOOST_CHECK(hello->first == "Hello" && hello->second == "World");
}


/**
Parsing an empty header with a wrong header name.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_wrong_empty_header)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "To: mailio <adresa@mailio.dev>\r\n"
        "User Agent:\r\n"
        "Date: Thu, 11 Feb 2016 22:56:22 +0000\r\n"
        "Subject: Proba\r\n"
        "\r\n"
        "Zdravo, Svete!\r\n";
    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
}


/**
Parsing a header with horizontal tabs.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_headers_htab)
{
    string msg_str = "From: mail io <test@sender.com>\r\n"
        "To: \tmail io <test@sender.com>\r\n"
        "Date: Sat, 18 Jun 2022 05:56:34 +0000\r\n"
        "Received: from SRV2 - test@sender.com\r\n"
        " (srv2 - test@sender.com[192.168.245.16])\tby\r\n"
        " smtp - 01.test@sender.com(Postfix) with ESMTP id 8D16C3CE\tfor\r\n"
        " <test@receiver.com>; Sat, 20 Aug 2022 11:01 : 35 + 0200\r\n"
        " (CEST)\r\n"
        "Subject: Hello, World!\r\n"
        "\r\n"
        "Hello, World!\r\n";
    message msg;
    msg.line_policy(mailio::codec::line_len_policy_t::MANDATORY, mailio::codec::line_len_policy_t::MANDATORY);
    msg.parse(msg_str);
    auto hdrs = msg.headers();
    auto rcv = hdrs.find("Received");
    BOOST_CHECK(rcv != hdrs.end());
}


/**
Copying the message by using the constructor and the assignment operator.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(object_copying)
{
    message msg1;
    msg1.from(mail_address("mailio", "adresa@mailio.dev"));
    msg1.reply_address(mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev"));
    msg1.add_recipient(mail_address("kontakt", "kontakt@mailio.dev"));
    msg1.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg1.add_recipient(mail_group("all", {mail_address("Tomislav", "qwerty@hotmail.com")}));
    msg1.add_cc_recipient(mail_group("mailio", {mail_address("", "karas@mailio.dev"), mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev")}));
    msg1.add_cc_recipient(mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev"));
    msg1.add_cc_recipient(mail_address("Tomislav @ Karastojkovic", "asdfg@gmail.com"));
    msg1.add_cc_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg1.add_cc_recipient(mail_group("all", {mail_address("", "qwerty@hotmail.com"), mail_address("Tomislav", "asdfg@gmail.com"),
        mail_address("Tomislav @ Karastojkovic", "zxcvb@zoho.com")}));
    msg1.add_bcc_recipient(mail_address("Tomislav Karastojkovic", "kontakt@mailio.dev"));
    msg1.add_bcc_recipient(mail_address("Tomislav @ Karastojkovic", "asdfg@gmail.com"));
    msg1.add_bcc_recipient(mail_address("mailio", "adresa@mailio.dev"));
    msg1.subject("Hello, World!");
    msg1.content("Hello, World!");
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg1.date_time(ldt);

    string msg1_str;
    msg1.format(msg1_str);

    {
        // Test the copy constructor.

        message msg2(msg1);
        string msg2_str;
        msg2.format(msg2_str);
        BOOST_CHECK(msg1_str == msg2_str);
    }

    {
        // Test for the assignment operator.

        message msg3;
        msg3 = msg1;
        string msg3_str;
        msg3.format(msg3_str);
        BOOST_CHECK(msg1_str == msg3_str);
    }
}
