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
 *
 * This source file contains common routines for image manipulation.
 */

/*************************************************************************************************/

#include "image.h"

#include "../../include/lrpt.h"
#include "error.h"
#include "utils.h"

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/** Perform gamma correction according to the BT.709 transfer function.
 *
 * \param val Linear pixel value.
 *
 * \return Corrected pixel value.
 */
static inline uint8_t bt709_gamma_encode(
        uint8_t val);

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

/* lrpt_image_dump_channel_pnm() */
bool lrpt_image_dump_channel_pnm(
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

    /* We're limiting our images to be 65535 * 65535 size at max */
    uint16_t w, h;

    w = (image->width > 65535) ? 65535 : image->width;
    h = (image->height > 65535) ? 65535 : image->height;

    /* Create final buffer */
    bool need_fill = (corr || (w != image->width) || (h != image->height));
    uint8_t *res = NULL;

    if (need_fill) {
        res = calloc(w * h, sizeof(uint8_t));

        if (!res) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Can't allocate temporary buffer for storing channel data");

            return false;
        }
    }
    else /* Just use existing data without unnecessary copying */
        res = image->channels[apid - 64];

    /* Fill resulting buffer and perform gamma correction (if requested) */
    if (need_fill)
        for (size_t j = 0; j < h; j++)
            for (size_t i = 0; i < w; i++) {
                uint8_t px = image->channels[apid - 64][i + j * image->width];

                res[i + j * w] = (corr) ? bt709_gamma_encode(px) : px;
            }


    FILE *fh = fopen(fname, "wb");

    if (!fh) {
        if (need_fill)
            free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open PNM file for writing");

        return false;
    }

    /* Write PNM identifier */
    fprintf(fh, "P5\n");

    /* Write creator comment */
    fprintf(fh, "# Created with liblrpt ver. %s\n", LIBLRPT_VERSION_FULL);

    /* Write width and height */
    fprintf(fh, "%" PRIu16 "\n", w);
    fprintf(fh, "%" PRIu16 "\n", h);

    /* Max value is 255 */
    fprintf(fh, "%d\n", 255);

    /* Write image itself */
    if (fwrite(res, sizeof(uint8_t), w * h, fh) != (w * h)) {
        fclose(fh);

        if (need_fill)
            free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "PNM file data write error");

        return false;
    }

    fclose(fh);

    if (need_fill)
        free(res);

    return true;
}

/*************************************************************************************************/

/* lrpt_image_dump_combo_pnm() */
bool lrpt_image_dump_combo_pnm(
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
                    "Requested APID number(s) is/are incorrect");

        return false;
    }

    /* We're limiting our images to be 65535 * 65535 size at max */
    uint16_t w, h;

    w = (image->width > 65535) ? 65535 : image->width;
    h = (image->height > 65535) ? 65535 : image->height;

    /* Final buffer is just a RGB combination of requested APIDs */
    uint8_t *res = calloc(3 * w * h, sizeof(uint8_t));

    if (!res) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Can't allocate temporary buffer for storing RGB data");

        return false;
    }

    /* Fill resulting buffer and perform gamma correction (if requested) */
    for (size_t j = 0; j < h; j++)
        for (size_t i = 0; i < w; i++) {
            uint8_t px_r = image->channels[apid_red - 64][i + j * image->width];
            uint8_t px_g = image->channels[apid_green - 64][i + j * image->width];
            uint8_t px_b = image->channels[apid_blue - 64][i + j * image->width];

            res[3 * (i + j * w) + 0] = (corr) ? bt709_gamma_encode(px_r) : px_r;
            res[3 * (i + j * w) + 1] = (corr) ? bt709_gamma_encode(px_g) : px_g;
            res[3 * (i + j * w) + 2] = (corr) ? bt709_gamma_encode(px_b) : px_b;
        }

    FILE *fh = fopen(fname, "wb");

    if (!fh) {
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open PNM file for writing");

        return false;
    }

    /* Write PNM identifier */
    fprintf(fh, "P6\n");

    /* Write creator comment */
    fprintf(fh, "# Created with liblrpt ver. %s\n", LIBLRPT_VERSION_FULL);

    /* Write width and height */
    fprintf(fh, "%" PRIu16 "\n", w);
    fprintf(fh, "%" PRIu16 "\n", h);

    /* Max value is 255 */
    fprintf(fh, "%d\n", 255);

    /* Write image itself */
    if (fwrite(res, 3 * sizeof(uint8_t), w * h, fh) != (w * h)) {
        fclose(fh);

        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "PNM file data write error");

        return false;
    }

    fclose(fh);
    free(res);

    return true;
}

