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

    /* Is sizes are the same just return true */
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

/* lrpt_iq_data_from_complex() */
bool lrpt_iq_data_from_complex(
        lrpt_iq_data_t *data,
        const complex double *samples,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !samples) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object and/or I/Q samples array are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, len, err))
        return false;

    /* Just copy samples */
    memcpy(data->iq, samples, sizeof(complex double) * len);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_complex() */
lrpt_iq_data_t *lrpt_iq_data_create_from_complex(
        const complex double *samples,
        size_t len,
        lrpt_error_t *err) {
    if (!samples) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q samples array is NULL");

        return NULL;
    }

    /* Allocate new storage */
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(len, err);

    if (!data)
        return NULL;

    /* Convert samples */
    if (!lrpt_iq_data_from_complex(data, samples, len, err)) {
        lrpt_iq_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* lrpt_iq_data_to_complex() */
bool lrpt_iq_data_to_complex(
        const lrpt_iq_data_t *data,
        complex double *samples,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !data->iq) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object is NULL or corrupted");

        return false;
    }

    if (len > data->len)
        len = data->len;

    /* Just copy samples */
    memcpy(samples, data->iq, sizeof(complex double) * len);

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

    /* Set requested length and allocate storage for soft symbols if length is not zero */
    data->len = len;

    if (len > 0) {
        data->qpsk = calloc(len, sizeof(int8_t));

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
        int8_t *new_s = reallocarray(data->qpsk, new_len, sizeof(int8_t));

        if (!new_s) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Data buffer reallocation for QPSK data object failed");

            return false;
        }
        else {
            /* Zero out newly allocated portion */
            if (new_len > data->len)
                memset(new_s + data->len, 0, sizeof(int8_t) * (new_len - data->len));

            data->len = new_len;
            data->qpsk = new_s;
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_from_soft() */
bool lrpt_qpsk_data_from_soft(
        lrpt_qpsk_data_t *data,
        const int8_t *symbols,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object and/or QPSK soft symbols array are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data, len, err))
        return false;

    /* Just copy symbols */
    memcpy(data->qpsk, symbols, sizeof(int8_t) * len);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_create_from_soft() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_soft(
        const int8_t *symbols,
        size_t len,
        lrpt_error_t *err) {
    if (!symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK soft symbols array is NULL");

        return NULL;
    }

    /* Allocate new storage */
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(len, err);

    if (!data)
        return NULL;

    /* Convert symbols */
    if (!lrpt_qpsk_data_from_soft(data, symbols, len, err)) {
        lrpt_qpsk_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_to_soft() */
bool lrpt_qpsk_data_to_soft(
        const lrpt_qpsk_data_t *data,
        int8_t *symbols,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !data->qpsk) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object is NULL or corrupted");

        return false;
    }

    if (len > data->len)
        len = data->len;

    /* Just copy symbols */
    memcpy(symbols, data->qpsk, sizeof(int8_t) * len);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_from_hard() */
bool lrpt_qpsk_data_from_hard(
        lrpt_qpsk_data_t *data,
        const unsigned char *symbols,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object and/or QPSK hard symbols array are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data, 8 * len, err))
        return false;

    /* Convert hard to soft and store in QPSK data object */
    for (size_t i = 0; i < len; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            const unsigned char b = ((symbols[i] >> (7 - j)) & 0x01);

            data->qpsk[8 * i + j] = (b == 0x01) ? 127 : -127;
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_create_from_hard() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_hard(
        const unsigned char *symbols,
        size_t len,
        lrpt_error_t *err) {
    if (!symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK hard symbols array is NULL");

        return NULL;
    }

    /* Allocate new storage */
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(8 * len, err);

    if (!data)
        return NULL;

    /* Convert symbols */
    if (!lrpt_qpsk_data_from_hard(data, symbols, len, err)) {
        lrpt_qpsk_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_to_hard() */
bool lrpt_qpsk_data_to_hard(
        const lrpt_qpsk_data_t *data,
        unsigned char *symbols,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !data->qpsk) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object is NULL or corrupted");

        return false;
    }

    if (len > data->len)
        len = data->len;

    /* Convert symbols */
    for (size_t i = 0; i <= (len / 8); i++) {
        unsigned char b = 0x00;

        for (uint8_t j = 0; j < 8; j++) {
            if ((i == (len / 8)) && ((8 * i + j) == len))
                break;

            if (data->qpsk[8 * i + j] >= 0)
                b |= (1 << (7 - j));
        }

        symbols[i] = b;
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
