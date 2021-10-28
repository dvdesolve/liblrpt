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
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Geometric image manipulation routines.
 */

/*************************************************************************************************/

#include "geom.h"

#include "../../include/lrpt.h"
#include "../liblrpt/error.h"
#include "../liblrpt/image.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/* lrpt_postproc_image_flip() */
bool lrpt_image_flip(
        lrpt_image_t *image,
        lrpt_error_t *err) {
    bool good = true;

    if (image && (image->height > 0)) {
        for (uint8_t i = 0; i < 6; i++)
            if (!image->channels[i]) {
                good = false;

                break;
            }
    }

    if (!image || !good) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "LRPT image object is NULL or corrupted");

        return false;
    }

    if ((image->width * image->height) == 0)
        return true;

    const size_t iw = image->width;
    const size_t ih = image->height;

    for (uint8_t i = 0; i < 6; i++) {
        for (size_t y = 0; y < (ih / 2 + ih % 2); y++) {
            const size_t border = (y == (ih / 2)) ? (iw / 2) : iw;

            for (size_t x = 0; x < border; x++) {
                const uint8_t tmp = image->channels[i][y * iw + x];

                image->channels[i][y * iw + x] = image->channels[i][(ih - 1 - y) * iw + (iw - 1 - x)];
                image->channels[i][(ih - 1 - y) * iw + (iw - 1 - x)] = tmp;
            }
        }
    }

    return true;
}
/*************************************************************************************************/

/** \endcond */
