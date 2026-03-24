# Makes a new target for each example file.
function(make_example example_file)
    # Target
    get_filename_component(EXAMPLE_TARGET "${example_file}" NAME_WE)
    add_executable("${EXAMPLE_TARGET}" "${example_file}")

    # Link libraries
    target_link_libraries("${EXAMPLE_TARGET}" PRIVATE "${PROJECT_NAME}")
endfunction()

file(GLOB EXAMPLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(file ${EXAMPLE_FILES})
    make_example("${file}")
endforeach()
