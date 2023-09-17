# This file is part of liblrpt.
#
# liblrpt is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# liblrpt is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with liblrpt. If not, see https://www.gnu.org/licenses/
#
# Author: Viktor Drobot

# global settings
cmake_minimum_required(VERSION 3.19)
project(liblrpt VERSION 0.1.0 LANGUAGES C DESCRIPTION "Demodulate, decode and post-process LRPT signals")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# use FHS
include(GNUInstallDirs)

# default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# prevent agressive and unsafe optimizations
# (by default CMake uses -O3 level)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# enable position-independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# check for necessary types
include(CheckTypeSize)

check_type_size("int8_t" T__INT8_T LANGUAGE C)
check_type_size("uint8_t" T__UINT8_T LANGUAGE C)
check_type_size("int16_t" T__INT16_T LANGUAGE C)
check_type_size("uint16_t" T__UINT16_T LANGUAGE C)
check_type_size("int32_t" T__INT32_T LANGUAGE C)
check_type_size("uint32_t" T__UINT32_T LANGUAGE C)
check_type_size("int64_t" T__INT64_T LANGUAGE C)
check_type_size("uint64_t" T__UINT64_T LANGUAGE C)

if (
        (NOT ${T__INT8_T} EQUAL 1) OR (NOT ${T__UINT8_T} EQUAL 1) OR
        (NOT ${T__INT16_T} EQUAL 2) OR (NOT ${T__UINT16_T} EQUAL 2) OR
        (NOT ${T__INT32_T} EQUAL 4) OR (NOT ${T__UINT32_T} EQUAL 4) OR
        (NOT ${T__INT64_T} EQUAL 8) OR (NOT ${T__UINT64_T} EQUAL 8)
        )
    message(FATAL_ERROR "One of the standard integer types doesn't have proper size.")
endif ()


# build project
add_subdirectory(src)


# prepare and install pkg-config stuff
configure_file(liblrpt.pc.in liblrpt.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/liblrpt.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)


# optionally build documentation
option(BUILD_DOC "Build API documentation" ON)
option(BUILD_DEVDOC "Build developer's documentation" OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Release" AND BUILD_DOC)
    find_package(Doxygen)

    if (DOXYGEN_FOUND)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc)

        if (BUILD_DEVDOC)
            set(EXTRA_SECTIONS "INTERNAL_API_DOCS")
        endif ()

        configure_file(doxygen.config.in doxygen.config @ONLY)

        add_custom_target(doc ALL
            COMMAND Doxygen::doxygen doxygen.config
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generate API documentation with Doxygen"
            VERBATIM)

        # actually install documentation
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html/ DESTINATION ${CMAKE_INSTALL_DOCDIR})
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen needs to be installed to generate documentation")
    endif (DOXYGEN_FOUND)
endif ()


# unit testing
enable_testing()
add_subdirectory(tests)
