cmake_policy(SET CMP0057 NEW)
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
	#doxygen_add_docs(docs ${LIBSRC} ${LIBINC} COMMENT "Library for MIME, SMTP, POP3 and IMAP over C++11 and Boost.")
	SET(DOCOUTDIR ${PROJECT_BINARY_DIR}/docs)
	SET (DOXYCONF ${PROJECT_BINARY_DIR}/doxygen.conf)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/doxygen.conf.in ${DOXYCONF})
	add_custom_target( docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE}  ${DOXYCONF}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Doxygen ${DOXYCONF}"
        VERBATIM )
	install(DIRECTORY ${PROJECT_BINARY_DIR}/docs/html DESTINATION docs)
	install(DIRECTORY ${PROJECT_BINARY_DIR}/docs/latex DESTINATION docs)
else(DOXYGEN_FOUND)
	message(STATUS "doxygen was not found, documentation will not be built")
endif(DOXYGEN_FOUND)