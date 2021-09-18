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
 * Basic image manipulation routines.
 *
 * This source file contains common routines for image manipulation.
 */

/*************************************************************************************************/

#include "image.h"

#include "../../include/lrpt.h"
#include "error.h"

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* bt709_gamma_encode() */
static inline uint8_t bt709_gamma_encode(
        uint8_t val) {
    if ((val / 255.0) < 0.018)
        return (4.5 * val);
    else
        return (255 * (1.099 * pow(val / 255.0, 0.45) - 0.099));
}

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
                    "LRPT image allocation failed");

        return NULL;
    }

    /* NULL-init all channel images before doing actual allocation */
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
                    "LRPT image buffer allocation failed");

        return NULL;
    }

    image->width = width;
    image->height = height;

    return image;
}

/*************************************************************************************************/

/* lrpt_image_free() */
void lrpt_image_free(
        lrpt_image_t *image) {
    if (!image)
        return;

    for (uint8_t i = 0; i < 6; i++)
        free(image->channels[i]);

    free(image);
}

/*************************************************************************************************/

/* lrpt_image_width() */
size_t lrpt_image_width(
        const lrpt_image_t *image) {
    if (!image)
        return 0;

    return image->width;
}

/*************************************************************************************************/

/* lrpt_image_height() */
size_t lrpt_image_height(
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
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "LRPT image object is NULL or corrupted");

        return false;
    }

    /* For the same widthes just return true */
    if (image->width == new_width)
        return true;

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
                        "LRPT image buffer reallocation failed");

            return false;
        }
        else {
            for (uint8_t i = 0; i < 6; i++) {
                /* Zero out newly allocated portions */
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

    return true;
}

/*************************************************************************************************/

/* lrpt_image_set_height() */
bool lrpt_image_set_height(
        lrpt_image_t *image,
        size_t new_height,
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

    /* For the same heights just return true */
    if (image->height == new_height)
        return true;

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
                        "LRPT image buffer reallocation failed");

            return false;
        }
        else {
            for (uint8_t i = 0; i < 6; i++) {
                /* Zero out newly allocated portions */
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

    return true;

}

/*************************************************************************************************/

/* lrpt_image_get_px() */
uint8_t lrpt_image_get_px(
        const lrpt_image_t *image,
        uint8_t apid,
        size_t pos) {
    if (!image || (pos > (image->height * image->width)) || (apid < 64) || (apid > 69))
        return 0;

    return image->channels[apid - 64][pos];
}

/*************************************************************************************************/

/* lrpt_image_set_px() */
void lrpt_image_set_px(
        lrpt_image_t *image,
        uint8_t apid,
        size_t pos,
        uint8_t val) {
    if (!image || (pos > (image->height * image->width)) || (apid < 64) || (apid > 69))
        return;

    image->channels[apid - 64][pos] = val;
}

/*************************************************************************************************/

/* lrpt_image_dump_pgm() */
bool lrpt_image_dump_pgm(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid,
        bool corr,
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

    if (!fname || (strlen(fname) == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "File name is NULL or empty");

        return false;
    }

    if ((apid < 64) || (apid > 69)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Requested APID number is incorrect");

        return false;
    }

    /* Perform gamma correction (if requested) */
    uint8_t *res = image->channels[apid - 64];
    uint8_t *corrected = NULL;

    if (corr) {
        corrected = calloc(image->width * image->height, sizeof(uint8_t));

        if (!corrected) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Can't allocate temporary buffer for gamma correction");

            return false;
        }

        for (size_t j = 0; j < image->height; j++) {
            for (size_t i = 0; i < image->width; i++)
                corrected[i + j * image->width] =
                    bt709_gamma_encode(lrpt_image_get_px(image, apid, i + j * image->width));
        }

        res = corrected;
    }

    FILE *fh = fopen(fname, "wb");

    if (!fh) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open PGM file for writing");

        return false;
    }

    /* Write PGM identifier */
    fprintf(fh, "P5\n");

    /* Write creator comment */
    /* TODO write liblrpt version too */
    fprintf(fh, "# Created with liblrpt\n");

    /* We're limiting our images to be 65535 * 65535 size at max */
    uint16_t w, h;

    w = (image->width > 65535) ? 65535 : image->width;
    h = (image->height > 65535) ? 65535 : image->height;

    /* Write width and height */
    fprintf(fh, "%" PRIu16 "\n", w);
    fprintf(fh, "%" PRIu16 "\n", h);

    /* Max value is 255 */
    fprintf(fh, "%d\n", 255);

    /* Write image itself */
    if (fwrite(res, sizeof(uint8_t), w * h, fh) != (w * h)) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "PGM file data write error");

        return false;
    }

    fclose(fh);

    free(corrected);

    return true;
}