/*************************************************************************************************/

/* lrpt_image_dump_channel_bmp() */
bool lrpt_image_dump_channel_bmp(
        const lrpt_image_t *image,
        const char *fname,
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

    /* We're limiting our images to be 65535 * 65535 size at max */
    uint16_t w, h;

    w = (image->width > 65535) ? 65535 : image->width;
    h = (image->height > 65535) ? 65535 : image->height;

    uint8_t pad = ((w % 4) == 0) ? 0 : (4 - w % 4); /* Padding for BMP scan lines */

    /* Final buffer is just a padded requested APID channel */
    uint8_t *res = calloc(h * (w + pad), sizeof(uint8_t));

    if (!res) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Can't allocate temporary buffer for storing channel data");

        return false;
    }

    /* Fill resulting buffer with padded channel data (fill direction is from bottom
     * to top)
     */
    for (size_t j = 0; j < h; j++)
        for (size_t i = 0; i < w; i++)
            res[i + (h - j - 1) * (w + pad)] = image->channels[apid - 64][i + j * image->width];

    FILE *fh = fopen(fname, "wb");

    if (!fh) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open BMP file for writing");

        return false;
    }

    uint32_t image_size = (h * (w + pad)); /* Padded image size */
    uint32_t file_size = (image_size + 54 + 256 * 4); /* Image size + header size */

    unsigned char b4_s[4];

    /* Write BMP identifier */
    fprintf(fh, "BM");

    /* Write BMP file size in bytes */
    lrpt_utils_s_uint32_t(file_size, b4_s, false);

    if (fwrite(b4_s, 1, 4, fh) != 4) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP file size write error");

        return false;
    }

    /* Write BMP Header constant data */
    unsigned char hdr_const_parts[12] = {
        0x00, 0x00, 0x00, 0x00, /* Reserved fields */
        0x36, 0x04, 0x00, 0x00, /* Data offset */
        0x28, 0x00, 0x00, 0x00 /* InfoHeader size */
    };

    if (fwrite(hdr_const_parts, 1, 12, fh) != 12) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP Header write error");

        return false;
    }

    /* Write width and height */
    lrpt_utils_s_uint32_t(w, b4_s, false); /* Width */

    if (fwrite(b4_s, 1, 4, fh) != 4) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP image width write error");

        return false;
    }

    lrpt_utils_s_uint32_t(h, b4_s, false); /* Height */

    if (fwrite(b4_s, 1, 4, fh) != 4) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP image height write error");

        return false;
    }

    /* Write BMP InfoHeader constant data, part 1 */
    unsigned char infohdr_const_parts_1[8] = {
        0x01, 0x00, /* Planes */
        0x08, 0x00, /* 8 bits per pixel */
        0x00, 0x00, 0x00, 0x00 /* Compression */
    };

    if (fwrite(infohdr_const_parts_1, 1, 8, fh) != 8) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP InfoHeader write error");

        return false;
    }

    /* Write BMP image size in bytes */
    lrpt_utils_s_uint32_t(image_size, b4_s, false);

    if (fwrite(b4_s, 1, 4, fh) != 4) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP image size write error");

        return false;
    }

    /* Write BMP InfoHeader constant data, part 2 */
    unsigned char infohdr_const_parts_2[16] = {
        0x23, 0x2E, 0x00, 0x00, /* 300 ppi horizontal resolution */
        0x23, 0x2E, 0x00, 0x00, /* 300 ppi vertical resolution */
        0x00, 0x01, 0x00, 0x00, /* Number of colors used (256) */
        0x00, 0x01, 0x00, 0x00 /* Number of important colors (256) */
    };

    if (fwrite(infohdr_const_parts_2, 1, 16, fh) != 16) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP InfoHeader write error");

        return false;
    }

    /* Write color table */
    for (uint16_t i = 0; i <= 255; i++) {
        unsigned char col_idx[4];

        col_idx[0] = i;
        col_idx[1] = i;
        col_idx[2] = i;
        col_idx[3] = 0;

        if (fwrite(col_idx, 1, 4, fh) != 4) {
            fclose(fh);
            free(res);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                        "BMP ColorTable write error");

            return false;
        }
    }

    /* Write image itself */
    if (fwrite(res, (w + pad) * sizeof(uint8_t), h, fh) != h) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP file data write error");

        return false;
    }

    fclose(fh);
    free(res);

    return true;
}

