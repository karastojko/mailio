/*

smtp_utf8_qp_msg.cpp
--------------------

Connects to SMTP server and sends a message with UTF8 content and subject.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <iostream>
#include <mailio/message.hpp>
#include <mailio/mime.hpp>
#include <mailio/smtp.hpp>


using mailio::message;
using mailio::mail_address;
using mailio::mime;
using mailio::smtp;
using mailio::smtp_error;
using mailio::dialog_error;
using std::cout;
using std::endl;


int main()
{
    try
    {
        // create mail message
        message msg;
        msg.header_codec(message::header_codec_t::BASE64);
        msg.from(mail_address("mailio library", "mailio@mailserver.com"));// set the correct sender name and address
        msg.add_recipient(mail_address("mailio library", "mailio@gmail.com"));// set the correct recipent name and address
        msg.add_recipient(mail_address("mailio library", "mailio@outlook.com"));// add more recipients
        msg.add_cc_recipient(mail_address("mailio library", "mailio@yahoo.com"));// add CC recipient
        msg.add_bcc_recipient(mail_address("mailio library", "mailio@zoho.com"));

        msg.subject("smtp utf8 quoted printable message");
        // create message in Cyrillic alphabet
        // set Transfer Encoding to Quoted Printable and set Content Type to UTF8
        msg.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
        msg.content_type(message::media_type_t::TEXT, "plain", "utf-8");

        const auto* msgu8 =
            u8"Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
            u8"па се надам да ће то овај текст показати.\r\n"
            u8"\r\n"
            u8"Треба видети како познати мејл клијенти ломе текст, па на\r\n"
            u8"основу тога дорадити форматирање мејла. А можда и нема потребе, јер libmailio није замишљен да се\r\n"
            u8"бави форматирањем текста.\r\n"
            u8"\r\n\r\n"
            u8"У сваком случају, после провере латинице треба урадити и проверу utf8 карактера одн. ћирилице\r\n"
            u8"и видети како се прелама текст када су карактери вишебајтни. Требало би да је небитно да ли је енкодинг\r\n"
            u8"base64 или quoted printable, јер се ascii карактери преламају у нове линије. Овај тест би требало да\r\n"
            u8"покаже има ли багова у логици форматирања,\r\n"
            u8"а исто то треба проверити са парсирањем.\r\n"
            u8"\r\n\r\n\r\n\r\n"
            u8"Овде је и провера за низ празних линија.";
        const char* msgs =
        #if defined(__cpp_char8_t)
            reinterpret_cast<const char *>(msgu8);
        #else
            msgu8;
        #endif
        msg.content(msgs);

        // use a server with plain (non-SSL) connectivity
        smtp conn("smtp.mailserver.com", 587);
        // modify username/password to use real credentials
        conn.authenticate("mailio@mailserver.com", "mailiopass", smtp::auth_method_t::LOGIN);
        conn.submit(msg);
    }
    catch (smtp_error& exc)
    {
        cout << exc.what() << endl;
    }
    catch (dialog_error& exc)
    {
        cout << exc.what() << endl;
    }

    return EXIT_SUCCESS;
}
