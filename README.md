
# mailio #

mailio is a cross platform C++ library for MIME format and SMTP, POP3 and IMAP protocols. It is based on standard C++ 11 and Boost library.

# Examples #

To send mail, one has to create `message` object and set it's attributes as sender, recipient, subject and so on. Then, an SMTP connection
is created by constructing `smtp` (or `smtps`) class. The message is sent over the connection:

```cpp
message msg;
msg.sender(mail_address("mailio library", "mailio@gmail.com"));
msg.add_recipient(mail_address("mailio library", "mailio@gmail.com"));
msg.subject("smtps simple message");
msg.content("Hello, World!");

smtps conn("smtp.gmail.com", 587);
conn.authenticate("mailio@gmail.com", "mailiopass", smtps::auth_method_t::START_TLS);
conn.submit(msg);
```
    
To receive a mail, `message` object is created to store the received message. Mail can be received over POP3 or IMAP, depending of mail server setup.
If POP3 is used, then instance of `pop3` (or `pop3s`) class is created and message is fetched:

```cpp
pop3s conn("pop.mail.yahoo.com", 995);
conn.authenticate("mailio@yahoo.com", "mailiopass", pop3s::auth_method_t::LOGIN);
message msg;
conn.fetch(1, msg);
```

Receiving a message over IMAP is analogous. Since IMAP recognizes folders, then one has to be specified, like *inbox*:

```cpp
imaps conn("imap.gmail.com", 993);
conn.authenticate("mailio@gmail.com", "mailiopass", imap::auth_method_t::LOGIN);
message msg;
conn.fetch("inbox", 1, msg);
```

More advanced features are shown in `examples` directory, see below how to compile them.

# Requirements #

mailio is currently under development on Linux, but it should work on any Unix derivative with gcc compiler. The following components are
required:

* gcc 4.8.2 or later.
* Boost 1.54 or later with Regex, Date Time, Random available.
* POSIX Threads, OpenSSL and Crypto libraries available on the system.

# Setup #

There is no (yet) auto configure tool, but `Makefile` is used. If, for instance, *gcc* compiler is not in the path, command line parameters should be
given to the Makefile:

```shell
gmake CXX=/usr/local/gcc-4.8.2/bin/g++ all
```

Variables that can be altered this way are `CXX` (compiler executable path), `BOOST_DIR` (Boost installation directory), `AR` (archiver executable path),
`DOXYGEN` (doxygen executable path). Alternative approach is to edit `Makefile`.

Run `gmake all`. The resulting static and shared library should be in newly created `lib` directory.

Example are stored in `examples` directory. Run `gmake examples` to compile them. They will be placed in `build/examples` directory.

Documentation can be generated if [Doxygen](http://www.doxygen.org) is installed. By executing `gmake doc` it will be generated in `doc`
directory.

# Features #

* Recursive formatter and parser of the MIME message.
* MIME message recognizes the most common headers like subject, recipients, content type and so on.
* Encodings that are supported for the message body: Seven bit, Eight bit, Binary, Base64 and Quoted Printable.
* Subject, attachment and name part of the mail address can be in ASCII or UTF-8 format.
* All media types are recognized, including MIME message embedded within another message.
* MIME message has configurable line length policy and strict mode for parsing.
* SMTP implementation with message sending. Both plain and SSL (including START TLS) versions are available.
* POP3 implementation with message receiving and removal, getting mailbox statistics. Both plain and SSL (including START TLS) versions are available.
* IMAP implementation with message receiving and removal, getting mailbox statistics. Both plain and SSL (including START TLS) versions are available.

# Issues #

The library is tested on valid mail servers, so probably there are negative test scenarios that are not covered by the code. In case you find one, please
contact me. Here is a list of issues known so far and planned to be fixed in the future.

* Non-ASCII subject is assumed to be UTF-8.
* Non-ASCII attachment name is assumed to be UTF-8.
* Header attribute cannot contain space between name and value.
* SSL certificate is not verified.

# Roadmap #

The following features are planned:

* version 0.8.0 (Q4/2016): More advanced test scenarios, more examples.
* version 0.9.0 (Q4/2016): Binary content transfer encoding, strict mode for codec parsers.
* version 0.10.0 (Q2/2017): Strict mode for message parser.
* version 0.11.0 (Q2/2017): Q codec to support non-ASCII message subjects.
* version 0.12.0 (Q3/2017): Non-ASCII name part of a mail address.
* version 0.13.0 (Q4/2017): UTF-8 filename of an attachment.
* version 0.14.0 (Q4/2017): Mail content is sent with attachments as another MIME part.
* version 0.15.0 (Q1/2018): Clang and FreeBSD compatibility.

# Contact #

The library is one man show. In case you find a bug, please drop me a mail to contact (at) alepho.com. Since this is my
side project, I'll do my best to be responsive.
