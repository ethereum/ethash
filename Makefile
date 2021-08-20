# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.20

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.20.2/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.20.2/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/nickh/code/Go/src/github.com/LuxorLabs/ethash

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/nickh/code/Go/src/github.com/LuxorLabs/ethash

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/opt/homebrew/Cellar/cmake/3.20.2/bin/cmake --regenerate-during-build -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake cache editor..."
	/opt/homebrew/Cellar/cmake/3.20.2/bin/ccmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/nickh/code/Go/src/github.com/LuxorLabs/ethash/CMakeFiles /Users/nickh/code/Go/src/github.com/LuxorLabs/ethash//CMakeFiles/progress.marks
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/nickh/code/Go/src/github.com/LuxorLabs/ethash/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named ethash

# Build rule for target.
ethash: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 ethash
.PHONY : ethash

# fast build rule for target.
ethash/fast:
	$(MAKE) $(MAKESILENT) -f src/libethash/CMakeFiles/ethash.dir/build.make src/libethash/CMakeFiles/ethash.dir/build
.PHONY : ethash/fast

#=============================================================================
# Target rules for targets named Benchmark_CL

# Build rule for target.
Benchmark_CL: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 Benchmark_CL
.PHONY : Benchmark_CL

# fast build rule for target.
Benchmark_CL/fast:
	$(MAKE) $(MAKESILENT) -f src/benchmark/CMakeFiles/Benchmark_CL.dir/build.make src/benchmark/CMakeFiles/Benchmark_CL.dir/build
.PHONY : Benchmark_CL/fast

#=============================================================================
# Target rules for targets named Benchmark_LIGHT

# Build rule for target.
Benchmark_LIGHT: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 Benchmark_LIGHT
.PHONY : Benchmark_LIGHT

# fast build rule for target.
Benchmark_LIGHT/fast:
	$(MAKE) $(MAKESILENT) -f src/benchmark/CMakeFiles/Benchmark_LIGHT.dir/build.make src/benchmark/CMakeFiles/Benchmark_LIGHT.dir/build
.PHONY : Benchmark_LIGHT/fast

#=============================================================================
# Target rules for targets named Benchmark_FULL

# Build rule for target.
Benchmark_FULL: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 Benchmark_FULL
.PHONY : Benchmark_FULL

# fast build rule for target.
Benchmark_FULL/fast:
	$(MAKE) $(MAKESILENT) -f src/benchmark/CMakeFiles/Benchmark_FULL.dir/build.make src/benchmark/CMakeFiles/Benchmark_FULL.dir/build
.PHONY : Benchmark_FULL/fast

#=============================================================================
# Target rules for targets named Test

# Build rule for target.
Test: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 Test
.PHONY : Test

# fast build rule for target.
Test/fast:
	$(MAKE) $(MAKESILENT) -f test/c/CMakeFiles/Test.dir/build.make test/c/CMakeFiles/Test.dir/build
.PHONY : Test/fast

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... Benchmark_CL"
	@echo "... Benchmark_FULL"
	@echo "... Benchmark_LIGHT"
	@echo "... Test"
	@echo "... ethash"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