/*************************************************************************************************/

/* lrpt_image_dump_ppm() */
bool lrpt_image_dump_ppm(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid_red,
        uint8_t apid_green,
        uint8_t apid_blue,
        bool corr,
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

    if (!fname || (strlen(fname) == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "File name is NULL or empty");

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

    /* Final buffer is just a RGB combination of requested APIDs */
    uint8_t *res = calloc(3 * image->width * image->height, sizeof(uint8_t));

    if (!res) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Can't allocate temporary buffer for storing RGB data");

        return false;
    }

    /* Perform gamma correction (if requested) */
    uint8_t *corrected_r = NULL;
    uint8_t *corrected_g = NULL;
    uint8_t *corrected_b = NULL;

    if (corr) {
        corrected_r = calloc(image->width * image->height, sizeof(uint8_t));
        corrected_g = calloc(image->width * image->height, sizeof(uint8_t));
        corrected_b = calloc(image->width * image->height, sizeof(uint8_t));

        if (!corrected_r || !corrected_g || !corrected_b) {
            free(corrected_r);
            free(corrected_g);
            free(corrected_b);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Can't allocate temporary buffer for gamma correction");

            return false;
        }

        for (size_t j = 0; j < image->height; j++) {
            for (size_t i = 0; i < image->width; i++) {
                corrected_r[i + j * image->width] =
                    bt709_gamma_encode(lrpt_image_get_px(image, apid_red, i + j * image->width));
                corrected_g[i + j * image->width] =
                    bt709_gamma_encode(lrpt_image_get_px(image, apid_green, j * image->width));
                corrected_b[i + j * image->width] =
                    bt709_gamma_encode(lrpt_image_get_px(image, apid_blue, i + j * image->width));
            }
        }
    }

    /* Fill resulting buffer with interleaved RGB data */
    uint8_t *res_r = (corr) ? corrected_r : image->channels[apid_red - 64];
    uint8_t *res_g = (corr) ? corrected_g : image->channels[apid_green - 64];
    uint8_t *res_b = (corr) ? corrected_b : image->channels[apid_blue - 64];

    for (size_t j = 0; j < image->height; j++) {
        for (size_t i = 0; i < image->width; i++) {
            res[3 * (i + j * image->width) + 0] = res_r[i + j * image->width];
            res[3 * (i + j * image->width) + 1] = res_g[i + j * image->width];
            res[3 * (i + j * image->width) + 2] = res_b[i + j * image->width];
        }
    }

    FILE *fh = fopen(fname, "wb");

    if (!fh) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open PPM file for writing");

        return false;
    }

    /* Write PPM identifier */
    fprintf(fh, "P6\n");

    /* Write creator comment */
    /* TODO write liblrpt version too */
    fprintf(fh, "# Created with liblrpt\n");

    /* We're limiting our images to be 65535 * 65535 size at max */
    uint16_t w, h;

    w = (image->width > 65535) ? 65535 : image->width;
    h = (image->height > 65535) ? 65535 : image->height;

    /* Write width and height */
    fprintf(fh, "%" PRIu16 "\n", w);
    fprintf(fh, "%" PRIu16 "\n", h);

    /* Max value is 255 */
    fprintf(fh, "%d\n", 255);

    /* Write image itself */
    if (fwrite(res, 3 * sizeof(uint8_t), w * h, fh) != (w * h)) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "PPM file data write error");

        return false;
    }

    fclose(fh);

    free(corrected_r);
    free(corrected_g);
    free(corrected_b);

    free(res);

    return true;
}

/*************************************************************************************************/

/** \endcond */
