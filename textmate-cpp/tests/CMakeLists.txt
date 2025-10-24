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
