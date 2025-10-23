cmake_minimum_required(VERSION 3.5)

# Example program for vscode-textmate C API
add_executable(textmate_example textmate_example.c)

# Link with onig library
target_link_libraries(textmate_example onig)

# Include directories
target_include_directories(textmate_example PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_BINARY_DIR}/..
)

# Go grammar test program
add_executable(textmate_go_test textmate_go_test.c)
target_link_libraries(textmate_go_test onig)
target_include_directories(textmate_go_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_BINARY_DIR}/..
)

# All tests program - comprehensive test suite
add_executable(all_tests all_tests.c)
target_link_libraries(all_tests onig)
target_include_directories(all_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_BINARY_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../cJSON
)