/*************************************************************************************************/

/* lrpt_image_dump_combo_bmp() */
bool lrpt_image_dump_combo_bmp(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid_red,
        uint8_t apid_green,
        uint8_t apid_blue,
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

    /* We're limiting our images to be 65535 * 65535 size at max */
    uint16_t w, h;

    w = (image->width > 65535) ? 65535 : image->width;
    h = (image->height > 65535) ? 65535 : image->height;

    uint8_t pad = ((w % 4) == 0) ? 0 : (4 - w % 4); /* Padding for BMP scan lines */

    /* Final buffer is just a BGR padded combination of requested APIDs */
    uint8_t *res = calloc(
            h * (3 * w + pad),
            sizeof(uint8_t));

    if (!res) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Can't allocate temporary buffer for storing RGB data");

        return false;
    }

    /* Fill resulting buffer with interleaved and padded BGR data (fill direction is from bottom
     * to top)
     */
    for (size_t j = 0; j < h; j++) {
        for (size_t i = 0; i < w; i++) {
            res[3 * i + (h - j - 1) * (3 * w + pad) + 0] =
                image->channels[apid_blue - 64][i + j * w];
            res[3 * i + (h - j - 1) * (3 * w + pad) + 1] =
                image->channels[apid_green - 64][i + j * w];
            res[3 * i + (h - j - 1) * (3 * w + pad) + 2] =
                image->channels[apid_red - 64][i + j * w];
        }
    }

    FILE *fh = fopen(fname, "wb");

    if (!fh) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open BMP file for writing");

        return false;
    }

    uint32_t image_size = (h * (3 * w + pad)); /* Padded image size */
    uint32_t file_size = (image_size + 54); /* Image size + header size */

    unsigned char b4_s[4];

    /* Write BMP identifier */
    fprintf(fh, "BM");

    /* Write BMP file size in bytes */
    lrpt_utils_s_uint32_t(file_size, b4_s, false);

    if (fwrite(b4_s, 1, 4, fh) != 4) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP file size write error");

        return false;
    }

    /* Write BMP Header constant data */
    unsigned char hdr_const_parts[12] = {
        0x00, 0x00, 0x00, 0x00, /* Reserved fields */
        0x36, 0x00, 0x00, 0x00, /* Data offset */
        0x28, 0x00, 0x00, 0x00 /* InfoHeader size */
    };

    if (fwrite(hdr_const_parts, 1, 12, fh) != 12) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP Header write error");

        return false;
    }

    /* Write width and height */
    lrpt_utils_s_uint32_t(w, b4_s, false); /* Width */

    if (fwrite(b4_s, 1, 4, fh) != 4) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP image width write error");

        return false;
    }

    lrpt_utils_s_uint32_t(h, b4_s, false); /* Height */

    if (fwrite(b4_s, 1, 4, fh) != 4) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP image height write error");

        return false;
    }

    /* Write BMP InfoHeader constant data, part 1 */
    unsigned char infohdr_const_parts_1[8] = {
        0x01, 0x00, /* Planes */
        0x18, 0x00, /* 24 bits per pixel */
        0x00, 0x00, 0x00, 0x00 /* Compression */
    };

    if (fwrite(infohdr_const_parts_1, 1, 8, fh) != 8) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP InfoHeader write error");

        return false;
    }

    /* Write BMP image size in bytes */
    lrpt_utils_s_uint32_t(image_size, b4_s, false);

    if (fwrite(b4_s, 1, 4, fh) != 4) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP image size write error");

        return false;
    }

    /* Write BMP InfoHeader constant data, part 2 */
    unsigned char infohdr_const_parts_2[16] = {
        0x23, 0x2E, 0x00, 0x00, /* 300 ppi horizontal resolution */
        0x23, 0x2E, 0x00, 0x00, /* 300 ppi vertical resolution */
        0x00, 0x00, 0x00, 0x00, /* Number of colors used */
        0x00, 0x00, 0x00, 0x00 /* Number of important colors */
    };

    if (fwrite(infohdr_const_parts_2, 1, 16, fh) != 16) {
        fclose(fh);
        free(res);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP InfoHeader write error");

        return false;
    }

    /* Write image itself */
    if (fwrite(res, (3 * w + pad) * sizeof(uint8_t), h, fh) != h) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "BMP file data write error");

        return false;
    }

    fclose(fh);
    free(res);

    return true;
}

/*************************************************************************************************/

/** \endcond */
