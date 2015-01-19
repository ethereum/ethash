include_directories(..)

if (MSVC)
	add_definitions("/openmp")
endif()

set(CMAKE_BUILD_TYPE Release)
add_executable (Benchmark benchmark.cpp)
target_link_libraries (Benchmark ${ETHHASH_LIBS})