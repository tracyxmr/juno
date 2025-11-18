# Set the minimum required version of CMake.
cmake_minimum_required(VERSION 3.21)

# Get the project name from the current source directory.
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Set the project name & supported language.
project(${PROJECT_NAME} LANGUAGES CXX)

# Set the C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler options for MSVC.
if (MSVC)
    add_compile_options(/MP) # /MP for multi-processor compilation
endif()

# Enable folder structure in IDEs.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Gather all header files in include/
file(GLOB_RECURSE PROJ_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

# Gather all source files in source/
file(GLOB_RECURSE PROJ_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cxx"
)

# Update the output variable to the recent git commit hash
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Make sure the output directory exists
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)

# Now configure it in the constants file
configure_file(
        ${CMAKE_SOURCE_DIR}/include/constants.hpp
        ${CMAKE_BINARY_DIR}/include/constants.hpp
        @ONLY
)

# Add executable target (Change to add_library if building DLL).
add_executable(${PROJECT_NAME} ${PROJ_SOURCES} ${PROJ_HEADERS}
        include/lexer/token.hpp
        include/lexer/lexer.hpp
        source/lexer/lexer.cpp
        include/parser/ast.hpp
        include/parser/parser.hpp
        source/parser/parser.cpp
        include/evaluator/evaluator.hpp
        source/evaluator/evaluator.cpp
)

# Add the include directories for the project.
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set output directories for binaries and libraries.
set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/exe"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/lib"
)