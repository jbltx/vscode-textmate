cmake_minimum_required(VERSION 3.10)

# Example program for vscode-textmate C API
add_executable(textmate_example textmate_example.c)

# Link with vscode-textmate library
target_link_libraries(textmate_example PRIVATE vscode-textmate ${ONIG_LIBRARY})

# Include directories
target_include_directories(textmate_example PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${ONIG_INCLUDE_DIR}
)

# Go grammar test program
add_executable(textmate_go_test textmate_go_test.c)
target_link_libraries(textmate_go_test PRIVATE vscode-textmate ${ONIG_LIBRARY})
target_include_directories(textmate_go_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${ONIG_INCLUDE_DIR}
)

# All tests program - comprehensive test suite
add_executable(all_tests all_tests.c)
target_link_libraries(all_tests PRIVATE vscode-textmate ${ONIG_LIBRARY})
target_include_directories(all_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${ONIG_INCLUDE_DIR}
  ${CJSON_INCLUDE_DIR}
)
