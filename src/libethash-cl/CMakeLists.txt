cmake_minimum_required(VERSION 2.8)

set(LIBRARY ethash-cl)
set(CMAKE_BUILD_TYPE Release)

include(bin2h.cmake)
bin2h(SOURCE_FILE ethash_cl_miner_kernel.cl VARIABLE_NAME ethash_cl_miner_kernel HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h)

set(OpenCL_FOUND TRUE)
set(OpenCL_INCLUDE_DIRS /usr/include/CL)
set(OpenCL_LIBRARIES -lOpenCL)

if (NOT OpenCL_FOUND)
	find_package(OpenCL)
endif()

if (OpenCL_FOUND)
	set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wno-unknown-pragmas -Wextra -Werror -pedantic -fPIC ${CMAKE_CXX_FLAGS}")
	include_directories(${OpenCL_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
	include_directories(..)
	add_library(${LIBRARY} ethash_cl_miner.cpp ethash_cl_miner.h)
	TARGET_LINK_LIBRARIES(${LIBRARY} ${OpenCL_LIBRARIES} ethash)
endif()
