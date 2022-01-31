# This software was developed at the National Institute of Standards and
# Technology (NIST) by employees of the Federal Government in the course
# of their official duties. Pursuant to title 17 Section 105 of the
# United States Code, this software is not subject to copyright protection
# and is in the public domain. NIST assumes no responsibility  whatsoever for
# its use by other parties, and makes no guarantees, expressed or implied,
# about its quality, reliability, or any other characteristic.

cmake_minimum_required(VERSION 3.16.3)

project(elft_msuimpl)

# Discover name and version
# Sets LIBRARY_NAME and LIBRARY_VERSION in parent context
function(find_name_and_version)
	# Read each line of a file into a list
	file(READ elft_msuimpl.h HEADER_CONTENTS)
	string(REGEX REPLACE ";" "\\\\;" HEADER_CONTENTS "${HEADER_CONTENTS}")
	string(REGEX REPLACE "\n" ";" HEADER_CONTENTS "${HEADER_CONTENTS}")

	foreach(line ${HEADER_CONTENTS})
		# Find line with version number
		string(REGEX MATCH ".*versionNumber{0x(....)}.*" VERSION_LINE ${line})
		if(VERSION_LINE AND NOT LIBRARY_VERSION)
			string(TOUPPER ${CMAKE_MATCH_1} LIBRARY_VERSION)
			set(LIBRARY_VERSION ${LIBRARY_VERSION} PARENT_SCOPE)
		endif()

		# Find line with library name
		string(REGEX MATCH ".*libraryIdentifier{\"(.+)\"}.*" NAME_LINE ${line})
		if(NAME_LINE AND NOT LIBRARY_NAME)
			set(LIBRARY_NAME ${CMAKE_MATCH_1} PARENT_SCOPE)
		endif()
	endforeach()
endfunction()

find_name_and_version()
if(NOT LIBRARY_NAME OR NOT LIBRARY_VERSION)
	message(FATAL_ERROR "Could not discover library name or version")
endif()
set(LIB_NAME "elft_${LIBRARY_NAME}_${LIBRARY_VERSION}")

add_library(${LIB_NAME} SHARED)
target_sources(${LIB_NAME} PRIVATE elft_msuimpl.cpp elft_msuimpl_enrolldb.cpp)
target_include_directories(${LIB_NAME} PRIVATE submodules/elft_public/elft_1_x/include)
target_include_directories(${LIB_NAME} PRIVATE ${PROJECT_SOURCE_DIR})

# Depend on libelft
add_subdirectory(submodules/elft_public/elft_1_x/libelft ${CMAKE_CURRENT_BINARY_DIR}/libelft)
add_dependencies(${LIB_NAME} elft)
target_link_libraries(${LIB_NAME} PUBLIC elft)

# Depend on msu-compare
add_subdirectory(${CMAKE_SOURCE_DIR}/libmsu-compare)
add_dependencies(${LIB_NAME} msu-compare)
target_link_libraries(${LIB_NAME} PUBLIC msu-compare)
target_include_directories(${LIB_NAME} SYSTEM PRIVATE submodules/msu-latentafis/matching)

# Turn on warnings
target_compile_options(${LIB_NAME} PRIVATE
   -Wall -Wextra -pedantic -Wconversion -Wsign-conversion)
# Turn off unused variable warning
target_compile_options(${LIB_NAME} PRIVATE
   -Wno-unused-parameter)

# Set RPATH to $ORIGIN
set_target_properties(${LIB_NAME} PROPERTIES
    BUILD_RPATH_USE_ORIGIN TRUE
    INSTALL_RPATH $ORIGIN
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

include(GNUInstallDirs)
install(TARGETS ${LIB_NAME}
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
