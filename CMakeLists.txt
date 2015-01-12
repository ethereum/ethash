cmake_minimum_required(VERSION 2.8.12)
project(daggerhashimoto)

add_subdirectory (libdaggerhashimoto)
add_subdirectory (test EXCLUDE_FROM_ALL)
add_subdirectory (benchmark EXCLUDE_FROM_ALL)