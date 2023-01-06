cmake_minimum_required(VERSION 3.16.3)

set(PROJECT_NAME test_message)
find_package(Boost REQUIRED COMPONENTS system date_time regex unit_test_framework)
find_package(OpenSSL)

add_executable(
  ${PROJECT_NAME}
  test_message.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/ PUBLIC ${Boost_INCLUDE_DIRS} PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC mailio ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})


