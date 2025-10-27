cmake_minimum_required(VERSION 3.10)
project(vscode-textmate-cpp VERSION 1.0.0 LANGUAGES CXX)

# Set C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -ffast-math")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/oniguruma/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/rapidjson/include
)

# Source files
set(TEXTMATE_SOURCES
    src/main.cpp
    src/utils.cpp
    src/onigLib.cpp
    src/rawGrammar.cpp
    src/parseRawGrammar.cpp
    src/theme.cpp
    src/encodedTokenAttributes.cpp
    src/rule.cpp
    src/matcher.cpp
    src/grammarDependencies.cpp
    src/basicScopesAttributeProvider.cpp
    src/tokenizeString.cpp
    src/grammar.cpp
    src/registry.cpp
    src/c_api.cpp
)

# Header files (for IDE support)
set(TEXTMATE_HEADERS
    src/types.h
    src/utils.h
    src/onigLib.h
    src/rawGrammar.h
    src/parseRawGrammar.h
    src/theme.h
    src/theme_c_api.h
    src/encodedTokenAttributes.h
    src/rule.h
    src/matcher.h
    src/grammarDependencies.h
    src/basicScopesAttributeProvider.h
    src/tokenizeString.h
    src/grammar.h
    src/registry.h
    src/main.h
    src/c_api.h
)

# Find Oniguruma library
find_library(ONIGURUMA_LIB
    NAMES onig
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/oniguruma/build
        ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/oniguruma/.libs
    NO_DEFAULT_PATH
)

if(NOT ONIGURUMA_LIB)
    message(STATUS "Oniguruma library not found, will need to build it first")
    # You can add custom commands to build oniguruma here if needed
else()
    message(STATUS "Found Oniguruma library: ${ONIGURUMA_LIB}")
endif()

# Create shared library
add_library(vscode-textmate-cpp SHARED
    ${TEXTMATE_SOURCES}
    ${TEXTMATE_HEADERS}
)

# Link libraries
if(ONIGURUMA_LIB)
    target_link_libraries(vscode-textmate-cpp ${ONIGURUMA_LIB})
else()
    target_link_libraries(vscode-textmate-cpp onig)
endif()

# Set output directory and define TEXTMATE_EXPORTS for Windows DLL export
set_target_properties(vscode-textmate-cpp PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)

# Define TEXTMATE_EXPORTS when building the library
target_compile_definitions(vscode-textmate-cpp PRIVATE TEXTMATE_EXPORTS)

# Install targets
install(TARGETS vscode-textmate-cpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${TEXTMATE_HEADERS}
    DESTINATION include/vscode-textmate
)

# Enable testing
enable_testing()

# Add test subdirectory if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()

# Print configuration
message(STATUS "vscode-textmate-cpp configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Source files: ${TEXTMATE_SOURCES}")
