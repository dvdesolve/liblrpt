/*
 * This file is part of liblrpt.
 *
 * liblrpt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liblrpt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with liblrpt. If not, see https://www.gnu.org/licenses/
 *
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Public internal API for I/O routines.
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_IO_H
#define LRPT_LIBLRPT_IO_H

/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*************************************************************************************************/

/** I/Q samples data file type */
struct lrpt_iq_file__ {
    FILE *fhandle; /**< File object handle */

    bool write_mode; /**< Flag for write mode */

    uint8_t version; /**< File format version */
    unsigned char flags; /**< I/Q data flags */
    uint32_t samplerate; /**< Sampling rate */
    uint32_t bandwidth; /**< Bandwidth of the signal */
    char *device_name; /**< Device name info */

    uint64_t header_len; /**< Length of header data */
    uint64_t data_len; /**< Number of I/Q samples in file */
    uint64_t current; /**< Current I/Q sample in file stream */

    unsigned char *iobuf; /**< Temporary buffer for reading/writing */
};

/** QPSK symbols data file type */
struct lrpt_qpsk_file__ {
    FILE *fhandle; /**< File object handle */

    bool write_mode; /**< Flag for write mode */

    uint8_t version; /**< File format version */
    unsigned char flags; /**< Demod flags (mode, diffcoding, interleaving, symbol type etc) */
    uint32_t symrate; /**< Symbol rate */

    uint64_t header_len; /**< Length of header data */
    uint64_t data_len; /**< Number of QPSK bytes in file */
    uint64_t current; /**< Current QPSK sample in file stream */

    unsigned char *iobuf; /**< Temporary buffer for reading/writing */
    unsigned char last_hardsym; /**< Last hard symbol that was written */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
