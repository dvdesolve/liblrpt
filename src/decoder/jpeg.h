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

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Public internal API for JPEG decoder routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_JPEG_H
#define LRPT_DECODER_JPEG_H

/*************************************************************************************************/

#include <stddef.h>

/*************************************************************************************************/

/** JPEG decoder object */
typedef struct lrpt_decoder_jpeg__ {
    /* TODO add flag for checking in source code (-1) */
    size_t last_mcu; /**< Last MCU number */

    /* TODO recheck, both were assigned to -1 */
    /** @{ */
    /** Need for tracking row number in transmitted image */
    size_t cur_y, last_y;
    /** @} */

    /* TODO recheck, may be type should be different */
    /** @{ */
    /** Packet indices */
    int first_pck, prev_pck;
    /** @} */
} lrpt_decoder_jpeg_t;

/*************************************************************************************************/

/** Allocate and initialize JPEG decoder object.
 *
 * \return Pointer to the allocated JPEG decoder object or \c NULL in case of error.
 */
lrpt_decoder_jpeg_t *lrpt_decoder_jpeg_init(void);

/** Free previously allocated JPEG decoder.
 *
 * \param jpeg Pointer to the JPEG decoder object.
 */
void lrpt_decoder_jpeg_deinit(lrpt_decoder_jpeg_t *jpeg);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */