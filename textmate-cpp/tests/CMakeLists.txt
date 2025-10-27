cmake_minimum_required(VERSION 3.10)

# RapidJSON include directory
set(RAPIDJSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/include")

# Google Test include directory
set(GTEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/thirdparty/gtest/googletest/include")

# First-Mate test suite executable
add_executable(test_first_mate
    test_first_mate.cpp
)

# Link against the library
target_link_libraries(test_first_mate
    vscode-textmate-cpp
)

# Include RapidJSON
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_first_mate PRIVATE ${RAPIDJSON_INCLUDE_DIR})
    message(STATUS "Using RapidJSON from: ${RAPIDJSON_INCLUDE_DIR}")
else()
    message(WARNING "RapidJSON not found at ${RAPIDJSON_INCLUDE_DIR}. test_first_mate may not compile.")
endif()

# Add test
add_test(NAME FirstMateTests COMMAND test_first_mate)

# Set output directory
set_target_properties(test_first_mate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Benchmark executable for large files
add_executable(benchmark_large
    benchmark_large.cpp
)

# Link against the library
target_link_libraries(benchmark_large
    vscode-textmate-cpp
)

# Set output directory
set_target_properties(benchmark_large PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Google Test sources
set(GTEST_SOURCES
    "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/thirdparty/gtest/googletest/src/gtest-all.cc"
    "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/thirdparty/gtest/googletest/src/gtest_main.cc"
)

# Simple theme test executable
add_executable(test_theme_simple
    test_theme_simple.cpp
    ${GTEST_SOURCES}
)

# Link against the library
target_link_libraries(test_theme_simple
    vscode-textmate-cpp
)

# Include directories for test_theme_simple
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_theme_simple PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_theme_simple PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/thirdparty/gtest/googletest"
    )
endif()

# Add test
add_test(NAME ThemeSimpleTests COMMAND test_theme_simple)

# Set output directory
set_target_properties(test_theme_simple PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Theme test executable
add_executable(test_theme
    test_theme.cpp
    ${GTEST_SOURCES}
)

# Link against the library
target_link_libraries(test_theme
    vscode-textmate-cpp
)

# Include directories for test_theme
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_theme PRIVATE ${RAPIDJSON_INCLUDE_DIR})
else()
    message(WARNING "RapidJSON not found at ${RAPIDJSON_INCLUDE_DIR}. test_theme may not compile.")
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_theme PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/thirdparty/gtest/googletest"
    )
else()
    message(WARNING "Google Test not found at ${GTEST_INCLUDE_DIR}. test_theme may not compile.")
endif()

# Add test
add_test(NAME ThemeTests COMMAND test_theme)

# Set output directory
set_target_properties(test_theme PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Debug theme test executable
add_executable(test_theme_debug
    test_theme_debug.cpp
    ${GTEST_SOURCES}
)

# Link against the library
target_link_libraries(test_theme_debug
    vscode-textmate-cpp
)

# Include directories for test_theme_debug
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_theme_debug PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_theme_debug PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/thirdparty/gtest/googletest"
    )
endif()

# Add test
add_test(NAME ThemeDebugTests COMMAND test_theme_debug)

# Set output directory
set_target_properties(test_theme_debug PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# File theme test executable
add_executable(test_theme_file
    test_theme_file.cpp
    ${GTEST_SOURCES}
)

# Link against the library
target_link_libraries(test_theme_file
    vscode-textmate-cpp
)

# Include directories for test_theme_file
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_theme_file PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_theme_file PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/thirdparty/gtest/googletest"
    )
endif()

# Add test
add_test(NAME ThemeFileTests COMMAND test_theme_file)

# Set output directory
set_target_properties(test_theme_file PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
