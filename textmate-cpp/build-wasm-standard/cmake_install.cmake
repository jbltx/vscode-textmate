# Install script for directory: /Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/mickaelbonfill/dev/emsdk/upstream/emscripten/cache/sysroot")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/textmate-standard.js")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/textmate-standard.js" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/textmate-standard.js")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/textmate-standard.js")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vscode-textmate" TYPE FILE FILES
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/types.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/utils.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/onigLib.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/rawGrammar.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/parseRawGrammar.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/theme.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/theme_c_api.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/encodedTokenAttributes.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/rule.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/matcher.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/grammarDependencies.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/basicScopesAttributeProvider.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/tokenizeString.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/grammar.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/registry.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/main.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/c_api.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/session.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/session_c_api.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/syntax_highlighter.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/src/syntax_highlighter_c_api.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
