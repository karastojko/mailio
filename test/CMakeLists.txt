 cmake_minimum_required(VERSION 3.16.3)

set(PROJECT_NAME test_message)
find_package(Boost REQUIRED COMPONENTS system date_time regex unit_test_framework)
find_package(OpenSSL)

enable_testing()

add_executable(
  ${PROJECT_NAME}
  test_message.cpp
  ${project_sources}
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/ PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/../ PUBLIC ${Boost_INCLUDE_DIRS} PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})


