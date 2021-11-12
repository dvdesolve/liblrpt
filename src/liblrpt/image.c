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
 * Basic image manipulation routines.
 */

/*************************************************************************************************/

#include "image.h"

#include "../../include/lrpt.h"
#include "error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* lrpt_image_alloc() */
lrpt_image_t *lrpt_image_alloc(
        size_t width,
        size_t height,
        lrpt_error_t *err) {
    lrpt_image_t *image = malloc(sizeof(lrpt_image_t));

    if (!image) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "LRPT image object allocation has failed");

        return NULL;
    }

    /* NULL-init all channel images before doing actual allocation for safe allocation later */
    for (uint8_t i = 0; i < 6; i++)
        image->channels[i] = NULL;

    bool ok = true;

    if ((width * height) > 0) {
        for (uint8_t i = 0; i < 6; i++) {
            image->channels[i] = calloc(width * height, sizeof(uint8_t));

            if (!image->channels[i]) {
                ok = false;

                break;
            }
        }
    }

    if (!ok) {
        lrpt_image_free(image);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Pixel buffer allocation for LRPT image object has failed");

        return NULL;
    }

    image->width = width;
    image->height = height;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return image;
}

/*************************************************************************************************/

/* lrpt_image_free() */
inline void lrpt_image_free(
        lrpt_image_t *image) {
    if (!image)
        return;

    for (uint8_t i = 0; i < 6; i++)
        free(image->channels[i]);

    free(image);
}

/*************************************************************************************************/

/* lrpt_image_width() */
inline size_t lrpt_image_width(
        const lrpt_image_t *image) {
    if (!image)
        return 0;

    return image->width;
}

/*************************************************************************************************/

/* lrpt_image_height() */
inline size_t lrpt_image_height(
        const lrpt_image_t *image) {
    if (!image)
        return 0;

    return image->height;
}

/*************************************************************************************************/

/* lrpt_image_set_width() */
bool lrpt_image_set_width(
        lrpt_image_t *image,
        size_t new_width,
        lrpt_error_t *err) {
    /* Check only image already have a width */
    bool good = true;

    if (image && (image->width > 0)) {
        for (uint8_t i = 0; i < 6; i++)
            if (!image->channels[i]) {
                good = false;

                break;
            }
    }

    if (!image || !good) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "LRPT image object is NULL or corrupted");

        return false;
    }

    /* If widthes are the same don't do anything */
    if (image->width == new_width) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_PARAM,
                    "New width of LRPT image object equals to the old one");

        return true;
    }

    /* In case of zero width create empty LRPT image object */
    if (new_width == 0) {
        for (uint8_t i = 0; i < 6; i++) {
            free(image->channels[i]);
            image->channels[i] = NULL;
        }

        image->width = 0;
    }
    else {
        uint8_t *new_bufs[6];

        for (uint8_t i = 0; i < 6; i++) {
            new_bufs[i] =
                reallocarray(image->channels[i], image->height * new_width, sizeof(uint8_t));

            if (!new_bufs[i]) {
                good = false;

                break;
            }
        }

        if (!good) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Pixel buffer reallocation for LRPT image object has failed");

            return false;
        }
        else {
            for (uint8_t i = 0; i < 6; i++) {
                /* Zero out newly allocated parts of pixel buffers */
                if (new_width > image->width)
                    memset(
                            new_bufs[i] + image->height * image->width,
                            0,
                            sizeof(uint8_t) * (image->height * (new_width - image->width)));

                image->channels[i] = new_bufs[i];
            }

            image->width = new_width;
        }
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_image_set_height() */
bool lrpt_image_set_height(
        lrpt_image_t *image,
        size_t new_height,
        lrpt_error_t *err) {
    /* Check only image already have a height */
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
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "LRPT image object is NULL or corrupted");

        return false;
    }

    /* If heights are the same don't do anything */
    if (image->height == new_height) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_PARAM,
                    "New height of LRPT image object equals to the old one");

        return true;
    }

    /* In case of zero height create empty LRPT image object */
    if (new_height == 0) {
        for (uint8_t i = 0; i < 6; i++) {
            free(image->channels[i]);
            image->channels[i] = NULL;
        }

        image->height = 0;
    }
    else {
        uint8_t *new_bufs[6];

        for (uint8_t i = 0; i < 6; i++) {
            new_bufs[i] =
                reallocarray(image->channels[i], image->width * new_height, sizeof(uint8_t));

            if (!new_bufs[i]) {
                good = false;

                break;
            }
        }

        if (!good) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Pixel buffer reallocation for LRPT image object has failed");

            return false;
        }
        else {
            for (uint8_t i = 0; i < 6; i++) {
                /* Zero out newly allocated parts of pixel buffers */
                if (new_height > image->height)
                    memset(
                            new_bufs[i] + image->height * image->width,
                            0,
                            sizeof(uint8_t) * (image->width * (new_height - image->height)));

                image->channels[i] = new_bufs[i];
            }

            image->height = new_height;
        }
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_image_get_px() */
inline uint8_t lrpt_image_get_px(
        const lrpt_image_t *image,
        uint8_t apid,
        size_t pos) {
    if (!image || (pos > (image->height * image->width)) ||
            (apid < 64) || (apid > 69) || !image->channels[apid - 64])
        return 0;

    return image->channels[apid - 64][pos];
}

/*************************************************************************************************/

/* lrpt_image_set_px() */
inline void lrpt_image_set_px(
        lrpt_image_t *image,
        uint8_t apid,
        size_t pos,
        uint8_t val) {
    if (!image || (pos > (image->height * image->width)) ||
            (apid < 64) || (apid > 69) || !image->channels[apid - 64])
        return;

    image->channels[apid - 64][pos] = val;
}

/*************************************************************************************************/

/** \endcond */
