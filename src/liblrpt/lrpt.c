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
 * Basic data types, memory management and I/O routines.
 *
 * This source file contains common routines for internal data types and objects management etc.
 */

/*************************************************************************************************/

#include "lrpt.h"

#include "../../include/lrpt.h"
#include "error.h"

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* liblrpt requires that IEEE 754 floating point standard is used. However some compilers doesn't
 * provide special macro for testing that. We're quite lenient in this requirement so just plain
 * warning will be given during compilation.
 */
#ifndef __STDC_IEC_559__
#pragma message ("Your compiler doesn't define __STDC_IEC_559__ macro!")
#pragma message ("Support for IEEE 754 floats and doubles may be unavailable or limited!")
#endif

/*************************************************************************************************/

/** Local definition for 2 * Pi */
const double LRPT_M_2PI = 6.28318530717958647692;

/*************************************************************************************************/

/* lrpt_iq_data_alloc() */
lrpt_iq_data_t *lrpt_iq_data_alloc(
        size_t len,
        lrpt_error_t *err) {
    lrpt_iq_data_t *data = malloc(sizeof(lrpt_iq_data_t));

    if (!data) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/Q data object allocation failed");

        return NULL;
    }

    /* Set requested length and allocate storage for I and Q samples if length is not zero */
    data->len = len;

    if (len > 0) {
        data->iq = calloc(len, sizeof(complex double));

        /* Return NULL only if allocation attempt has failed */
        if (!data->iq) {
            lrpt_iq_data_free(data);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Data buffer allocation for I/Q data object failed");

            return NULL;
        }
    }
    else
        data->iq = NULL;

    return data;
}

/*************************************************************************************************/

/* lrpt_iq_data_free() */
void lrpt_iq_data_free(
        lrpt_iq_data_t *data) {
    if (!data)
        return;

    free(data->iq);
    free(data);
}

/*************************************************************************************************/

/* lrpt_iq_data_length() */
size_t lrpt_iq_data_length(
        const lrpt_iq_data_t *data) {
    if (!data)
        return 0;

    return data->len;
}

/*************************************************************************************************/

/* lrpt_iq_data_resize() */
bool lrpt_iq_data_resize(
        lrpt_iq_data_t *data,
        size_t new_len,
        lrpt_error_t *err) {
    /* We accept only valid data objects or simple empty objects */
    if (!data || ((data->len > 0) && !data->iq)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object is NULL or corrupted");

        return false;
    }

    /* If sizes are the same just return true */
    if (data->len == new_len)
        return true;

    /* In case of zero length create empty but valid data object */
    if (new_len == 0) {
        free(data->iq);

        data->len = 0;
        data->iq = NULL;
    }
    else {
        complex double *new_iq = reallocarray(data->iq, new_len, sizeof(complex double));

        if (!new_iq) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Data buffer reallocation for I/Q data object failed");

            return false;
        }
        else {
            /* Zero out newly allocated portion */
            if (new_len > data->len)
                memset(new_iq + data->len, 0, sizeof(complex double) * (new_len - data->len));

            data->len = new_len;
            data->iq = new_iq;
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_append() */
bool lrpt_iq_data_append(
        lrpt_iq_data_t *data,
        const lrpt_iq_data_t *add,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !add || (data == add)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Original and/or added I/Q data objects are NULL or the same");

        return false;
    }

    /* Silently ignore empty added data */
    if (add->len == 0)
        return true;

    if (n > (add->len - offset))
        n = (add->len - offset);

    const size_t old_len = data->len;

    /* Resize original storage */
    if (!lrpt_iq_data_resize(data, old_len + n, err))
        return false;

    /* Just copy extra samples */
    memcpy(data->iq + old_len, add->iq + offset, sizeof(complex double) * n);

    return true;
}

/*************************************************************************************************/

/* TODO add subset helper to extract part of the self samples */
/* lrpt_iq_data_from_iq() */
bool lrpt_iq_data_from_iq(
        lrpt_iq_data_t *data,
        const lrpt_iq_data_t *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !samples || (samples->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object and/or I/Q source data are NULL or I/Q source data is empty");

        return false;
    }

    if (n > (samples->len - offset))
        n = (samples->len - offset);

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, n, err))
        return false;

    /* Just copy samples */
    memcpy(data->iq, samples->iq + offset, sizeof(complex double) * n);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_iq() */
