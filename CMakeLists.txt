cmake_minimum_required(VERSION 2.8.12)
project(ethash)

set(ETHHASH_LIBS ethash)

option(WANT_CRYPTOPP "Use CryptoPP at ../cryptopp for SHA3" OFF)
if (WANT_CRYPTOPP)
	add_subdirectory(cryptopp)
	include_directories(..)
	add_definitions("-DWANT_CRYPTOPP")
	set(ETHHASH_LIBS ${ETHHASH_LIBS} cryptopp)
endif()

add_subdirectory(libethash)
add_subdirectory(benchmark)
#add_subdirectory(test)
