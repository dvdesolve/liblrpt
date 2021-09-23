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
 * Public internal API for basic data types and memory management.
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_DATATYPE_H
#define LRPT_LIBLRPT_DATATYPE_H

/*************************************************************************************************/

#include <complex.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Storage type for I/Q data */
struct lrpt_iq_data__ {
    complex double *iq; /**< Array of I/Q samples */
    size_t len; /**< Total number of I/Q samples */
};

/** Ring buffer type for I/Q data */
struct lrpt_iq_rb__ {
    complex double *iq; /**< Array of I/Q samples */
    size_t len; /**< Total number of I/Q samples + 1, for determining full/empty states */
    size_t head; /**< Pointer to the data head */
    size_t tail; /**< Pointer to the data tail */
};

/** Storage type for QPSK data */
struct lrpt_qpsk_data__ {
    int8_t *qpsk; /**< Array of QPSK bytes */
    size_t len; /**< Total number of QPSK bytes */
};

/** Ring buffer type for QPSK data */
struct lrpt_qpsk_rb__ {
    int8_t *qpsk; /**< Array of QPSK bytes */
    size_t len; /**< Total number of QPSK samples + 1, for determining full/empty states */
    size_t head; /**< Pointer to the data head */
    size_t tail; /**< Pointer to the data tail */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
