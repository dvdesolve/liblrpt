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
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_LRPT_H
#define LRPT_LIBLRPT_LRPT_H

/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/* Storage type for raw I/Q data */
struct lrpt_iq_data__ {
    /* I and Q samples storage */
    double *i;
    double *q;

    /* Total number of I/Q samples pairs */
    size_t len;
};

/* Storage type for QPSK soft symbols data */
struct lrpt_qpsk_data__ {
    /* QPSK soft symbols storage */
    int8_t *s;

    /* Total number of QPSK soft symbols */
    size_t len;
};

/*************************************************************************************************/

#endif
