#
# Licence: GPL
# Created: Tue, 12 Jan 2016 14:37:39 +0100
# Main authors:
#     - hoel <hvasseur@openwide.fr>
#


#set Cmake minimum required version
cmake_minimum_required(VERSION 2.8)

#projet name and version
project(mib_module)
# The version number.
set (Tutorial_VERSION_MAJOR 0)
set (Tutorial_VERSION_MINOR 1)

#add directories of project
include_directories(
  mib_module/src
  mib_module/bin
  mib_module/include
  /usr/local/include
  /usr/include
)

#add source and header 
file(GLOB_RECURSE mib_module_SRC src/*)
file(GLOB_RECURSE mib_module_HEADER include/*)
add_executable(mib_module ${mib_module_SRC} ${mib_module_HEADER} )


#Link specific libraries to target: here net-snmp
#link that need to be set up to use net-snmp API
execute_process(COMMAND "net-snmp-config" "--libs"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE BUILDLIBS
				OUTPUT_STRIP_TRAILING_WHITESPACE)
#link that need to be set up to compile an Agent using net-snmp API
execute_process(COMMAND "net-snmp-config" "--agent-libs"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE BUILDAGENT_LIBS
				OUTPUT_STRIP_TRAILING_WHITESPACE)
target_link_libraries(mib_module ${BUILDLIBS} ${BUILDAGENT_LIBS})

#set debug as default configuration, to buld release run "cmake -DCMAKE_BUILD_TYPE=Release" or simply the alias cmakerelease
if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE DEBUG CACHE STRING
		"Build type set default as not specified."
		FORCE )
endif()

#Compilation flags
#C_FLAGS needed, net-snmp-config provide an option to get the CFLAGS to use for compilation using net-snmp API
execute_process(COMMAND "net-snmp-config" "--cflags"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE CFLAGS
				OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -I. ${CFLAGS}") #always
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --ggdb") #for debugging
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")#for release

#set ouput directory based on build type
if( CMAKE_BUILD_TYPE STREQUAL DEBUG)
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
message(STATUS "BUILDLIBS    			: ${BUILDLIBS}")
message(STATUS "BUILDAGENT_LIBS    			: ${BUILDAGENT_LIBS}")
message(STATUS "CFLAGS    			: ${CFLAGS}")


