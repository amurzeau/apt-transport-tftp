# apt-transport-tftp implementation to support source.list pointing
# on a TFTP server.
#
# Copyright 2024 Alexis Murzeau <amubtdx@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.13)

project(apt-transport-tftp VERSION 1.0.0 LANGUAGES C CXX)

add_executable(${PROJECT_NAME} tftp.cpp libtftp.cpp libtftp.h)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
target_link_libraries(${PROJECT_NAME} PRIVATE apt-pkg)
install(PROGRAMS "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION /usr/lib/apt/methods RENAME tftp)


add_executable(test_tftp test.cpp libtftp.cpp libtftp.h)
target_compile_options(test_tftp PRIVATE -Wall)

set(CPACK_GENERATOR "DEB")
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alexis Murzeau <amubtdx@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "APT transport for downloading via the TFTP protocol
This package contains the APT TFTP transport. It makes it possible to
use 'deb tftp://localhost:69/foo distro main' type lines in your
sources.list file.")
set(CPACK_DEBIAN_PACKAGE_SECTION "admin")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/amurzeau/apt-transport-tftp")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
include(CPack)
