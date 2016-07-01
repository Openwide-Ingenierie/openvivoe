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
  /usr/local/include
  /usr/include
)

#Check dependencies
include(FindPkgConfig)
pkg_check_modules(GLIB2 REQUIRED glib-2.0>=2.46.2)
pkg_check_modules(GST 	REQUIRED gstreamer-1.0>=1.6.3)

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

#link that need to be set up to compile the Glibc, needed for the management of configuration files
execute_process(COMMAND "pkg-config" "--libs" "glib-2.0"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE GLIB
				OUTPUT_STRIP_TRAILING_WHITESPACE)

#link for Gstreamer-1.0
execute_process(COMMAND "pkg-config" "--libs" "gstreamer-1.0"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	OUTPUT_VARIABLE GST_LIBS
	OUTPUT_STRIP_TRAILING_WHITESPACE)

#link for Gstreamer-sdp-1.0
execute_process(COMMAND "pkg-config" "--libs" "gstreamer-video-1.0"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	OUTPUT_VARIABLE GST_VIDEO_LIBS
	OUTPUT_STRIP_TRAILING_WHITESPACE)

#link for Gstreamer-sdp-1.0
execute_process(COMMAND "pkg-config" "--libs" "gstreamer-sdp-1.0"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	OUTPUT_VARIABLE GST_SDP_LIBS
	OUTPUT_STRIP_TRAILING_WHITESPACE)

#link for Gstreamer-sdp-1.0
execute_process(COMMAND "pkg-config" "--libs" "gstreamer-rtp-1.0"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	OUTPUT_VARIABLE GST_RTP_LIBS
	OUTPUT_STRIP_TRAILING_WHITESPACE)

#link for Gstreamer-app-1.0
execute_process(COMMAND "pkg-config" "--libs" "gstreamer-app-1.0"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	OUTPUT_VARIABLE GST_APP_LIBS
	OUTPUT_STRIP_TRAILING_WHITESPACE)

#link librairies
target_link_libraries(openvivoe ${SNMP_LIBS} ${SNMPAGENT_LIBS} ${GLIB} ${GST_LIBS} ${GST_VIDEO_LIBS} ${GST_SDP_LIBS} ${GST_RTP_LIBS} ${GST_APP_LIBS})

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

#Cflags needed for the Glibc
execute_process(COMMAND "pkg-config" "--cflags" "glib-2.0"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE GLIBC_FLAGS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

#Cflags for Gstreamer-1.0
execute_process(COMMAND "pkg-config" "--cflags" "gstreamer-1.0"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE GST_FLAGS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

#Cflags for Gstreamer-sdp-1.0
execute_process(COMMAND "pkg-config" "--cflags" "gstreamer-video-1.0"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE GST_VIDEO_FLAGS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

#Cflags for Gstreamer-sdp-1.0
execute_process(COMMAND "pkg-config" "--cflags" "gstreamer-sdp-1.0"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE GST_SDP_FLAGS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

#Cflags for Gstreamer-app-1.0
execute_process(COMMAND "pkg-config" "--cflags" "gstreamer-rtp-1.0"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE GST_RTP_FLAGS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

#Cflags for Gstreamer-app-1.0
execute_process(COMMAND "pkg-config" "--cflags" "gstreamer-app-1.0"
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				OUTPUT_VARIABLE GST_APP_FLAGS
				OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CFLAGS "${CFLAGS} ${SNMP_FLAGS} ${GLIBC_FLAGS} ${GST_FLAGS} ${GST_VIDEO_FLAGS} ${GST_SDP_FLAGS} ${GST_RTP_FLAGS} ${GST_APP_FLAGS}")
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

#display info to user
message(STATUS "CMAKE_GENERATOR 	: ${CMAKE_GENERATOR}")
message(STATUS "PRJ 			: ${PROJECT_NAME}")
message(STATUS "BINARY_DIR 		: ${EXECUTABLE_OUTPUT_PATH}")
message(STATUS "SRC_DIR 		: ${PROJECT_SOURCE_DIR}/src")
message(STATUS "SYSTEM_NAME 		: ${CMAKE_SYSTEM_NAME}")
message(STATUS "BUILD_TYPE 		: ${CMAKE_BUILD_TYPE}")
message(STATUS "LIBS 		:${SNMP_LIBS} ${SNMPAGENT_LIBS} ${GLIB} ${GST_LIBS}")
message(STATUS "CFLAGS 		: ${CFLAGS}")