lrpt_iq_data_t *lrpt_iq_data_create_from_iq(
        const lrpt_iq_data_t *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!samples || (samples->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q source data is NULL or empty");

        return NULL;
    }

    if (n > (samples->len - offset))
        n = (samples->len - offset);

    /* Allocate new storage */
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(n, err);

    if (!data)
        return NULL;

    /* Convert samples */
    if (!lrpt_iq_data_from_iq(data, samples, offset, n, err)) {
        lrpt_iq_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_iq_data_from_complex() */
bool lrpt_iq_data_from_complex(
        lrpt_iq_data_t *data,
        const complex double *samples,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !samples) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object and/or I/Q samples array are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, n, err))
        return false;

    /* Just copy samples */
    memcpy(data->iq, samples, sizeof(complex double) * n);

    return true;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_iq_data_create_from_complex() */
lrpt_iq_data_t *lrpt_iq_data_create_from_complex(
        const complex double *samples,
        size_t n,
        lrpt_error_t *err) {
    if (!samples) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q samples array is NULL");

        return NULL;
    }

    /* Allocate new storage */
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(n, err);

    if (!data)
        return NULL;

    /* Convert samples */
    if (!lrpt_iq_data_from_complex(data, samples, n, err)) {
        lrpt_iq_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_iq_data_to_complex() */
bool lrpt_iq_data_to_complex(
        const lrpt_iq_data_t *data,
        complex double *samples,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !data->iq) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object is NULL or corrupted");

        return false;
    }

    if (n > data->len)
        n = data->len;

    /* Just copy samples */
    memcpy(samples, data->iq, sizeof(complex double) * n);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_alloc() */
lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(
        size_t len,
        lrpt_error_t *err) {
    lrpt_qpsk_data_t *data = malloc(sizeof(lrpt_qpsk_data_t));

    if (!data) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "QPSK data object allocation failed");

        return NULL;
    }

    /* Set requested length and allocate storage for symbols if length is not zero */
    data->len = len;

    if (len > 0) {
        /* Twice a length because 1 symbol consists of two bytes */
        data->qpsk = calloc(2 * len, sizeof(int8_t));

        /* Return NULL only if allocation attempt has failed */
        if (!data->qpsk) {
            lrpt_qpsk_data_free(data);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Data buffer allocation for QPSK data object failed");

            return NULL;
        }
    }
    else
        data->qpsk = NULL;

    return data;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_free() */
void lrpt_qpsk_data_free(
        lrpt_qpsk_data_t *data) {
    if (!data)
        return;

    free(data->qpsk);
    free(data);
}

/*************************************************************************************************/

