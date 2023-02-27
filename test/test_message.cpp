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
using mailio::message_error;


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
Formatting an alternative multipart with the first part HTML default charset Base64 encoded, the second part text UTF-8 charset Quoted Printable encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_multipart_html_default_base64_text_utf8_qp)
{
    message msg;
    msg.from(mail_address("mailio", "adresa@mailio.dev"));
    msg.reply_address(mail_address("Tomislav Karastojkovic", "adresa@mailio.dev"));
    msg.add_recipient(mail_address("mailio", "adresa@mailio.dev"));
    ptime t = time_from_string("2014-01-17 13:09:22");
    time_zone_ptr tz(new posix_time_zone("-07:30"));
    local_date_time ldt(t, tz);
    msg.date_time(ldt);
    msg.subject("format multipart html default base64 text utf8 qp");
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
        "Subject: format multipart html default base64 text utf8 qp\r\n"
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
Formatting a multipart message with leading dots and the escaping flag turned off.

The first part is HTML ASCII charset Seven Bit encoded, the second part is text UTF-8 charset Quoted Printable encoded, the third part is text UTF-8
charset Quoted Printable encoded, the fourth part is HTML ASCII charset Base64 encoded.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(format_dotted_multipart_no_esc)
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
    msg.subject("format dotted multipart no escape");
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
        "Subject: format dotted multipart no escape\r\n"
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
Parsing the content type in the strict mode which does not follow the specification, so the exception is thrown.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_strict_content_type)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Content-Type: text/plain; charset       =   \"UTF-8\"\r\n"
        "To: adresa@mailio.dev\r\n"
        "Subject: parse strict content type\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.strict_mode(true);
    BOOST_CHECK_THROW(msg.parse(msg_str), mime_error);
}


/**
Parsing the content type in the non-strict mode which does not follow the specification, so the exception is not thrown.

@pre  None.
@post None.
**/
BOOST_AUTO_TEST_CASE(parse_non_strict_content_type)
{
    string msg_str = "From: mailio <adresa@mailio.dev>\r\n"
        "Content-Type: text/plain; charset       =   \"UTF-8\"\r\n"
        "To: adresa@mailio.dev\r\n"
        "Subject: parse strict content type\r\n"
        "\r\n"
        "Hello, World!";

    message msg;
    msg.line_policy(codec::line_len_policy_t::MANDATORY, codec::line_len_policy_t::MANDATORY);
    msg.strict_mode(false);
    msg.parse(msg_str);
    BOOST_CHECK(msg.content_type().type == mailio::mime::media_type_t::TEXT && msg.content_type().subtype == "plain" && msg.content_type().charset == "utf-8");
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
