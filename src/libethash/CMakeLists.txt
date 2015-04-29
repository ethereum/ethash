set(LIBRARY ethash)

if (CPPETHEREUM)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif ()

set(CMAKE_BUILD_TYPE Release)

if (NOT MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(FILES 	util.h
			keccak.h keccak.cpp
          	io.c
          	internal.c
          	ethash.h
          	endian.h
          	compiler.h
          	fnv.h
          	data_sizes.h)

if (MSVC)
	list(APPEND FILES util.c io_win32.c mmap_win32.c)
else()
	list(APPEND FILES io_posix.c)
endif()

add_library(${LIBRARY} ${FILES})

if (CRYPTOPP_FOUND)
	TARGET_LINK_LIBRARIES(${LIBRARY} ${CRYPTOPP_LIBRARIES})
endif()
