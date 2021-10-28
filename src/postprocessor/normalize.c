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
 * Image normalization routines.
 */

/*************************************************************************************************/

#include "normalize.h"

#include "../../include/lrpt.h"
#include "../liblrpt/error.h"
#include "../liblrpt/image.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*************************************************************************************************/

/* Needed for avoiding no signal areas (black bands) */
static const uint8_t HIST_MIN_BLACK = 2;

/* Cutoff percentiles for normalization */
static const uint8_t HIST_CUTOFF_BLACK = 1;
static const uint8_t HIST_CUTOFF_WHITE = 1;

/*************************************************************************************************/

/* lrpt_postproc_image_normalize() */
bool lrpt_postproc_image_normalize(
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

    size_t hist[256]; /* Intensity histogram */

    for (uint8_t i = 0; i < 6; i++) {
        /* Zero out histogram */
        memset(hist, 0, sizeof(size_t) * 256);

        /* Count up intensities */
        for (size_t j = 0; j < (image->width * image->height); j++)
            hist[image->channels[i][j]]++;

        /* Determine black/white cut-off counts */
        size_t black_cutoff = (image->width * image->height * HIST_CUTOFF_BLACK) / 100;
        size_t white_cutoff = (image->width * image->height * HIST_CUTOFF_WHITE) / 100;

        /* Find black cutoff intensity */
        size_t cnt = 0;
        uint8_t black_cutval = 0;

        for (black_cutval = HIST_MIN_BLACK; black_cutval < 255; black_cutval++) {
            cnt += hist[black_cutval];

            if (cnt >= black_cutoff)
                break;
        }

        /* Find white cutoff intensity */
        cnt = 0;
        uint8_t white_cutval = 0;

        for (white_cutval = 255; white_cutval > 0; white_cutval--) {
            cnt += hist[white_cutval];

            if (cnt >= white_cutoff)
                break;
        }

        /* Rescale pixels in image for required intensity range. We should be careful because
         * line below will give integer underflow.
         */
        uint8_t rng_in = (white_cutval - black_cutval);

        if (rng_in == 0)
            continue;

        /* Perform histogram normalization on image */
        for (size_t j = 0; j < (image->width * image->height); j++) {
            /* Input image pixel values are relative to input black cut off. Clamp pixel values
             * within black and white cut off values.
             */
            uint8_t val = image->channels[i][j];

            if (val > white_cutval)
                val = white_cutval;
            else if (val < black_cutval)
                val = black_cutval;

            val -= black_cutval;

            /* Normalized pixel values are scaled according to the ratio of required pixel value
             * range to input pixel value range */
            image->channels[i][j] = (0 + (val * 255) / rng_in);
        }
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
