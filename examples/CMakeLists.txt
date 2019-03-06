# "inline" function to add mailio sources easily.
function(add_mailio_example SOURCE_FILE)
    get_filename_component(file_name ${SOURCE_FILE} NAME_WE)
    add_executable(${file_name} ${SOURCE_FILE})
    target_link_libraries(${file_name} PUBLIC mailio ${CMAKE_THREAD_LIBS_INIT})
    install(TARGETS ${file_name} DESTINATION "${SHARE_INSTALL_DIR}/${PROJECT_NAME}/examples")
endfunction(add_mailio_example)

# find all the example files.
file(GLOB example_files ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# loop through each file and add the example
foreach(file_name ${example_files})
    add_mailio_example(${file_name})
endforeach(file_name ${example_files})
