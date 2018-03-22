
set( LIBSRC 
	${PROJECT_SOURCE_DIR}/src/base64.cpp
	${PROJECT_SOURCE_DIR}/src/binary.cpp
	${PROJECT_SOURCE_DIR}/src/bit7.cpp
	${PROJECT_SOURCE_DIR}/src/bit8.cpp
	${PROJECT_SOURCE_DIR}/src/codec.cpp
	${PROJECT_SOURCE_DIR}/src/dialog.cpp
	${PROJECT_SOURCE_DIR}/src/imap.cpp
	${PROJECT_SOURCE_DIR}/src/mailboxes.cpp
	${PROJECT_SOURCE_DIR}/src/message.cpp
	${PROJECT_SOURCE_DIR}/src/mime.cpp
	${PROJECT_SOURCE_DIR}/src/pop3.cpp
	${PROJECT_SOURCE_DIR}/src/quoted_printable.cpp
	${PROJECT_SOURCE_DIR}/src/q_codec.cpp
	${PROJECT_SOURCE_DIR}/src/smtp.cpp
)

set( LIBINC
	${PROJECT_SOURCE_DIR}/include/mailio/base64.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/binary.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/bit7.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/bit8.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/codec.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/dialog.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/imap.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/mailboxes.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/message.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/mime.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/pop3.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/quoted_printable.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/q_codec.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/smtp.hpp
	${PROJECT_SOURCE_DIR}/include/mailio/export.hpp
)