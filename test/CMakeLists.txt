# Dependencies
find_package("Boost" REQUIRED COMPONENTS "unit_test_framework")

# Makes a new target for each test file.
function(make_test test_file)
    # Target
    enable_testing()
    include("CTest")
    get_filename_component(TEST_TARGET "${test_file}" NAME_WE)
    add_executable("${TEST_TARGET}" ${test_file})
    add_test(
        NAME "${TEST_TARGET}"
        COMMAND "${TEST_TARGET}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")

    if(${MAILIO_DYN_LINK_TESTS})
        target_compile_definitions("${TEST_TARGET}" PRIVATE "BOOST_TEST_DYN_LINK")
    endif()

    # Link libraries
    target_link_libraries("${TEST_TARGET}" PRIVATE "${PROJECT_NAME}")
    target_link_libraries("${TEST_TARGET}" PRIVATE "Boost::unit_test_framework")
endfunction()

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(file ${TEST_FILES})
    make_test("${file}")
endforeach()
