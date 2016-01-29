#
# Licence: GPL
# Created: Tue, 12 Jan 2016 14:37:39 +0100
# Main authors:
#     - hoel <hvasseur@openwide.fr>
#


#set Cmake minimum required version
cmake_minimum_required(VERSION 2.8)

#projet name and version
project(streaming)
# The version number.
set (Tutorial_VERSION_MAJOR 0)
set (Tutorial_VERSION_MINOR 1)

#add directories of project
include_directories(
  streaming/src
  streaming/bin
  streaming/include
  /usr/local/include
  /usr/include
)

#add source and header 
file(GLOB_RECURSE streaming_SRC src/*)
file(GLOB_RECURSE streaming_HEADER include/*)
add_executable(streaming ${streaming_SRC} ${streaming_HEADER} )

#link that need to be set up to compile the Glibc, needed for the management of configuration files
execute_process(COMMAND "pkg-config" "--libs" "glib-2.0"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_VARIABLE GLIB				
		OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND "pkg-config" "--libs" "gstreamer-1.0"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_VARIABLE GST_LIBS				
		OUTPUT_STRIP_TRAILING_WHITESPACE)

target_link_libraries(streaming ${GLIB} ${GST_LIBS})

#set debug as default configuration, to buld release run "cmake -DCMAKE_BUILD_TYPE=Release" or simply the alias cmakerelease
if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE DEBUG CACHE STRING
		"Build type set default as not specified."
		FORCE )
endif()

execute_process(COMMAND "pkg-config" "--cflags" "glib-2.0"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_VARIABLE GLIBC_FLAGS
		OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND "pkg-config" "--cflags" "gstreamer-1.0"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_VARIABLE GST_FLAGS
		OUTPUT_STRIP_TRAILING_WHITESPACE)

set( CFLAGS "${CFLAGS} ${GLIBC_FLAGS} ${GST_FLAGS}")

#Compilation flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -I. ${CFLAGS}") #always
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -00") #for debugging
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")#for release

#set ouput directory based on build type
if(CMAKE_BUILD_TYPE STREQUAL DEBUG)
	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/Debug) #debug VERSION
elseif ( CMAKE_BUILD_TYPE STREQUAL RELEASE)
	SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/Release)#release version
else()
	SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)#default version
endif()


#display info to user
message(STATUS "CMAKE_GENERATOR      : ${CMAKE_GENERATOR}")
message(STATUS "PRJ 		        : ${PROJECT_NAME}")
message(STATUS "BINARY_DIR           : ${EXECUTABLE_OUTPUT_PATH}")
message(STATUS "SRC_DIR 		: ${PROJECT_SOURCE_DIR}/src")
message(STATUS "SYSTEM_NAME 		: ${CMAKE_SYSTEM_NAME}")
message(STATUS "BUILD_TYPE  		: ${CMAKE_BUILD_TYPE}")
message(STATUS "CFLAGS  		: ${CFLAGS}")
message(STATUS "GLIB  		: ${GLIB}")

