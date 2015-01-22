include_directories(..)

INCLUDE(CMakeForceCompiler)

# Bootstrap emscripten
if ("${EMSCRIPTEN_ROOT_PATH}" STREQUAL "")
  execute_process(COMMAND em-config EMSCRIPTEN_ROOT OUTPUT_VARIABLE EMSCRIPTEN_ROOT_PATH)
  string(STRIP "${EMSCRIPTEN_ROOT_PATH}" EMSCRIPTEN_ROOT_PATH)
endif()

# Abort if not found.
if ("${EMSCRIPTEN_ROOT_PATH}" STREQUAL "")
  message(WARNING "Could not locate the Emscripten compiler toolchain directory! Either set the EMSCRIPTEN environment variable, or pass -DEMSCRIPTEN_ROOT_PATH=xxx to CMake to explicitly specify the location of the compiler!")
else()
  message("EMSCRIPTEN_ROOT_PATH: ${EMSCRIPTEN_ROOT_PATH}")

# Check version
set(EMSCRIPTEN_REQUIRED_VERSION 1.21.4)
if(EXISTS ${EMSCRIPTEN_ROOT_PATH}/emscripten-version.txt)
  file(READ ${EMSCRIPTEN_ROOT_PATH}/emscripten-version.txt EMSCRIPTEN_VERSION)
  string(STRIP ${EMSCRIPTEN_VERSION} EMSCRIPTEN_VERSION)
  message(STATUS "Found Emscripten ${EMSCRIPTEN_VERSION}")
  if(EMSCRIPTEN_VERSION VERSION_LESS ${EMSCRIPTEN_REQUIRED_VERSION})
    message(FATAL_ERROR "Emscripten >= ${EMSCRIPTEN_REQUIRED_VERSION} is required.")
  endif()
endif()

set( EMSCRIPTEN                 ON )
set( PLATFORM_EMBEDDED          ON )
set( PLATFORM_NAME              "Emscripten" )
set( PLATFORM_TRIPLET           "emscripten" )
set( PLATFORM_PREFIX            "$ENV{EMSCRIPTEN}" )
set( PLATFORM_PORTS_PREFIX      "${CMAKE_SOURCE_DIR}/ports/Emscripten" )
set( PLATFORM_EXE_SUFFIX        ".js" )
set( CMAKE_SYSTEM_NAME          "Generic" CACHE STRING "Target system." )
set( CMAKE_SYSTEM_PROCESSOR     "JavaScript" CACHE STRING "Target processor." )
set( CMAKE_AR                   "${EMSCRIPTEN_ROOT_PATH}/emar")
set( CMAKE_RANLIB               "${EMSCRIPTEN_ROOT_PATH}/emranlib")
set( CMAKE_C_COMPILER           "${EMSCRIPTEN_ROOT_PATH}/emcc")
set( CMAKE_CXX_COMPILER         "${EMSCRIPTEN_ROOT_PATH}/em++")
set( CMAKE_C_ARCHIVE_CREATE     "${CMAKE_C_COMPILER} -o <TARGET> <LINK_FLAGS> <OBJECTS>" )
set( CMAKE_CXX_ARCHIVE_CREATE   "${CMAKE_CXX_COMPILER} -o <TARGET>  <LINK_FLAGS> <OBJECTS>" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -O3 -s ASM_JS=1")

CMAKE_FORCE_C_COMPILER(         ${CMAKE_C_COMPILER} Clang )
CMAKE_FORCE_CXX_COMPILER(       ${CMAKE_CXX_COMPILER} Clang )

# TODO: Not very clever
set(FILES 	../libethash/util.c
          	../libethash/util.h
          	../libethash/ethash.c
          	../libethash/ethash.h
          	../libethash/blum_blum_shub.c
          	../libethash/blum_blum_shub.h
          	../libethash/endian.h
          	../libethash/compiler.h
          	../libethash/fnv.h
          	../libethash/sha3.c
          	../libethash/sha3.h)

add_executable(benchmark.js EXCLUDE_FROM_ALL benchmark.c ${FILES})
SET_TARGET_PROPERTIES(benchmark.js PROPERTIES LINK_FLAGS "-s TOTAL_MEMORY=50331648 --bind")
add_custom_command(TARGET benchmark.js POST_BUILD COMMAND node benchmark.js)

endif()
