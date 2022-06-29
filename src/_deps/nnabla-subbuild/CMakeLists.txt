# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.16.3)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(nnabla-populate NONE)

include(ExternalProject)
ExternalProject_Add(nnabla-populate
                     "UPDATE_DISCONNECTED" "False" "URL" "https://github.com/sony/nnabla/archive/v1.19.0.tar.gz" "URL_HASH" "SHA256=75e9b1934c66d564abdeba19f84d1c1ae46afa0b5e5db00fb8b392a46bb0f0f9" "PATCH_COMMAND" "/usr/bin/patch" "-p1" "<" "/home/at02/デスクトップ/ai-server-ssl-h/3rd/nnabla/nnabla-v1.19.0.patch"
                    SOURCE_DIR          "/home/at02/デスクトップ/ai-server-ssl-h/src/_deps/nnabla-src"
                    BINARY_DIR          "/home/at02/デスクトップ/ai-server-ssl-h/src/_deps/nnabla-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)
