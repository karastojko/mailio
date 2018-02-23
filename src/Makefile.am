ACLOCAL_AMFLAGS = -I m4
lib_LTLIBRARIES = libmailio.la
pkginclude_HEADERS = \
	../include/libmailio/base64.hpp \
	../include/libmailio/binary.hpp \
	../include/libmailio/bit7.hpp \
	../include/libmailio/bit8.hpp \
	../include/libmailio/codec.hpp \
	../include/libmailio/dialog.hpp \
        ../include/libmailio/imap.hpp \
        ../include/libmailio/mailboxes.hpp \
        ../include/libmailio/message.hpp \
        ../include/libmailio/mime.hpp \
        ../include/libmailio/pop3.hpp \
        ../include/libmailio/q_codec.hpp \
        ../include/libmailio/quoted_printable.hpp \
        ../include/libmailio/smtp.hpp

AM_CXXFLAGS = -I../include -g -std=c++11 -Wall -Wno-unused-local-typedefs -Wno-unused-variable $(BOOST_CPPFLAGS) $(SSL_CFLAGS) $(CRYPTO_CFLAGS)
LIBADD = -lpthread $(BOOST_SYSTEM_LIBS) $(BOOST_REGEX_LIBS) $(BOOST_DATE_TIME_LIBS) -lboost_random $(SSL_LIBS) $(CRYPTO_LIBS)

libmailio_la_SOURCES = codec.cpp base64.cpp quoted_printable.cpp bit7.cpp bit8.cpp binary.cpp q_codec.cpp mailboxes.cpp mime.cpp message.cpp dialog.cpp smtp.cpp pop3.cpp imap.cpp
