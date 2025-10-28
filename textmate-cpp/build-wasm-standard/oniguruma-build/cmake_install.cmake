# Install script for directory: /Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-wasm")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/libonig.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/src/oniguruma.h"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/src/oniggnu.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/oniguruma" TYPE FILE FILES
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/generated/onigurumaConfig.cmake"
    "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/generated/onigurumaConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/oniguruma/onigurumaTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/oniguruma/onigurumaTargets.cmake"
         "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/CMakeFiles/Export/8c73ef290f98ec84adf6bb2530e3b311/onigurumaTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/oniguruma/onigurumaTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/oniguruma/onigurumaTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/oniguruma" TYPE FILE FILES "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/CMakeFiles/Export/8c73ef290f98ec84adf6bb2530e3b311/onigurumaTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/oniguruma" TYPE FILE FILES "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/CMakeFiles/Export/8c73ef290f98ec84adf6bb2530e3b311/onigurumaTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/onig" TYPE FILE FILES
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/API"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/API.ja"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/RE"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/RE.ja"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/FAQ"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/FAQ.ja"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/CALLOUTS.BUILTIN"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/CALLOUTS.BUILTIN.ja"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/CALLOUTS.API"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/CALLOUTS.API.ja"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/doc/UNICODE_PROPERTIES"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/onig" TYPE FILE FILES
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/AUTHORS"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/COPYING"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/HISTORY"
    "/Users/mickaelbonfill/dev/vscode-textmate/ThirdParty/oniguruma/README.md"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/oniguruma.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/onig-config")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/test/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp/build-wasm-standard/oniguruma-build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
