include_directories(..)

if (MSVC)
	add_definitions("/openmp")
endif()

set(CMAKE_BUILD_TYPE Release)
add_executable (Benchmark_FULL benchmark.cpp)
target_link_libraries (Benchmark_FULL ${ETHHASH_LIBS})
SET_TARGET_PROPERTIES(Benchmark_FULL PROPERTIES COMPILE_FLAGS "-DFULL")

add_executable (Benchmark_CACHE benchmark.cpp)
target_link_libraries (Benchmark_CACHE ${ETHHASH_LIBS})