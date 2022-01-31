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
target_include_directories(${LIB_NAME} PRIVATE ${PROJECT_SOURCE_DIR})

# Turn on warnings
target_compile_options(${LIB_NAME} PRIVATE
	-Wall -Wextra -pedantic)
# Turn off unused variable warning
target_compile_options(${LIB_NAME} PRIVATE
	-Wno-unused-parameter)

# Set RPATH to $ORIGIN
set_target_properties(${LIB_NAME} PROPERTIES
	BUILD_RPATH_USE_ORIGIN TRUE
	INSTALL_RPATH $ORIGIN
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED ON
)

include(GNUInstallDirs)
install(TARGETS ${LIB_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

#
# Dependencies
#
find_package(Eigen3 REQUIRED)
target_link_libraries(${LIB_NAME} PRIVATE Eigen3::Eigen)

find_package(OpenMP REQUIRED)
target_link_libraries(${LIB_NAME} PRIVATE OpenMP::OpenMP_CXX)

find_package(Boost REQUIRED COMPONENTS system filesystem)
target_link_libraries(${LIB_NAME} PRIVATE Boost::system Boost::filesystem)

