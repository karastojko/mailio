ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = smtps_simple_msg smtp_utf8_qp_msg smtps_attachment \
	pop3s_fetch_one pop3_remove_msg pop3s_attachment \
	imaps_stat imap_remove_msg

AM_CXXFLAGS = -I../include -g -std=c++11 -Wall -Wno-unused-local-typedefs -Wno-unused-variable $(BOOST_CPPFLAGS) $(SSL_CFLAGS) $(CRYPTO_CFLAGS)
LDADD = -lpthread -L../src $(BOOST_SYSTEM_LIBS) $(BOOST_REGEX_LIBS) $(BOOST_DATE_TIME_LIBS) -lboost_random $(SSL_LIBS) $(CRYPTO_LIBS) -lmailio

smtps_simple_msg_SOURCES = smtps_simple_msg.cpp

smtp_utf8_qp_msg_SOURCES = smtp_utf8_qp_msg.cpp

smtps_attachment_SOURCES = smtps_attachment.cpp

pop3s_fetch_one_SOURCES = pop3s_fetch_one.cpp

pop3_remove_msg_SOURCES = pop3_remove_msg.cpp

pop3s_attachment_SOURCES = pop3s_attachment.cpp

imaps_stat_SOURCES = imaps_stat.cpp

imap_remove_msg_SOURCES = imap_remove_msg.cpp
