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

# global settings
cmake_minimum_required(VERSION 3.12)
project(liblrpt VERSION 0.1.0 LANGUAGES C DESCRIPTION "Demodulate, decode and post-process LRPT signals")


# use FHS
include(GNUInstallDirs)

# default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# prevent agressive and unsafe optimizations
# (by default CMake uses -O3 level)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")


# build project
add_subdirectory(src)
