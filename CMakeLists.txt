cmake_minimum_required (VERSION 3.9.6)

project(mailio VERSION 0.17.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# options for mailio to control how the project is built.
option(MAILIO_BUILD_SHARED_LIBRARY "Turn on to build mailio as a shared library." ON)
option(MAILIO_BUILD_DOCUMENTATION "Turn on to build doxygen based documentation." ON)
option(MAILIO_BUILD_EXAMPLES "Turn on to build examples." ON)

# add a dependent option to build latex documentation or not.
include(CMakeDependentOption)
cmake_dependent_option(MAILIO_BUILD_LATEX_DOCUMENTATION "Build latex docs" ON "MAILIO_BUILD_DOCUMENTATION" ON)

if(WIN32)
    set(Boost_USE_STATIC_LIBS ON)
endif(WIN32)

find_package(Boost REQUIRED COMPONENTS system date_time regex)
find_package(OpenSSL)
set(CMAKE_THREAD_PREFER_PTHREAD)
find_package(Threads)

set(project_sources 
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

set(project_headers
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
)

# handle documentation
if(${MAILIO_BUILD_DOCUMENTATION})
	find_package(Doxygen COMPONENTS doxygen OPTIONAL_COMPONENTS dot)
	if(DOXYGEN_FOUND)
		set(DOCOUTDIR ${PROJECT_BINARY_DIR}/docs)	
		set(DOXYGEN_GENERATE_LATEX ${MAILIO_BUILD_LATEX_DOCUMENTATION})
		set(DOXYGEN_OUTPUT_PATH ${DOCOUTDIR})

		doxygen_add_docs(docs ${PROJECT_SOURCE_DIR}/include WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

		# install the documentation
		install(DIRECTORY ${PROJECT_BINARY_DIR}/docs/html DESTINATION docs)
		install(DIRECTORY ${PROJECT_BINARY_DIR}/docs/latex DESTINATION docs)
	else(DOXYGEN_FOUND)
		message(STATUS "doxygen was not found, documentation will not be built")
	endif(DOXYGEN_FOUND)
endif(${MAILIO_BUILD_DOCUMENTATION})

set(BUILD_SHARED_LIBS ${MAILIO_BUILD_SHARED_LIBRARY})

add_library(${PROJECT_NAME} ${project_sources} ${project_headers})

# generate the export header for exporting symbols
# this is needed to generate a shared library.
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME export.hpp)
target_include_directories(${PROJECT_NAME} 
	PUBLIC
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${CMAKE_CURRENT_BINARY_DIR}>"
		"$<INSTALL_INTERFACE:include>"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/export.hpp DESTINATION include/mailio)

if(Boost_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif()

if(OPENSSL_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
endif()

# install the project headers
install(DIRECTORY include/mailio DESTINATION include)

# install the mailio library.
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# optionally build examples
if(${MAILIO_BUILD_EXAMPLES})
	add_subdirectory(examples)
	file(GLOB PNGS "${PROJECT_SOURCE_DIR}/examples/*.png")
	install(FILES ${PNGS} DESTINATION examples/)
endif(${MAILIO_BUILD_EXAMPLES})
