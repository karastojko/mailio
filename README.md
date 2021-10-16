
# mailio #

*mailio* is a cross platform C++ library for MIME format and SMTP, POP3 and IMAP protocols. It is based on the standard C++ 17 and Boost library.


# Examples #

To send a mail, one has to create `message` object and set it's attributes as author, recipient, subject and so on. Then, an SMTP connection
is created by constructing `smtp` (or `smtps`) class. The message is sent over the connection:

```cpp
message msg;
msg.from(mail_address("mailio library", "mailio@gmail.com"));
msg.add_recipient(mail_address("mailio library", "mailio@gmail.com"));
msg.subject("smtps simple message");
msg.content("Hello, World!");

smtps conn("smtp.gmail.com", 587);
conn.authenticate("mailio@gmail.com", "mailiopass", smtps::auth_method_t::START_TLS);
conn.submit(msg);
```

To receive a mail, a `message` object is created to store the received message. Mail can be received over POP3 or IMAP, depending of mail server setup.
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

Note for Gmail users: if 2FA is turned on, then instead of the primary password, the application password must be used. Follow
[Gmail instructions](https://support.google.com/accounts/answer/185833) to add *mailio* as trusted application and use the generated password for all three
protocols.

Note for Zoho users: if 2FA is turned on, then instead of the primary password, the application password must be used. Follow
[Zoho instructions](https://www.zoho.com/mail/help/adminconsole/two-factor-authentication.html#alink5) to add *mailio* as trusted application and use the
generated password for all three protocols.


# Requirements #

*mailio* library is supposed to work on all platforms supporting C++ 17 compiler, recent Boost libraries and CMake build tool.

For Linux the following configuration is tested:
* Ubuntu 20.04.3 LTS.
* Gcc 8.3.0.
* Boost 1.66 with Regex, Date Time available.
* POSIX Threads, OpenSSL and Crypto libraries available on the system.
* CMake 3.16.3

For FreeBSD the following configuration is tested:
* FreeBSD 13.
* Clang 11.0.1.
* Boost 1.72.0 (port).
* CMake 3.21.3.

For MacOS the following configuration is tested:
* Apple LLVM 9.0.0.
* Boost 1.66.
* OpenSSL 1.0.2n available on the system.
* CMake 3.16.3.

For Microsoft Windows the following configuration is tested:
* Windows 10.
* Visual Studio 2019 Community Edition.
* Boost 1.71.0.
* OpenSSL 1.0.2t.
* CMake 3.17.3.

For Cygwin the following configuration is tested:
* Cygwin 3.2.0 on Windows 10.
* Gcc 10.2.
* Boost 1.66.
* CMake 3.20.
* LibSSL 1.0.2t and LibSSL 1.1.1f development packages.

For MinGW the following configuration is tested:
* MinGW 17.1 on Windows 10 which comes with the bundled Gcc and Boost.
* Gcc 9.2.
* Boost 1.71.0.
* OpenSSL 1.0.2t.
* CMake 3.17.3.

# Setup #

There are two ways to build *mailio*: by cloning the [repo](https://github.com/karastojko/mailio.git) and using Cmake or by using Vcpkg.


## CMake ##

Ensure that OpenSSL, Boost and CMake are in the path. If they are not in the path, one could use CMake options `-DOPENSSL_ROOT_DIR` and `-DBOOST_ROOT` to set
them. Boost must be built with the OpenSSL support. If it cannot be found in the path, set the path explicitly via `library-path` and `include` parameters of
`b2` script (after `bootstrap` finishes). Both static and dynamic libraries should be built in the `build` directory. If one wants to specify non-default
installation directory say `/opt/mailio`, then use the CMake option `-DCMAKE_INSTALL_PREFIX`. Other available options are `MAILIO_BUILD_SHARED_LIBRARY`
(by default is on, if turned off then the static library is built), `MAILIO_BUILD_DOCUMENTATION` (if Doxygen documentation is generated, by default is on)
and `MAILIO_BUILD_EXAMPLES` (if examples are built, by default is on).


### Linux, FreeBSD, MacOS, Cygwin ###

From the terminal go into the directory where the library is downloaded to, and execute:
```
mkdir build
cd ./build
cmake ..
make install
```


### Microsoft Windows/Visual Studio ###

From the command prompt go into the directory where the library is downloaded, and execute:
```
mkdir build
cd .\build
cmake ..
```
A solution file will be built, open it from Visual Studio and build the project.


### Microsoft Windows/MinGW ###

Open the command prompt by using the `open_distro_window.bat` script, and execute:
```
mkdir build
cd .\build
cmake.exe .. -G "MinGW Makefiles"
make install
```


## Vcpkg ##

Install [Vcpkg](https://github.com/microsoft/vcpkg) and run:
```
vcpkg install mailio
```


# Features #

* Recursive formatter and parser of the MIME message.
* MIME message recognizes the most common headers like subject, recipients, content type and so on.
* Encodings that are supported for the message body: Seven bit, Eight bit, Binary, Base64 and Quoted Printable.
* Subject, attachment and name part of the mail address can be in ASCII or UTF-8 format.
* All media types are recognized, including MIME message embedded within another message.
* MIME message has configurable line length policy and strict mode for parsing.
* SMTP implementation with message sending. Both plain and SSL (including START TLS) versions are available.
* POP3 implementation with message receiving and removal, getting mailbox statistics. Both plain and SSL (including START TLS) versions are available.
* IMAP implementation with message receiving, removal and search, getting mailbox statistics, managing folders. Both plain and SSL (including START TLS)
  versions are available.


# Issues #

The library is tested on valid mail servers, so probably there are negative test scenarios that are not covered by the code. In case you find one, please
contact me. Here is a list of issues known so far and planned to be fixed in the future.

* Non-ASCII subject is assumed to be UTF-8.
* Non-ASCII attachment name is assumed to be UTF-8.
* Header attribute cannot contain space between name and value.
* SSL certificate is not verified.
* SSL version not configurable (v2.3 hardcoded).


# Contributors #

* [Trevor Mellon](https://github.com/TrevorMellon): CMake build scripts.
* [Kira Backes](mailto:kira.backes[at]nrwsoft.de): Fix for correct default message date.
* [sledgehammer_999](mailto:hammered999[at]gmail.com): Replacement of Boost random function with the standard one.
* [Paul Tsouchlos](mailto:developer.paul.123[at]gmail.com): Modernizing build scripts.
* [Anton Zhvakin](mailto:a.zhvakin[at]galament.com): Replacement of deprecated Boost Asio entities.
* [terminator356](mailto:termtech[at]rogers.com): IMAP searching, fetching messages by UIDs.
* [Ilya Tsybulsky](mailto:ilya.tsybulsky[at]gmail.com): MIME parsing and formatting issues. UIDL command for POP3.
* [Ayaz Salikhov](https://github.com/mathbunnyru): Conan packaging.
* [Tim Lukas Harken](tlh[at]tlharken.de): Removing compilation warnings.
* [Rainer Sabelka](mailto:saba[at]sabanet.at]): SMTP server response on accepting a mail.
* [David Garcia](mailto:david.garcia[at]antiteum.com): Vcpkg port.
* [ImJustStokedToBeHere](https://github.com/ImJustStokedToBeHere): Typo error in IMAP.
* [lifof](mailto:youssef.beddad[at]gmail.com): Support for the MinGW compilation.


# Contact #

In case you find a bug, please drop me a mail to contact (at) alepho.com. Since this is my side project, I'll do my best to be responsive.
