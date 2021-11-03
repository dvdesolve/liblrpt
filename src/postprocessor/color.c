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
 * Color enhancement routines.
 */

/*************************************************************************************************/

#include "color.h"

#include "../../include/lrpt.h"
#include "../liblrpt/error.h"
#include "../liblrpt/image.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/* lrpt_postproc_image_rescale_range() */
bool lrpt_postproc_image_rescale_range(
        lrpt_image_t *image,
        uint8_t apid,
        uint8_t pxval_min,
        uint8_t pxval_max,
        lrpt_error_t *err) {
    /* TODO check for min < max */
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

    if ((apid < 64) || (apid > 69)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Requested APID number is incorrect");

        return false;
    }

    for (size_t i = 0; i < (image->width * image->height); i++)
        image->channels[apid - 64][i] =
            (pxval_min + (image->channels[apid - 64][i] * (pxval_max - pxval_min)) / 255);

    return true;
}

/*************************************************************************************************/

/* lrpt_postproc_image_fix_water() */
bool lrpt_postproc_image_fix_water(
        lrpt_image_t *image,
        uint8_t apid_blue,
        uint8_t pxval_min,
        uint8_t pxval_max,
        lrpt_error_t *err) {
    /* TODO check for min < max */
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

    if ((apid_blue < 64) || (apid_blue > 69)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Requested APID number is incorrect");

        return false;
    }

    for (size_t i = 0; i < (image->width * image->height); i++)
        if (image->channels[apid_blue - 64][i] < pxval_min)
            image->channels[apid_blue - 64][i] =
                (pxval_min +
                 (image->channels[apid_blue - 64][i] * (pxval_max - pxval_min)) / pxval_min);

    return true;
}

/*************************************************************************************************/

/* lrpt_postproc_image_fix_clouds() */
bool lrpt_postproc_image_fix_clouds(
        lrpt_image_t *image,
        uint8_t apid_red,
        uint8_t apid_green,
        uint8_t apid_blue,
        uint8_t red_min,
        uint8_t red_max,
        uint8_t threshold,
        lrpt_error_t *err) {
    /* TODO check for min < max */
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

    if (
            (apid_red < 64) ||
            (apid_red > 69) ||
            (apid_green < 64) ||
            (apid_green > 69) ||
            (apid_blue < 64) ||
            (apid_blue > 69)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Requested APID number is incorrect");

        return false;
    }

    for (size_t i = 0; i < (image->width * image->height); i++)
        if (image->channels[apid_blue - 64][i] > threshold) {
            image->channels[apid_red - 64][i] = image->channels[apid_blue - 64][i];
            image->channels[apid_green - 64][i] = image->channels[apid_blue - 64][i];
        }
        else
            image->channels[apid_red - 64][i] =
                (red_min + (image->channels[apid_red - 64][i] * (red_max - red_min)) / 255);

    return true;
}

/*************************************************************************************************/

/* lrpt_postproc_image_invert_channel() */
bool lrpt_postproc_image_invert_channel(
        lrpt_image_t *image,
        uint8_t apid,
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

    if ((apid < 64) || (apid > 69)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Requested APID number is incorrect");

        return false;
    }

    for (size_t i = 0; i < (image->width * image->height); i++)
        image->channels[apid - 64][i] = (255 - image->channels[apid - 64][i]);

    return true;
}

/*************************************************************************************************/

/** \endcond */
