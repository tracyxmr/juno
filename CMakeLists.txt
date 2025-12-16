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

# Add executable target (Change to add_library if building DLL).
add_executable(${PROJECT_NAME} ${PROJ_SOURCES} ${PROJ_HEADERS})

# Add the include directories for the project.
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set output directories for binaries and libraries.
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/exe"    
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/lib"    
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/lib"    
)
