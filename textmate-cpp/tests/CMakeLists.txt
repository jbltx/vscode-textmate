cmake_minimum_required(VERSION 3.10)

# Test executable
add_executable(test_grammar
    test_grammar.cpp
)

# Link against the library
target_link_libraries(test_grammar
    vscode-textmate-cpp
)

# Add test
add_test(NAME GrammarTest COMMAND test_grammar)

# Set output directory
set_target_properties(test_grammar PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# First-Mate test suite executable
add_executable(test_first_mate
    test_first_mate.cpp
)

# Link against the library and RapidJSON
target_link_libraries(test_first_mate
    vscode-textmate-cpp
)

# Include RapidJSON from ThirdParty folder
set(RAPIDJSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../ThirdParty/rapidjson/include")
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
