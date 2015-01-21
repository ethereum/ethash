include_directories(..)

set(CMAKE_BUILD_TYPE Release)

if (MSVC)
	add_definitions("/openmp")
endif()

find_package(MPI)
if (MPI_FOUND)
    include_directories(${MPI_INCLUDE_PATH})
    add_executable (Benchmark_MPI_FULL benchmark.cpp)
    target_link_libraries (Benchmark_MPI_FULL ${ETHHASH_LIBS} ${MPI_LIBRARIES})
    SET_TARGET_PROPERTIES(Benchmark_MPI_FULL PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} -DFULL -DMPI")

    add_executable (Benchmark_MPI_CACHE benchmark.cpp)
    target_link_libraries (Benchmark_MPI_CACHE ${ETHHASH_LIBS} ${MPI_LIBRARIES})
    SET_TARGET_PROPERTIES(Benchmark_MPI_CACHE PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} -DMPI")
endif()

add_executable (Benchmark_FULL benchmark.cpp)
target_link_libraries (Benchmark_FULL ${ETHHASH_LIBS})
SET_TARGET_PROPERTIES(Benchmark_FULL PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -DFULL")

add_executable (Benchmark_CACHE benchmark.cpp)
target_link_libraries (Benchmark_CACHE ${ETHHASH_LIBS})