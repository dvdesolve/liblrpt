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

# sources
set(lrpt_SOURCES
    decoder/bitop.c
    decoder/correlator.c
    decoder/data.c
    decoder/decoder.c
    decoder/ecc.c
    decoder/jpeg.c
    decoder/huffman.c
    decoder/packet.c
    decoder/viterbi.c
    demodulator/agc.c
    demodulator/demodulator.c
    demodulator/pll.c
    demodulator/rrc.c
    liblrpt/datatype.c
    liblrpt/dsp.c
    liblrpt/error.c
    liblrpt/image.c
    liblrpt/io.c
    liblrpt/utils.c
)

set(lrpt_HEADERS
    ../include/lrpt.h
    decoder/bitop.h
    decoder/correlator.h
    decoder/data.h
    decoder/decoder.h
    decoder/ecc.h
    decoder/jpeg.h
    decoder/huffman.h
    decoder/packet.h
    decoder/viterbi.h
    demodulator/agc.h
    demodulator/demodulator.h
    demodulator/pll.h
    demodulator/rrc.h
    liblrpt/datatype.h
    liblrpt/dsp.h
    liblrpt/error.h
    liblrpt/image.h
    liblrpt/io.h
    liblrpt/utils.h
)


# primary and the only target
add_library(lrpt SHARED ${lrpt_SOURCES})


# compile-time definitions
target_compile_definitions(lrpt PRIVATE LIBLRPT_VERSION_FULL="${PROJECT_VERSION}")
target_compile_definitions(lrpt PRIVATE LIBLRPT_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}")
target_compile_definitions(lrpt PRIVATE LIBLRPT_VERSION_MINOR="${PROJECT_VERSION_MINOR}")
target_compile_definitions(lrpt PRIVATE LIBLRPT_VERSION_PATCH="${PROJECT_VERSION_PATCH}")


# extra compiler options
target_compile_options(lrpt PRIVATE -Wall -pedantic)
target_compile_options(lrpt PRIVATE -fvisibility=hidden)
target_compile_options(lrpt PRIVATE -fstack-protector-strong)


# link libraries
target_link_libraries(lrpt PRIVATE m)


# target settings
set_target_properties(lrpt PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(lrpt PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(lrpt PROPERTIES PUBLIC_HEADER "../include/lrpt.h")
set_target_properties(lrpt PROPERTIES DEFINE_SYMBOL "LRPT_DLL_EXPORTS")


# install library
install(TARGETS lrpt LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
