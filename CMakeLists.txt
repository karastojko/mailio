cmake_minimum_required (VERSION 3.16.3)

project(mailio VERSION 0.21.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED LIB_SUFFIX)
  set(LIB_SUFFIX "" CACHE STRING "Suffix for installed library path. Ex. 64 for lib64")
endif(NOT DEFINED LIB_SUFFIX)

# Set bindir, if not use -DBIN_INSTALL_DIR
if(NOT BIN_INSTALL_DIR)
  if(CMAKE_INSTALL_BINDIR)
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
  else(CMAKE_INSTALL_BINDIR)
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
  endif(CMAKE_INSTALL_BINDIR)
endif(NOT BIN_INSTALL_DIR)

# Set libdir, if not use -DLIB_INSTALL_DIR
if(NOT LIB_INSTALL_DIR)
  if(CMAKE_INSTALL_LIBDIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
  else(CMAKE_INSTALL_LIBDIR)
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
  endif(CMAKE_INSTALL_LIBDIR)
endif(NOT LIB_INSTALL_DIR)

# Set includedir, if not use -DINCLUDE_INSTALL_DIR
if(NOT INCLUDE_INSTALL_DIR)
  if(CMAKE_INSTALL_INCLUDEDIR)
    set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
  else(CMAKE_INSTALL_INCLUDEDIR)
    set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
  endif(CMAKE_INSTALL_INCLUDEDIR)
endif(NOT INCLUDE_INSTALL_DIR)

# Set sharedir, if not use -DSHARE_INSTALL_DIR
if(NOT SHARE_INSTALL_DIR)
  if(CMAKE_INSTALL_DATADIR)
    set(SHARE_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}")
  else(CMAKE_INSTALL_DATADIR)
    set(SHARE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share")
  endif(CMAKE_INSTALL_DATADIR)
endif(NOT SHARE_INSTALL_DIR)

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
# "Use of both the imported target as well as this switch is highly recommended for new code."
set(THREADS_PREFER_PTHREAD_FLAG)
find_package(Threads)

set(project_sources 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base64.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/binary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bit7.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bit8.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mailboxes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pop3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/quoted_printable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/q_codec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/smtp.cpp
)

set(project_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/base64.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/binary.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/bit7.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/bit8.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/codec.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/dialog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/imap.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/mailboxes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/message.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/mime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/pop3.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/quoted_printable.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/q_codec.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mailio/smtp.hpp
)

# handle documentation
if(${MAILIO_BUILD_DOCUMENTATION})
    find_package(Doxygen COMPONENTS doxygen OPTIONAL_COMPONENTS dot)
    if(DOXYGEN_FOUND)
        set(DOCOUTDIR ${PROJECT_BINARY_DIR}/docs)	
        set(DOXYGEN_GENERATE_LATEX ${MAILIO_BUILD_LATEX_DOCUMENTATION})
        set(DOXYGEN_OUTPUT_PATH ${DOCOUTDIR})

        doxygen_add_docs(docs ${CMAKE_CURRENT_SOURCE_DIR}/include WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        # install the documentation
        install(DIRECTORY ${PROJECT_BINARY_DIR}/docs/html DESTINATION docs)
        install(DIRECTORY ${PROJECT_BINARY_DIR}/docs/latex DESTINATION docs)
    else(DOXYGEN_FOUND)
        message(STATUS "doxygen was not found, documentation will not be built")
    endif(DOXYGEN_FOUND)
endif(${MAILIO_BUILD_DOCUMENTATION})

set(BUILD_SHARED_LIBS ${MAILIO_BUILD_SHARED_LIBRARY})

add_library(${PROJECT_NAME} ${project_sources} ${project_headers})


# pkg-config support
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}" )

if(IS_ABSOLUTE "${LIB_INSTALL_DIR}")
  set(libdir "${LIB_INSTALL_DIR}")
else(IS_ABSOLUTE "${LIB_INSTALL_DIR}")
  set(libdir "\${exec_prefix}/${LIB_INSTALL_DIR}")
endif(IS_ABSOLUTE "${LIB_INSTALL_DIR}")

if(IS_ABSOLUTE "${INCLUDE_INSTALL_DIR}")
  set(includedir "${INCLUDE_INSTALL_DIR}/${PROJECT_NAME}")
else(IS_ABSOLUTE "${INCLUDE_INSTALL_DIR}")
  set(includedir "\${prefix}/${INCLUDE_INSTALL_DIR}/${PROJECT_NAME}")
endif(IS_ABSOLUTE "${INCLUDE_INSTALL_DIR}")

configure_file(mailio.pc.in ${CMAKE_BINARY_DIR}/mailio.pc IMMEDIATE @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/mailio.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in version.hpp)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")
install(FILES ${CMAKE_BINARY_DIR}/version.hpp DESTINATION ${INCLUDE_INSTALL_DIR}/${PROJECT_NAME})

# generate the export header for exporting symbols
# this is needed to generate a shared library.
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME export.hpp)
target_include_directories(${PROJECT_NAME} 
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_BINARY_DIR}>"
        "$<INSTALL_INTERFACE:include>"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/export.hpp DESTINATION "${INCLUDE_INSTALL_DIR}/${PROJECT_NAME}")

if(Boost_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif()

if(OPENSSL_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
endif()

install(DIRECTORY include/mailio DESTINATION ${INCLUDE_INSTALL_DIR})

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
)

# optionally build examples
if(${MAILIO_BUILD_EXAMPLES})
    add_subdirectory(examples)
    file(GLOB PNGS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.png")
    install(FILES ${PNGS} DESTINATION "${SHARE_INSTALL_DIR}/${PROJECT_NAME}/examples/")
endif(${MAILIO_BUILD_EXAMPLES})
