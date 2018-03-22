SET(PROJ mailio)

SET(LIBNAME ${PROJ}-static)

SET(DLLNAME ${PROJ})

if(WIN32)
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		SET(LIBNAME ${PROJ}-static_d )
		SET(DLLNAME ${PROJ}_d)
	endif()
endif(WIN32)