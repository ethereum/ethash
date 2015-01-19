cmake_minimum_required(VERSION 2.8.2)
project(ethash)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(ETHHASH_LIBS ethash)

if (NOT MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
endif()

add_subdirectory(libethash)
add_subdirectory(benchmark EXCLUDE_FROM_ALL)
add_subdirectory(test EXCLUDE_FROM_ALL)
