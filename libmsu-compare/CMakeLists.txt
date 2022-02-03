# This software was developed at the National Institute of Standards and
# Technology (NIST) by employees of the Federal Government in the course
# of their official duties. Pursuant to title 17 Section 105 of the
# United States Code, this software is not subject to copyright protection
# and is in the public domain. NIST assumes no responsibility  whatsoever for
# its use by other parties, and makes no guarantees, expressed or implied,
# about its quality, reliability, or any other characteristic.

cmake_minimum_required(VERSION 3.16.3)

project(msu-latentafis)

set(LIB_NAME "msu-compare")

add_library(${LIB_NAME} SHARED)
target_sources(${LIB_NAME} PRIVATE
	../submodules/msu-latentafis/matching/matcher.cpp)

# Turn on warnings
target_compile_options(${LIB_NAME} PRIVATE
	-Wall -Wextra -pedantic)
# Turn off unused variable warning
target_compile_options(${LIB_NAME} PRIVATE
	-Wno-unused-parameter)

set_target_properties(${LIB_NAME} PROPERTIES
	# Set RPATH to $ORIGIN
	BUILD_RPATH_USE_ORIGIN TRUE
	INSTALL_RPATH $ORIGIN
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED ON
)

# Use RUNPATH over RPATH
if(NOT APPLE)
	set_target_properties(${LIB_NAME} PROPERTIES
	    LINK_FLAGS -Wl,--enable-new-dtags)
endif()

include(GNUInstallDirs)
install(TARGETS ${LIB_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

#
# Dependencies
#
find_package(Eigen3 REQUIRED)
target_link_libraries(${LIB_NAME} PRIVATE Eigen3::Eigen)

# OpenMP not required
option(WITH_OPENMP "Use OpenMP" OFF)
if (WITH_OPENMP)
	find_package(OpenMP)
	if (TARGET OpenMP::OpenMP_CXX)
		target_link_libraries(${LIB_NAME} PRIVATE OpenMP::OpenMP_CXX)
	else()
		message(FATAL_ERROR "OpenMP requested, but not found")
	endif()
endif()

find_package(Boost REQUIRED COMPONENTS filesystem)
target_link_libraries(${LIB_NAME} PRIVATE Boost::filesystem)