/* lrpt_qpsk_data_length() */
size_t lrpt_qpsk_data_length(
        const lrpt_qpsk_data_t *data) {
    if (!data)
        return 0;

    return data->len;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_resize() */
bool lrpt_qpsk_data_resize(
        lrpt_qpsk_data_t *data,
        size_t new_len,
        lrpt_error_t *err) {
    /* We accept only valid data objects or simple empty objects */
    if (!data || ((data->len > 0) && !data->qpsk)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object is NULL or corrupted");

        return false;
    }

    /* Is sizes are the same just return true */
    if (data->len == new_len)
        return true;

    /* In case of zero length create empty but valid data object */
    if (new_len == 0) {
        free(data->qpsk);

        data->len = 0;
        data->qpsk = NULL;
    }
    else {
        int8_t *new_s = reallocarray(data->qpsk, 2 * new_len, sizeof(int8_t));

        if (!new_s) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Data buffer reallocation for QPSK data object failed");

            return false;
        }
        else {
            /* Zero out newly allocated portion */
            if (new_len > data->len)
                memset(new_s + 2 * data->len, 0, sizeof(int8_t) * 2 * (new_len - data->len));

            data->len = new_len;
            data->qpsk = new_s;
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_append() */
bool lrpt_qpsk_data_append(
        lrpt_qpsk_data_t *data,
        const lrpt_qpsk_data_t *add,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !add || (data == add)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Original and/or added QPSK data objects are NULL or the same");

        return false;
    }

    /* Silently ignore empty added data */
    if (add->len == 0)
        return true;

    if (n > (add->len - offset))
        n = (add->len - offset);

    const size_t old_len = data->len;

    /* Resize original storage */
    if (!lrpt_qpsk_data_resize(data, old_len + n, err))
        return false;

    /* Just copy extra symbols */
    memcpy(data->qpsk + 2 * old_len, add->qpsk + 2 * offset, sizeof(int8_t) * 2 * n);

    return true;
}

/*************************************************************************************************/

/* TODO add subset helper to extract part of the self symbols */
/* lrpt_qpsk_data_from_qpsk() */
bool lrpt_qpsk_data_from_qpsk(
        lrpt_qpsk_data_t *data,
        const lrpt_qpsk_data_t *symbols,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !symbols || (symbols->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object and/or QPSK source data are NULL or QPSK source data is empty");

        return false;
    }

    if (n > (symbols->len - offset))
        n = (symbols->len - offset);

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data, n, err))
        return false;

    /* Just copy symbols */
    memcpy(data->qpsk, symbols->qpsk + 2 * offset, sizeof(int8_t) * 2 * n);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_create_from_qpsk() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_qpsk(
        const lrpt_qpsk_data_t *symbols,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!symbols || (symbols->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK source data is NULL or empty");

        return NULL;
    }

    if (n > (symbols->len - offset))
        n = (symbols->len - offset);

    /* Allocate new storage */
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(n, err);

    if (!data)
        return NULL;

    /* Convert symbols */
    if (!lrpt_qpsk_data_from_qpsk(data, symbols, offset, n, err)) {
        lrpt_qpsk_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_qpsk_data_from_soft() */
bool lrpt_qpsk_data_from_soft(
        lrpt_qpsk_data_t *data,
        const int8_t *symbols,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object and/or QPSK soft symbols array are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data, n, err))
        return false;

    /* Just copy symbols */
    memcpy(data->qpsk, symbols, sizeof(int8_t) * 2 * n);

    return true;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_qpsk_data_create_from_soft() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_soft(
        const int8_t *symbols,
        size_t n,
        lrpt_error_t *err) {
    if (!symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK soft symbols array is NULL");

        return NULL;
    }

    /* Allocate new storage */
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(n, err);

    if (!data)
        return NULL;

    /* Convert symbols */
    if (!lrpt_qpsk_data_from_soft(data, symbols, n, err)) {
        lrpt_qpsk_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_qpsk_data_to_soft() */
bool lrpt_qpsk_data_to_soft(
        const lrpt_qpsk_data_t *data,
        int8_t *symbols,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !data->qpsk) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object is NULL or corrupted");

        return false;
    }

    if (n > data->len)
        n = data->len;

    /* Just copy symbols */
    memcpy(symbols, data->qpsk, sizeof(int8_t) * 2 * n);

    return true;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_qpsk_data_from_hard() */
bool lrpt_qpsk_data_from_hard(
        lrpt_qpsk_data_t *data,
        const unsigned char *symbols,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object and/or QPSK hard symbols array are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data, n, err))
        return false;

    /* Convert hard to soft and store in QPSK data object */
    size_t i = 0;
    uint8_t j = 0;

    while (i < n) {
        const unsigned char b = ((symbols[i / 4] >> (7 - j)) & 0x01);

        data->qpsk[2 * i + j % 2] = (b == 0x01) ? 127 : -127;
        j = (j + 1) % 8;

        if ((j % 2) == 0)
            i++;
    }

    return true;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_qpsk_data_create_from_hard() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_hard(
        const unsigned char *symbols,
        size_t n,
        lrpt_error_t *err) {
    if (!symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK hard symbols array is NULL");

        return NULL;
    }

    /* Allocate new storage */
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(n, err);

    if (!data)
        return NULL;

    /* Convert symbols */
    if (!lrpt_qpsk_data_from_hard(data, symbols, n, err)) {
        lrpt_qpsk_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_qpsk_data_to_hard() */
bool lrpt_qpsk_data_to_hard(
        const lrpt_qpsk_data_t *data,
        unsigned char *symbols,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !data->qpsk) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object is NULL or corrupted");

        return false;
    }

    if (n > data->len)
        n = data->len;

    /* Convert symbols */
    size_t i = 0;
    uint8_t j = 0;
    unsigned char b = 0x00;

    while (i < n) {
        if (data->qpsk[2 * i + j % 2] >= 0)
            b |= (1 << (7 - j));

        j++;

        if (j == 8) {
            symbols[i / 4] = b;
            b = 0x00;
            j = 0;
        }

        if ((j % 2) == 0)
            i++;

        if ((i == n) && ((j % 2) == 0))
            symbols[i / 4] = b;
    }

    return true;
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
        lrpt_image_t *image,
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

/** \endcond */
