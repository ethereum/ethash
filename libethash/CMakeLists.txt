set(LIBRARY ethash)
set(CMAKE_BUILD_TYPE Release)

set(FILES 	util.c
          	util.h
          	ethash.c
          	ethash.h
          	blum_blum_shub.c
          	blum_blum_shub.h
          	bitfn.h
          	compiler.h
          	fnv.h)

find_package (CryptoPP)

if (CRYPTOPP_FOUND)
	add_definitions(-DWITH_CRYPTOPP)
	include_directories( ${CRYPTOPP_INCLUDE_DIRS} )
	list(APPEND FILES sha3_cryptopp.cpp sha3_cryptopp.h)
else()
	list(APPEND FILES sha3.c sha3.h)
endif()

add_library(${LIBRARY} ${FILES})

if (CRYPTOPP_FOUND)
	TARGET_LINK_LIBRARIES(${LIBRARY} ${CRYPTOPP_LIBRARIES})
endif()