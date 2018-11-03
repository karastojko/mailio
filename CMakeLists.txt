cmake_minimum_required (VERSION 2.8)
cmake_policy(SET CMP0048 NEW)
set(PROJ mailio)
project(${PROJ} VERSION 0.17.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBNAME ${PROJ}-static)
set(DLLNAME ${PROJ})
if(WIN32)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(LIBNAME ${PROJ}-static_d )
        set(DLLNAME ${PROJ}_d)
    endif()
endif(WIN32)
if(WIN32)
    set(Boost_USE_STATIC_LIBS ON)
endif(WIN32)

find_package(Boost REQUIRED COMPONENTS system date_time regex)
find_package(OpenSSL)
find_package(Threads)

set(LIBSRC 
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

set(LIBINC
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

cmake_policy(SET CMP0057 NEW)
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
    SET(DOCOUTDIR ${PROJECT_BINARY_DIR}/docs)
    SET (DOXYCONF ${PROJECT_BINARY_DIR}/doxygen.conf)
    configure_file(${PROJECT_SOURCE_DIR}/doxygen.conf.in ${DOXYCONF})
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

include_directories(include)

add_library(${LIBNAME} STATIC ${LIBSRC} ${LIBINC})
add_library(${DLLNAME} SHARED ${LIBSRC} ${LIBINC})
if(WIN32)
    target_compile_definitions(${DLLNAME} PUBLIC "-DMAILIO_BUILDING_DLL")
endif(WIN32)

if(Boost_FOUND)
    target_include_directories(${LIBNAME} PUBLIC ${Boost_INCLUDE_DIRS})
    target_include_directories(${DLLNAME} PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(${DLLNAME} ${Boost_LIBRARIES})
endif()

if(OPENSSL_FOUND)
    target_include_directories(${LIBNAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_include_directories(${DLLNAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${DLLNAME} ${OPENSSL_LIBRARIES})
endif()

file(GLOB HDRS ${PROJECT_SOURCE_DIR}/include/mailio/*.hpp)

install(FILES ${HDRS} DESTINATION include/mailio)
install(TARGETS ${LIBNAME} ${DLLNAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(imap_remove_msg examples/imap_remove_msg.cpp)
add_executable(imaps_stat examples/imaps_stat.cpp)
add_executable(pop3_remove_msg examples/pop3_remove_msg.cpp)
add_executable(pop3s_attachment examples/pop3s_attachment.cpp)
add_executable(pop3s_fetch_one examples/pop3s_fetch_one.cpp)
add_executable(smtps_attachment examples/smtps_attachment.cpp)
add_executable(smtps_simple_msg examples/smtps_simple_msg.cpp)
add_executable(smtp_utf8_qp_msg examples/smtp_utf8_qp_msg.cpp)
set_target_properties(imap_remove_msg imaps_stat pop3_remove_msg pop3s_attachment pop3s_fetch_one smtps_attachment smtps_simple_msg smtp_utf8_qp_msg PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples")
target_link_libraries(imap_remove_msg ${LIBNAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(imaps_stat ${LIBNAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(pop3_remove_msg ${LIBNAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(pop3s_attachment ${LIBNAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(pop3s_fetch_one ${LIBNAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(smtps_attachment ${LIBNAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(smtps_simple_msg ${LIBNAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(smtp_utf8_qp_msg ${LIBNAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
install(TARGETS imap_remove_msg imaps_stat pop3_remove_msg pop3s_attachment pop3s_fetch_one smtps_attachment smtps_simple_msg smtp_utf8_qp_msg DESTINATION examples/)
file(GLOB PNGS "${PROJECT_SOURCE_DIR}/examples/*.png")
install(FILES ${PNGS} DESTINATION examples/)
