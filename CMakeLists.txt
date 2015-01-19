cmake_minimum_required(VERSION 2.8.2)
project(ethash)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(ETHHASH_LIBS ethash)

option(WANT_CRYPTOPP "Use CryptoPP at ../cryptopp for SHA3" OFF)
if (WANT_CRYPTOPP)
	add_subdirectory(cryptopp)
	include_directories(..)
	add_definitions("-DWANT_CRYPTOPP")
	set(ETHHASH_LIBS ${ETHHASH_LIBS} cryptopp)
endif()

if (NOT MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
endif()

add_subdirectory(libethash)
add_subdirectory(benchmark EXCLUDE_FROM_ALL)
#add_subdirectory(test)
