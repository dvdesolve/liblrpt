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

/* lrpt_iq_data_from_samples() */
bool lrpt_iq_data_from_samples(
        lrpt_iq_data_t *data,
        const complex double *iq,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !iq) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object and/or I/Q samples array are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, len, err))
        return false;

    /* Merge samples into I/Q data */
    memcpy(data->iq, iq, sizeof(complex double) * len);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_samples() */
lrpt_iq_data_t *lrpt_iq_data_create_from_samples(
        const complex double *iq,
        size_t len,
        lrpt_error_t *err) {
    if (!iq) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q samples array is NULL");

        return NULL;
    }

    lrpt_iq_data_t *data = lrpt_iq_data_alloc(len, err);

    if (!data)
        return NULL;

    if (!lrpt_iq_data_from_samples(data, iq, len, err)) {
        lrpt_iq_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_doubles() */
bool lrpt_iq_data_from_doubles(
        lrpt_iq_data_t *data,
        const double *i,
        const double *q,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !i || !q) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object and/or I/Q sample arrays are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, len, err))
        return false;

    /* Repack doubles into I/Q data */
    for (size_t k = 0; k < len; k++)
        data->iq[k] = i[k] + q[k] * I;

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_doubles() */
lrpt_iq_data_t *lrpt_iq_data_create_from_doubles(
        const double *i,
        const double *q,
        size_t len,
        lrpt_error_t *err) {
    if (!i || !q) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I and/or Q sample arrays are NULL");

        return NULL;
    }

    lrpt_iq_data_t *data = lrpt_iq_data_alloc(len, err);

    if (!data)
        return NULL;

    if (!lrpt_iq_data_from_doubles(data, i, q, len, err)) {
        lrpt_iq_data_free(data);

        return NULL;
    }

    return data;
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

/* lrpt_qpsk_data_from_symbols() */
bool lrpt_qpsk_data_from_symbols(
        lrpt_qpsk_data_t *data,
        const int8_t *qpsk,
        size_t len,
        lrpt_error_t *err) {
    if (!data || !qpsk) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object and/or QPSK symbols array are NULL");

        return false;
    }

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data, len, err))
        return false;

    /* Merge symbols into QPSK data */
    memcpy(data->qpsk, qpsk, sizeof(int8_t) * len);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_create_from_symbols() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_symbols(
        const int8_t *qpsk,
        size_t len,
        lrpt_error_t *err) {
    if (!qpsk) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK symbols array is NULL");

        return NULL;
    }

    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(len, err);

    if (!data)
        return NULL;

    if (!lrpt_qpsk_data_from_symbols(data, qpsk, len, err)) {
        lrpt_qpsk_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_to_ints() */
bool lrpt_qpsk_data_to_ints(
        const lrpt_qpsk_data_t *data,
        int8_t *qpsk,
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

    memcpy(qpsk, data->qpsk, sizeof(int8_t) * len);

    return true;
}

/*************************************************************************************************/

/** \endcond */
