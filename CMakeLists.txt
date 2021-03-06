#
# Licence: GPL
# Created: Tue, 12 Jan 2016 14:37:39 +0100
# Main authors:
#     - hoel <hvasseur@openwide.fr>
#

#set Cmake minimum required version
cmake_minimum_required(VERSION 2.8)

#projet name and version
project(openvivoe)

# The version number.
set (Tutorial_VERSION_MAJOR 0)
set (Tutorial_VERSION_MINOR 1)

#add directories of project
include_directories(
  src
  bin
  include
)

#Check dependencies
include(FindPkgConfig)
pkg_check_modules(GLIB2     REQUIRED glib-2.0>=2.46.2)
pkg_check_modules(GST       REQUIRED gstreamer-1.0>=1.6.3 gstreamer-video-1.0>=1.6.3 gstreamer-rtp-1.0>=1.6.3  gstreamer-sdp-1.0>=1.6.3 gstreamer-app-1.0>=1.6.3)

#Special case of net-snmp (that is not referenced by pkg-config)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}")
find_package(NetSnmp)

#Find doxygen package, required to make doc
find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile @ONLY)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc
		COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)


#add source and header
file(GLOB_RECURSE openvivoe_SRC 	src/*)
file(GLOB_RECURSE openvivoe_HEADER 	include/*)
add_executable(openvivoe ${openvivoe_SRC} ${openvivoe_HEADER} )

#check if net-snmp types are well defined and included
include(CheckIncludeFiles)

CHECK_INCLUDE_FILES( inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES( stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES( unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES( string.h HAVE_STRING_H)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)


#
#Link specific libraries to target
#

#link that need to be set up to use net-snmp API
execute_process(COMMAND "net-snmp-config" "--libs"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE SNMP_LIBS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

#link that need to be set up to compile an Agent using net-snmp API
execute_process(COMMAND "net-snmp-config" "--agent-libs"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE SNMPAGENT_LIBS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

#link librairies
target_link_libraries(openvivoe
	${SNMP_LIBS}
	${SNMPAGENT_LIBS}
	${GLIB2_LIBRARIES}
	${GST_LIBRARIES}
	)

#include directories
target_include_directories(openvivoe PUBLIC ${GLIB2_INCLUDE_DIRS} ${GST_INCLUDE_DIRS})

#compile flags
target_compile_options(openvivoe PUBLIC ${GLIB2_OTHER_CFLAGS} ${GST_OTHER_CFLAGS})

#set debug as default configuration, to buld release run "cmake -DCMAKE_BUILD_TYPE=Release" or simply the alias cmakerelease
if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE DEBUG CACHE STRING
		 "Build type set to default as not specified."
		 FORCE )
endif()

#Compilation flags
#C_FLAGS needed, net-snmp-config provide an option to get the CFLAGS to use for compilation using net-snmp API
execute_process(COMMAND "net-snmp-config" "--cflags"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE SNMP_FLAGS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CFLAGS "${CFLAGS} ${SNMP_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ${CFLAGS}") #always
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -00 -v -da -Q") #for debugging
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall ")#for release

#set ouput directory based on build type
if( CMAKE_BUILD_TYPE STREQUAL DEBUG)
	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/Debug) #debug version
elseif ( CMAKE_BUILD_TYPE STREQUAL RELEASE)
	SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/Release)#release version
else()
	SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)#default version
endif()

# the install rules for OpenVivoe
INSTALL( TARGETS openvivoe DESTINATION bin )
INSTALL( FILES conf/vivoe-mib.conf conf/vivoe-stream.conf DESTINATION /etc/openvivoe )
INSTALL( FILES conf/mibs/VIVOE-MIB DESTINATION /usr/share/snmp/mibs )

#display info to user
message(STATUS "CMAKE_GENERATOR 	: ${CMAKE_GENERATOR}")
message(STATUS "PRJ 			: ${PROJECT_NAME}")
message(STATUS "BINARY_DIR 		: ${EXECUTABLE_OUTPUT_PATH}")
message(STATUS "SRC_DIR 		: ${PROJECT_SOURCE_DIR}/src")
message(STATUS "SYSTEM_NAME 		: ${CMAKE_SYSTEM_NAME}")
message(STATUS "BUILD_TYPE 		: ${CMAKE_BUILD_TYPE}")
