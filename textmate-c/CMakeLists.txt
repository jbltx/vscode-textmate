cmake_minimum_required(VERSION 3.10)
project(vscode-textmate-c VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall")

# Option to build shared library
option(BUILD_SHARED_LIBS "Build shared library instead of static" ON)

# Setup paths for ThirdParty dependencies
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/../ThirdParty")

# Setup cJSON
set(CJSON_INCLUDE_DIR "${THIRDPARTY_DIR}/cJSON")
set(CJSON_SOURCES "${THIRDPARTY_DIR}/cJSON/cJSON.c")

if(NOT EXISTS ${CJSON_SOURCES})
    message(FATAL_ERROR "cJSON not found at ${CJSON_SOURCES}. Please run: git submodule update --init --recursive")
endif()

# Setup oniguruma - check if it needs to be built
set(ONIG_INCLUDE_DIR "${THIRDPARTY_DIR}/oniguruma/src")
set(ONIG_SOURCE_DIR "${THIRDPARTY_DIR}/oniguruma")

if(NOT EXISTS "${ONIG_INCLUDE_DIR}/oniguruma.h")
    message(FATAL_ERROR "Oniguruma not found at ${ONIG_INCLUDE_DIR}. Please run: git submodule update --init --recursive")
endif()

# Try to find system oniguruma library first, otherwise we'll build it
find_library(ONIG_LIBRARY NAMES onig)

if(NOT ONIG_LIBRARY)
    message(STATUS "System oniguruma not found, will need to build from source")
    message(STATUS "Please build oniguruma first:")
    message(STATUS "  cd ${ONIG_SOURCE_DIR}")
    message(STATUS "  ./autogen.sh && ./configure && make && sudo make install")
    message(FATAL_ERROR "Oniguruma library not found")
endif()

message(STATUS "Found oniguruma library: ${ONIG_LIBRARY}")

# Source files for the library
set(TEXTMATE_SOURCES
    src/vscode_textmate.c
    src/vscode_textmate_parser.c
)

# Add cJSON source if using bundled version
if(CJSON_SOURCES)
    list(APPEND TEXTMATE_SOURCES ${CJSON_SOURCES})
endif()

# Create the library
if(BUILD_SHARED_LIBS)
    add_library(vscode-textmate SHARED ${TEXTMATE_SOURCES})
    message(STATUS "Building shared library")
else()
    add_library(vscode-textmate STATIC ${TEXTMATE_SOURCES})
    message(STATUS "Building static library")
endif()

# Set library properties
set_target_properties(vscode-textmate PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "src/vscode_textmate.h"
)

# Include directories for the library
target_include_directories(vscode-textmate
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${ONIG_INCLUDE_DIR}
        ${CJSON_INCLUDE_DIR}
)

# Link libraries (cJSON is compiled in, so only link oniguruma)
target_link_libraries(vscode-textmate PRIVATE ${ONIG_LIBRARY})

# Export library for other CMake projects
export(TARGETS vscode-textmate FILE vscode-textmate-config.cmake)

# Build examples subdirectory
add_subdirectory(example)

# Installation rules
include(GNUInstallDirs)

install(TARGETS vscode-textmate
    EXPORT vscode-textmate-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install internal header (optional, for development)
install(FILES src/vscode_textmate_internal.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Print summary
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  vscode-textmate-c Configuration")
message(STATUS "========================================")
message(STATUS "  Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library:  ${BUILD_SHARED_LIBS}")
message(STATUS "  C compiler:      ${CMAKE_C_COMPILER}")
message(STATUS "  Oniguruma lib:   ${ONIG_LIBRARY}")
message(STATUS "  Oniguruma inc:   ${ONIG_INCLUDE_DIR}")
message(STATUS "  cJSON inc:       ${CJSON_INCLUDE_DIR}")
message(STATUS "  cJSON (bundled): ${CJSON_SOURCES}")
message(STATUS "========================================")
message(STATUS "")
