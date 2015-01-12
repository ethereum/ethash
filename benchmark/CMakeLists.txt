include_directories(..)
file(GLOB HEADERS "*.h")
add_executable (Benchmark benchmark.c ${HEADERS})
target_link_libraries (Benchmark daggerhashimoto)