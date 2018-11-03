
if(WIN32)
    SET(Boost_USE_STATIC_LIBS ON)
endif(WIN32)
find_package(Boost REQUIRED COMPONENTS system date_time regex)

find_package(OpenSSL)
