cmake_minimum_required (VERSION 2.8)

include(cmake/vars.cmake)

cmake_policy(SET CMP0048 NEW)
project (${PROJ} VERSION 0.7.0)

include(cmake/src.cmake)
include(cmake/req.cmake)

include_directories(include)

add_library(${LIBNAME} STATIC ${LIBSRC} ${LIBINC})

add_library(${DLLNAME} SHARED ${LIBSRC} ${LIBINC})
target_compile_definitions(${DLLNAME} PUBLIC "-DMAILIO_BUILDING_DLL")

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

FILE(GLOB HDRS ${PROJECT_SOURCE_DIR}/include/mailio/*.hpp)

install(FILES ${HDRS}  
		 DESTINATION include/mailio
)

install(TARGETS ${LIBNAME} ${DLLNAME}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)

include(cmake/doxy.cmake)