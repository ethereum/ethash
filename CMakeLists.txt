cmake_minimum_required(VERSION 3.1.2)
project(ethash)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(ETHHASH_LIBS ethash)

if (WIN32 AND WANT_CRYPTOPP)
	add_subdirectory(cryptopp)
endif()

add_subdirectory(libethash)
add_subdirectory(libethash-cl EXCLUDE_FROM_ALL)
add_subdirectory(benchmark EXCLUDE_FROM_ALL)
add_subdirectory(test EXCLUDE_FROM_ALL)
