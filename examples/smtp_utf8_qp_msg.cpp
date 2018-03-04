/*

smtp_utf8_qp_msg.cpp
--------------------

Connects to SMTP server and sends a message with UTF8 content and subject.

 
Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html. 
 
*/


#include <iostream>
#include <message.hpp>
#include <mime.hpp>
#include <smtp.hpp>


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
        msg.sender(mail_address("mailio library", "mailio@mailserver.com"));// set the correct sender name and address
        msg.add_recipient(mail_address("mailio library", "mailio@gmail.com"));// set the correct recipent name and address
        msg.add_recipient(mail_address("mailio library", "mailio@outlook.com"));// add more recipients
        msg.add_cc_recipient(mail_address("mailio library", "mailio@yahoo.com"));// add CC recipient
        msg.add_bcc_recipient(mail_address("mailio library", "mailio@zoho.com"));

        msg.subject(u8"smtp utf8 quoted printable Ð¿Ð¾ÑÑÐºÐ°");
        // create message in Cyrillic alphabet
        // set Transfer Encoding to Quoted Printable and set Content Type to UTF8
        msg.content_transfer_encoding(mime::content_transfer_encoding_t::QUOTED_PRINTABLE);
        msg.content_type(message::media_type_t::TEXT, "plain", "utf-8");
        msg.content(u8"Ово је јако дугачка порука која има и празних линија и предугачких линија. Није јасно како ће се текст преломити\r\n"
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
