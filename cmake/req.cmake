
SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS system date_time regex random)

find_package(OpenSSL)