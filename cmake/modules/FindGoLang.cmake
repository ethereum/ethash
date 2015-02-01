# This module provides facilities for building Go code using either golang
# (preferred) or gccgo.
#
# This module defines
#  GO_FOUND, if a compiler was found

# https://raw.githubusercontent.com/couchbase/tlm/master/cmake/Modules/FindCouchbaseGo.cmake

SET(GO_MINIMUM_VERSION 1.3)

# Prevent double-definition if two projects use this script
IF (NOT FindGoLang_INCLUDED)

  INCLUDE (ParseArguments)

  # Have to remember cwd when this find is INCLUDE()d
  SET (TLM_MODULES_DIR "${CMAKE_CURRENT_LIST_DIR}")

  # Figure out which Go compiler to use
  FIND_PROGRAM (GO_EXECUTABLE NAMES go DOC "Go executable")
  IF (GO_EXECUTABLE)
    EXECUTE_PROCESS (COMMAND ${GO_EXECUTABLE} version
                     OUTPUT_VARIABLE GO_VERSION_STRING)
    STRING (REGEX REPLACE "^go version go([0-9.]+).*$" "\\1" GO_VERSION ${GO_VERSION_STRING})
    MESSAGE (STATUS "Found Go compiler: ${GO_EXECUTABLE} (${GO_VERSION})")

    IF(GO_VERSION VERSION_LESS GO_MINIMUM_VERSION)
      MESSAGE(FATAL_ERROR "Go version of ${GO_MINIMUM_VERSION} or higher required (found version ${GO_VERSION})")
    ENDIF(GO_VERSION VERSION_LESS GO_MINIMUM_VERSION)

    SET (GO_COMMAND_LINE "${GO_EXECUTABLE}" build -x)
    SET (GO_FOUND 1 CACHE BOOL "Whether Go compiler was found")

    IF (DEFINED ENV{GOBIN})
      MESSAGE(WARNING "The environment variable GOBIN is set and MAY cause your build to fail")
    ENDIF (DEFINED ENV{GOBIN})
  ELSE (GO_EXECUTABLE)
    FIND_PROGRAM (GCCGO_EXECUTABLE NAMES gccgo DOC "gccgo executable")
    IF (GCCGO_EXECUTABLE)
      MESSAGE (STATUS "Found gccgo compiler: ${GCCGO_EXECUTABLE}")
      SET (GO_COMMAND_LINE "${GCCGO_EXECUTABLE}" -Os -g)
      SET (GO_FOUND 1 CACHE BOOL "Whether Go compiler was found")
    ELSE (GCCGO_EXECUTABLE)
      SET (GO_FOUND 0)
    ENDIF (GCCGO_EXECUTABLE)
  ENDIF (GO_EXECUTABLE)


  # Adds a target named TARGET which produces an output executable
  # named OUTPUT in the current binary directory, based on a set of
  # Go source files.
  #
  # This command is somewhat deprecated; when possible, use GoInstall(),
  # which interacts more cleanly with the Go compiler and has better
  # incremental build semantics.
  #
  # Required arguments:
  #
  # TARGET - name of CMake target to create
  #
  # OUTPUT - name of executable to create (.exe will be appended on Windows)
  #
  # SOURCES - A list of .go files. They will be passed as-is to the
  # go compiler, and also TARGET will depend on them (so it will be
  # re-invoked only if these .go files change). These files may be
  # relative paths as the go compiler will be invoked with the
  # working directory set to the current CMakeLists directory.
  #
  # Optional arguments:
  #
  # DEPENDS - list of other CMake targets on which TARGET will depend
  #
  # INSTALL_PATH - if specified, a CMake INSTALL() directive will be
  # created to install the output into the named path
  MACRO (GoBuild)
    IF (NOT GO_FOUND)
      MESSAGE (FATAL_ERROR "No go compiler was found!")
    ENDIF (NOT GO_FOUND)

    PARSE_ARGUMENTS (Go "DEPENDS;SOURCES" "TARGET;OUTPUT;INSTALL_PATH"
      "" ${ARGN})

    IF (NOT Go_TARGET)
      MESSAGE (FATAL_ERROR "TARGET is required!")
    ENDIF (NOT Go_TARGET)
    IF (NOT Go_OUTPUT)
      MESSAGE (FATAL_ERROR "OUTPUT is required!")
    ENDIF (NOT Go_OUTPUT)

    IF (WIN32)
      SET (Go_OUTPUT "${Go_OUTPUT}.exe")
    ENDIF (WIN32)
    SET (Go_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/${Go_OUTPUT}")
    ADD_CUSTOM_COMMAND (OUTPUT "${Go_OUTPUT_PATH}"
      COMMAND ${GO_COMMAND_LINE} -o ${Go_OUTPUT_PATH} ${Go_SOURCES}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      DEPENDS ${Go_SOURCES} VERBATIM)
    ADD_CUSTOM_TARGET ("${Go_TARGET}" ALL
      DEPENDS "${Go_OUTPUT_PATH}" SOURCES ${Go_SOURCES})
    SET_TARGET_PROPERTIES ("${Go_TARGET}" PROPERTIES
      RUNTIME_OUTPUT_NAME "${Go_OUTPUT}"
      RUNTIME_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

    IF (Go_DEPENDS)
      ADD_DEPENDENCIES (${Go_TARGET} ${Go_DEPENDS})
    ENDIF (Go_DEPENDS)

    IF (Go_INSTALL_PATH)
      INSTALL (PROGRAMS "${Go_OUTPUT_PATH}" DESTINATION "${Go_INSTALL_PATH}")
    ENDIF (Go_INSTALL_PATH)
  ENDMACRO (GoBuild)

  # Adds a target named TARGET which (always) calls "go install
  # PACKAGE".  This delegates incremental-build responsibilities to
  # the go compiler, which is generally what you want.
  #
  # Note that, unlike GoBuild(), this macro requires using the
  # Golang compiler, not gccgo. A CMake error will be raised if this
  # macro is used when only gccgo is detected.
  #
  # Required arguments:
  #
  # TARGET - name of CMake target to create
  #
  # PACKAGE - A single Go package to build. When this is specified,
  # the package and all dependencies on GOPATH will be built, using
  # the Go compiler's normal dependency-handling system.
  #
  # GOPATH - Every entry on this list will be placed onto the GOPATH
  # environment variable before invoking the compiler.
  #
  # Optional arguments:
  #
  # GCFLAGS - flags that will be passed (via -gcflags) to all compile
  # steps; should be a single string value, with spaces if necessary
  #
  # DEPENDS - list of other CMake targets on which TARGET will depend
  #
  # INSTALL_PATH - if specified, a CMake INSTALL() directive will be
  # created to install the output into the named path
  #
  # OUTPUT - name of the installed executable (only applicable if
  # INSTALL_PATH is specified). Default value is the basename of
  # PACKAGE, per the go compiler. On Windows, ".exe" will be
  # appended.
  #
  # CGO_INCLUDE_DIRS - path(s) to directories to search for C include files
  #
  # CGO_LIBRARY_DIRS - path(s) to libraries to search for C link libraries
  #
  MACRO (GoInstall)
    IF (NOT GO_EXECUTABLE)
      MESSAGE (FATAL_ERROR "Go compiler was not found!")
    ENDIF (NOT GO_EXECUTABLE)

    PARSE_ARGUMENTS (Go "DEPENDS;GOPATH;CGO_INCLUDE_DIRS;CGO_LIBRARY_DIRS"
      "TARGET;PACKAGE;OUTPUT;INSTALL_PATH;GCFLAGS" "" ${ARGN})

    IF (NOT Go_TARGET)
      MESSAGE (FATAL_ERROR "TARGET is required!")
    ENDIF (NOT Go_TARGET)
    IF (NOT Go_PACKAGE)
      MESSAGE (FATAL_ERROR "PACKAGE is required!")
    ENDIF (NOT Go_PACKAGE)

    # Hunt for the requested package on GOPATH (used for installing)
    SET (_found)
    FOREACH (_dir ${Go_GOPATH})
      FILE (TO_NATIVE_PATH "${_dir}/src/${Go_PACKAGE}" _pkgdir)
      IF (IS_DIRECTORY "${_pkgdir}")
        SET (_found 1)
        SET (_workspace "${_dir}")
        BREAK ()
      ENDIF (IS_DIRECTORY "${_pkgdir}")
    ENDFOREACH (_dir)
    IF (NOT _found)
      MESSAGE (FATAL_ERROR "Package ${Go_PACKAGE} not found in any workspace on GOPATH!")
    ENDIF (NOT _found)

    # Extract the binary name from the package, and tweak for Windows.
    GET_FILENAME_COMPONENT (_pkgexe "${Go_PACKAGE}" NAME)
    IF (WIN32)
      SET (_pkgexe "${_pkgexe}.exe")
    ENDIF (WIN32)
    IF (Go_OUTPUT)
      IF (WIN32)
        SET (Go_OUTPUT "${Go_OUTPUT}.exe")
      ENDIF (WIN32)
    ENDIF (Go_OUTPUT)

    # Go install target
    ADD_CUSTOM_TARGET ("${Go_TARGET}" ALL
      COMMAND "${CMAKE_COMMAND}"
      -D "GO_EXECUTABLE=${GO_EXECUTABLE}"
      -D "GOPATH=${Go_GOPATH}"
      -D "WORKSPACE=${_workspace}"
      -D "GCFLAGS=${Go_GCFLAGS}"
      -D "PKGEXE=${_pkgexe}"
      -D "PACKAGE=${Go_PACKAGE}"
      -D "OUTPUT=${Go_OUTPUT}"
      -D "CGO_INCLUDE_DIRS=${Go_CGO_INCLUDE_DIRS}"
      -D "CGO_LIBRARY_DIRS=${Go_CGO_LIBRARY_DIRS}"
      -P "${TLM_MODULES_DIR}/go-install.cmake"
      VERBATIM)
    IF (Go_DEPENDS)
      ADD_DEPENDENCIES (${Go_TARGET} ${Go_DEPENDS})
    ENDIF (Go_DEPENDS)

    # We expect multiple go targets to be operating over the same
    # GOPATH.  It seems like the go compiler doesn't like be invoked
    # in parallel in this case, as would happen if we parallelize the
    # Couchbase build (eg., 'make -j8'). Since the go compiler itself
    # does parallel building, we want to serialize all go targets. So,
    # we make them all depend on any earlier Go targets.
    GET_PROPERTY (_go_targets GLOBAL PROPERTY CB_GO_TARGETS)
    IF (_go_targets)
      ADD_DEPENDENCIES(${Go_TARGET} ${_go_targets})
    ENDIF (_go_targets)
    SET_PROPERTY (GLOBAL APPEND PROPERTY CB_GO_TARGETS ${Go_TARGET})

    # Tweaks for installing and output renaming. go-install.cmake will
    # arrange for the workspace's bin directory to contain a file with
    # the right name (either OUTPUT, or the Go package name if OUTPUT
    # is not specified). We need to know what that name is so we can
    # INSTALL() it.
    IF (Go_OUTPUT)
      SET (_finalexe "${Go_OUTPUT}")
    ELSE (Go_OUTPUT)
      SET (_finalexe "${_pkgexe}")
    ENDIF (Go_OUTPUT)
    IF (Go_INSTALL_PATH)
      INSTALL (PROGRAMS "${_workspace}/bin/${_finalexe}"
        DESTINATION "${Go_INSTALL_PATH}")
    ENDIF (Go_INSTALL_PATH)

  ENDMACRO (GoInstall)


  # Adds a target named TARGET which (always) calls "go tool yacc
  # PATH".
  #
  # Note that, unlike GoBuild(), this macro requires using the
  # Golang compiler, not gccgo. A CMake error will be raised if this
  # macro is used when only gccgo is detected.
  #
  # Required arguments:
  #
  # TARGET - name of CMake target to create
  #
  # YFILE - Absolute path to .y file.
  #
  # Optional arguments:
  #
  # DEPENDS - list of other CMake targets on which TARGET will depend
  #
  #
  MACRO (GoYacc)
    IF (NOT GO_EXECUTABLE)
      MESSAGE (FATAL_ERROR "Go compiler was not found!")
    ENDIF (NOT GO_EXECUTABLE)

    PARSE_ARGUMENTS (Go "DEPENDS" "TARGET;YFILE" "" ${ARGN})

    IF (NOT Go_TARGET)
      MESSAGE (FATAL_ERROR "TARGET is required!")
    ENDIF (NOT Go_TARGET)
    IF (NOT Go_YFILE)
      MESSAGE (FATAL_ERROR "YFILE is required!")
    ENDIF (NOT Go_YFILE)

    GET_FILENAME_COMPONENT (_ypath "${Go_YFILE}" PATH)
    GET_FILENAME_COMPONENT (_yname "${Go_YFILE}" NAME)

    ADD_CUSTOM_TARGET ("${Go_TARGET}" ALL
      COMMAND "${CMAKE_COMMAND}" -E echo
      "-- Executing: ${GO_EXECUTABLE} tool yacc ${_yname}"
      COMMAND "${GO_EXECUTABLE}" tool yacc "${_yname}"
      WORKING_DIRECTORY "${_ypath}"
      VERBATIM)

    IF (Go_DEPENDS)
      ADD_DEPENDENCIES (${Go_TARGET} ${Go_DEPENDS})
    ENDIF (Go_DEPENDS)

  ENDMACRO (GoYacc)

  SET (FindGoLang_INCLUDED 1)

ENDIF (NOT FindGoLang_INCLUDED)