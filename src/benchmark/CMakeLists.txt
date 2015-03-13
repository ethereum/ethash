include_directories(..)

set(CMAKE_BUILD_TYPE Release)

#if (MSVC)
#	add_definitions("/openmp")
#endif()
#
#if (NOT MPI_FOUND)
#    find_package(MPI)
#endif()
#
#if (NOT CRYPTOPP_FOUND)
#	find_package(CryptoPP 5.6.2)
#endif()
#
#if (CRYPTOPP_FOUND)
#	add_definitions(-DWITH_CRYPTOPP)
#endif()
#
#
#if (MPI_FOUND)
#    include_directories(${MPI_INCLUDE_PATH})
#    add_executable (Benchmark_MPI_FULL benchmark.cpp)
#    target_link_libraries (Benchmark_MPI_FULL ${ETHHASH_LIBS} ${MPI_LIBRARIES})
#    SET_TARGET_PROPERTIES(Benchmark_MPI_FULL PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} -DFULL -DMPI")
#
#    add_executable (Benchmark_MPI_LIGHT benchmark.cpp)
#    target_link_libraries (Benchmark_MPI_LIGHT ${ETHHASH_LIBS} ${MPI_LIBRARIES})
#    SET_TARGET_PROPERTIES(Benchmark_MPI_LIGHT PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} -DMPI")
#endif()
#
#add_executable (Benchmark_FULL benchmark.cpp)
#target_link_libraries (Benchmark_FULL ${ETHHASH_LIBS})
#SET_TARGET_PROPERTIES(Benchmark_FULL PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -DFULL")
#
#add_executable (Benchmark_LIGHT benchmark.cpp)
#target_link_libraries (Benchmark_LIGHT ${ETHHASH_LIBS})

if (NOT OpenCL_FOUND)
	find_package(OpenCL)
endif()

if (OpenCL_FOUND)
        if (NOT MSVC)
            # Initialize CXXFLAGS for c++11
            set(CMAKE_CXX_FLAGS                "-Wall -std=c++11")
            set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
            set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
            set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
            set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
        
            # Compiler-specific C++11 activation.
            if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
                execute_process(
                    COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
                if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
                    message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7 or greater.")
                endif ()
            elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
            else ()
                message(FATAL_ERROR "Your C++ compiler does not support C++11.")
            endif ()
        endif()

	add_definitions(-DWITH_OPENCL)
	include_directories(${OpenCL_INCLUDE_DIRS})
	list(APPEND FILES ethash_cl_miner.cpp ethash_cl_miner.h)
	add_executable (Benchmark_CL benchmark.cpp)
	target_link_libraries (Benchmark_CL ${ETHHASH_LIBS} ethash-cl)
	SET_TARGET_PROPERTIES(Benchmark_CL PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -DOPENCL")
endif()
