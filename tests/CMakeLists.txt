cmake_minimum_required(VERSION 3.16.3)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

set(PROJECT_NAME test_mailio)
find_package(Boost REQUIRED COMPONENTS system date_time regex)
find_package(OpenSSL)

enable_testing()

add_executable(
  ${PROJECT_NAME}
  test_message.cpp
  ${project_sources}
)
target_link_libraries(
  ${PROJECT_NAME}
  gtest_main
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/ PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/../ PUBLIC ${Boost_INCLUDE_DIRS} PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})


include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

