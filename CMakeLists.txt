cmake_minimum_required(VERSION 3.13)

project(apt-transport-tftp VERSION 1.0.0 LANGUAGES C CXX)

add_executable(${PROJECT_NAME} tftp.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE apt-pkg)
install(PROGRAMS "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION /usr/lib/apt/methods RENAME tftp)

set(CPACK_GENERATOR "DEB")
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "atftp")
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
