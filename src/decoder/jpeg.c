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
 * JPEG decoder routines.
 *
 * This source file contains routines for performing JPEG decoding.
 */

/*************************************************************************************************/

#include "jpeg.h"

#include <stddef.h>
#include <stdlib.h>

/*************************************************************************************************/

/* lrpt_decoder_jpeg_init() */
lrpt_decoder_jpeg_t *lrpt_decoder_jpeg_init(void) {
    /* Allocate JPEG decoder object */
    lrpt_decoder_jpeg_t *jpeg = malloc(sizeof(lrpt_decoder_jpeg_t));

    if (!jpeg)
        return NULL;

    /* Set internal state variables */
    jpeg->last_mcu = 0;
    jpeg->cur_y = 0;
    jpeg->last_y = 0;
    jpeg->first_pck = 0;
    jpeg->prev_pck = 0;

    return jpeg;
}

/*************************************************************************************************/

/* lrpt_decoder_jpeg_deinit() */
void lrpt_decoder_jpeg_deinit(lrpt_decoder_jpeg_t *jpeg) {
    if (!jpeg)
        return;

    free(jpeg);
}

/*************************************************************************************************/

/** \endcond */
