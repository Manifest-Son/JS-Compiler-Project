# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.30

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

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "C:\Program Files\JetBrains\CLion 2024.3.1.1\bin\cmake\win\x64\bin\cmake.exe"

# The command to remove a file.
RM = "C:\Program Files\JetBrains\CLion 2024.3.1.1\bin\cmake\win\x64\bin\cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\cmake-build-debug-event-trace

# Include any dependencies generated for this target.
include CMakeFiles/ast_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/ast_test.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/ast_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ast_test.dir/flags.make

CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.obj: CMakeFiles/ast_test.dir/flags.make
CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.obj: CMakeFiles/ast_test.dir/includes_CXX.rsp
CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.obj: C:/Users/YVONNE/Desktop/JS_Compiler_Final_Project/tests/ast/ast_test.cc
CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.obj: CMakeFiles/ast_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\cmake-build-debug-event-trace\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.obj"
	C:\PROGRA~1\JETBRA~1\CLION2~1.1\bin\mingw\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.obj -MF CMakeFiles\ast_test.dir\tests\ast\ast_test.cc.obj.d -o CMakeFiles\ast_test.dir\tests\ast\ast_test.cc.obj -c C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\tests\ast\ast_test.cc

CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.i"
	C:\PROGRA~1\JETBRA~1\CLION2~1.1\bin\mingw\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\tests\ast\ast_test.cc > CMakeFiles\ast_test.dir\tests\ast\ast_test.cc.i

CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.s"
	C:\PROGRA~1\JETBRA~1\CLION2~1.1\bin\mingw\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\tests\ast\ast_test.cc -o CMakeFiles\ast_test.dir\tests\ast\ast_test.cc.s

CMakeFiles/ast_test.dir/src/ast.cpp.obj: CMakeFiles/ast_test.dir/flags.make
CMakeFiles/ast_test.dir/src/ast.cpp.obj: CMakeFiles/ast_test.dir/includes_CXX.rsp
CMakeFiles/ast_test.dir/src/ast.cpp.obj: C:/Users/YVONNE/Desktop/JS_Compiler_Final_Project/src/ast.cpp
CMakeFiles/ast_test.dir/src/ast.cpp.obj: CMakeFiles/ast_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\cmake-build-debug-event-trace\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/ast_test.dir/src/ast.cpp.obj"
	C:\PROGRA~1\JETBRA~1\CLION2~1.1\bin\mingw\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/ast_test.dir/src/ast.cpp.obj -MF CMakeFiles\ast_test.dir\src\ast.cpp.obj.d -o CMakeFiles\ast_test.dir\src\ast.cpp.obj -c C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\src\ast.cpp

CMakeFiles/ast_test.dir/src/ast.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/ast_test.dir/src/ast.cpp.i"
	C:\PROGRA~1\JETBRA~1\CLION2~1.1\bin\mingw\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\src\ast.cpp > CMakeFiles\ast_test.dir\src\ast.cpp.i

CMakeFiles/ast_test.dir/src/ast.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/ast_test.dir/src/ast.cpp.s"
	C:\PROGRA~1\JETBRA~1\CLION2~1.1\bin\mingw\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\src\ast.cpp -o CMakeFiles\ast_test.dir\src\ast.cpp.s

# Object files for target ast_test
ast_test_OBJECTS = \
"CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.obj" \
"CMakeFiles/ast_test.dir/src/ast.cpp.obj"

# External object files for target ast_test
ast_test_EXTERNAL_OBJECTS =

ast_test.exe: CMakeFiles/ast_test.dir/tests/ast/ast_test.cc.obj
ast_test.exe: CMakeFiles/ast_test.dir/src/ast.cpp.obj
ast_test.exe: CMakeFiles/ast_test.dir/build.make
ast_test.exe: lib/libgtest.a
ast_test.exe: lib/libgtest_main.a
ast_test.exe: lib/libgtest.a
ast_test.exe: CMakeFiles/ast_test.dir/linkLibs.rsp
ast_test.exe: CMakeFiles/ast_test.dir/objects1.rsp
ast_test.exe: CMakeFiles/ast_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\cmake-build-debug-event-trace\CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable ast_test.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\ast_test.dir\link.txt --verbose=$(VERBOSE)
	C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -noprofile -executionpolicy Bypass -file C:/Users/YVONNE/.vcpkg-clion/vcpkg/scripts/buildsystems/msbuild/applocal.ps1 -targetBinary C:/Users/YVONNE/Desktop/JS_Compiler_Final_Project/cmake-build-debug-event-trace/ast_test.exe -installedDir C:/Users/YVONNE/.vcpkg-clion/vcpkg/installed/x64-windows/debug/bin -OutVariable out
	"C:\Program Files\JetBrains\CLion 2024.3.1.1\bin\cmake\win\x64\bin\cmake.exe" -D TEST_TARGET=ast_test -D TEST_EXECUTABLE=C:/Users/YVONNE/Desktop/JS_Compiler_Final_Project/cmake-build-debug-event-trace/ast_test.exe -D TEST_EXECUTOR= -D TEST_WORKING_DIR=C:/Users/YVONNE/Desktop/JS_Compiler_Final_Project/cmake-build-debug-event-trace -D TEST_EXTRA_ARGS= -D TEST_PROPERTIES= -D TEST_PREFIX= -D TEST_SUFFIX= -D TEST_FILTER= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=ast_test_TESTS -D CTEST_FILE=C:/Users/YVONNE/Desktop/JS_Compiler_Final_Project/cmake-build-debug-event-trace/ast_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=5 -D TEST_XML_OUTPUT_DIR= -P "C:/Program Files/JetBrains/CLion 2024.3.1.1/bin/cmake/win/x64/share/cmake-3.30/Modules/GoogleTestAddTests.cmake"

# Rule to build all files generated by this target.
CMakeFiles/ast_test.dir/build: ast_test.exe
.PHONY : CMakeFiles/ast_test.dir/build

CMakeFiles/ast_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\ast_test.dir\cmake_clean.cmake
.PHONY : CMakeFiles/ast_test.dir/clean

CMakeFiles/ast_test.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\cmake-build-debug-event-trace C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\cmake-build-debug-event-trace C:\Users\YVONNE\Desktop\JS_Compiler_Final_Project\cmake-build-debug-event-trace\CMakeFiles\ast_test.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/ast_test.dir/depend

