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

find_package(Check REQUIRED)


include_directories(../include)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})


add_executable(check_iq_data datatype/iq_data.c)
add_executable(check_qpsk_data datatype/qpsk_data.c)

target_link_libraries(check_iq_data PRIVATE lrpt ${CHECK_LIBRARIES})
target_link_libraries(check_qpsk_data PRIVATE lrpt ${CHECK_LIBRARIES})


cmake_policy(SET CMP0110 NEW)
add_test(NAME "I/Q data" COMMAND check_iq_data)
add_test(NAME "QPSK data" COMMAND check_qpsk_data)
