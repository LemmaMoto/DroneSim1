# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/build

# Include any dependencies generated for this target.
include CMakeFiles/watchdog.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/watchdog.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/watchdog.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/watchdog.dir/flags.make

CMakeFiles/watchdog.dir/watchdog.c.o: CMakeFiles/watchdog.dir/flags.make
CMakeFiles/watchdog.dir/watchdog.c.o: ../watchdog.c
CMakeFiles/watchdog.dir/watchdog.c.o: CMakeFiles/watchdog.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/watchdog.dir/watchdog.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/watchdog.dir/watchdog.c.o -MF CMakeFiles/watchdog.dir/watchdog.c.o.d -o CMakeFiles/watchdog.dir/watchdog.c.o -c /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/watchdog.c

CMakeFiles/watchdog.dir/watchdog.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/watchdog.dir/watchdog.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/watchdog.c > CMakeFiles/watchdog.dir/watchdog.c.i

CMakeFiles/watchdog.dir/watchdog.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/watchdog.dir/watchdog.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/watchdog.c -o CMakeFiles/watchdog.dir/watchdog.c.s

CMakeFiles/watchdog.dir/utility/wrapfunc.c.o: CMakeFiles/watchdog.dir/flags.make
CMakeFiles/watchdog.dir/utility/wrapfunc.c.o: ../utility/wrapfunc.c
CMakeFiles/watchdog.dir/utility/wrapfunc.c.o: CMakeFiles/watchdog.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/watchdog.dir/utility/wrapfunc.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/watchdog.dir/utility/wrapfunc.c.o -MF CMakeFiles/watchdog.dir/utility/wrapfunc.c.o.d -o CMakeFiles/watchdog.dir/utility/wrapfunc.c.o -c /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/utility/wrapfunc.c

CMakeFiles/watchdog.dir/utility/wrapfunc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/watchdog.dir/utility/wrapfunc.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/utility/wrapfunc.c > CMakeFiles/watchdog.dir/utility/wrapfunc.c.i

CMakeFiles/watchdog.dir/utility/wrapfunc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/watchdog.dir/utility/wrapfunc.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/utility/wrapfunc.c -o CMakeFiles/watchdog.dir/utility/wrapfunc.c.s

# Object files for target watchdog
watchdog_OBJECTS = \
"CMakeFiles/watchdog.dir/watchdog.c.o" \
"CMakeFiles/watchdog.dir/utility/wrapfunc.c.o"

# External object files for target watchdog
watchdog_EXTERNAL_OBJECTS =

../watchdog: CMakeFiles/watchdog.dir/watchdog.c.o
../watchdog: CMakeFiles/watchdog.dir/utility/wrapfunc.c.o
../watchdog: CMakeFiles/watchdog.dir/build.make
../watchdog: /usr/lib/x86_64-linux-gnu/libcurses.so
../watchdog: /usr/lib/x86_64-linux-gnu/libform.so
../watchdog: CMakeFiles/watchdog.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable ../watchdog"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/watchdog.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/watchdog.dir/build: ../watchdog
.PHONY : CMakeFiles/watchdog.dir/build

CMakeFiles/watchdog.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/watchdog.dir/cmake_clean.cmake
.PHONY : CMakeFiles/watchdog.dir/clean

CMakeFiles/watchdog.dir/depend:
	cd /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10 /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10 /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/build /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/build /home/ema/Documents/Unige/ARP/DroneSim1/WatchdogWithoutPipes17_10/build/CMakeFiles/watchdog.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/watchdog.dir/depend

