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
 * Basic data types, memory management and I/O routines.
 *
 * This source file contains common routines for internal data types and objects management etc.
 */

/*************************************************************************************************/

#include "lrpt.h"

#include "../../include/lrpt.h"

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

/** Local definition for 2 * π */
const double LRPT_M_2PI = 6.28318530717958647692;

/*************************************************************************************************/

/* lrpt_iq_data_alloc() */
lrpt_iq_data_t *lrpt_iq_data_alloc(
        size_t length) {
    lrpt_iq_data_t *data = malloc(sizeof(lrpt_iq_data_t));

    if (!data)
        return NULL;

    /* Set requested length and allocate storage for I and Q samples if length is not zero */
    data->len = length;

    if (length > 0) {
        data->iq = calloc(length, sizeof(complex double));

        /* Return NULL only if allocation attempt has failed */
        if (!data->iq) {
            lrpt_iq_data_free(data);

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
    return data->len;
}

/*************************************************************************************************/

/* lrpt_iq_data_resize() */
bool lrpt_iq_data_resize(
        lrpt_iq_data_t *data,
        size_t new_length) {
    /* We accept only valid data objects or simple empty objects */
    if (!data || ((data->len > 0) && !data->iq))
        return false;

    /* Is sizes are the same just return true */
    if (data->len == new_length)
        return true;

    /* In case of zero length create empty but valid data object */
    if (new_length == 0) {
        free(data->iq);

        data->len = 0;
        data->iq = NULL;
    }
    else {
        complex double *new_iq = reallocarray(data->iq, new_length, sizeof(complex double));

        if (!new_iq)
            return false;
        else {
            /* Zero out newly allocated portion */
            if (new_length > data->len)
                memset(new_iq + data->len, 0, new_length - data->len);

            data->len = new_length;
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
        size_t length) {
    if (!data)
        return false;

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, length))
        return false;

    /* Merge samples into I/Q data */
    for (size_t k = 0; k < length; k++)
        data->iq[k] = *(iq + k); /* TODO may be use memcpy here */

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_samples() */
lrpt_iq_data_t *lrpt_iq_data_create_from_samples(
        const complex double *iq,
        size_t length) {
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(length);

    if (!data)
        return NULL;

    if (!lrpt_iq_data_from_samples(data, iq, length)) {
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
        size_t length) {
    if (!data)
        return false;

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, length))
        return false;

    /* Repack doubles into I/Q data */
    for (size_t k = 0; k < length; k++)
        data->iq[k] = i[k] + q[k] * I;

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_doubles() */
lrpt_iq_data_t *lrpt_iq_data_create_from_doubles(
        const double *i,
        const double *q,
        size_t length) {
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(length);

    if (!data)
        return NULL;

    if (!lrpt_iq_data_from_doubles(data, i, q, length)) {
        lrpt_iq_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_alloc() */
lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(
        size_t length) {
    lrpt_qpsk_data_t *data = malloc(sizeof(lrpt_qpsk_data_t));

    if (!data)
        return NULL;

    /* Set requested length and allocate storage for soft symbols if length is not zero */
    data->len = length;

    if (length > 0) {
        data->qpsk = calloc(length, sizeof(int8_t));

        /* Return NULL only if allocation attempt has failed */
        if (!data->qpsk) {
            lrpt_qpsk_data_free(data);

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
    return data->len;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_resize() */
bool lrpt_qpsk_data_resize(
        lrpt_qpsk_data_t *data,
        size_t new_length) {
    /* We accept only valid data objects or simple empty objects */
    if (!data || ((data->len > 0) && !data->qpsk))
        return false;

    /* Is sizes are the same just return true */
    if (data->len == new_length)
        return true;

    /* In case of zero length create empty but valid data object */
    if (new_length == 0) {
        free(data->qpsk);

        data->len = 0;
        data->qpsk = NULL;
    }
    else {
        int8_t *new_s = reallocarray(data->qpsk, new_length, 1);

        if (!new_s)
            return false;
        else {
            /* Zero out newly allocated portion */
            if (new_length > data->len)
                memset(new_s + data->len, 0, new_length - data->len);

            data->len = new_length;
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
        size_t length) {
    if (!data)
        return false;

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data, length))
        return false;

    /* Merge symbols into QPSK data */
    for (size_t k = 0; k < length; k++)
        data->qpsk[k] = *(qpsk + k); /* TODO may be use memcpy here */

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_create_from_symbols() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_symbols(
        const int8_t *qpsk,
        size_t length) {
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(length);

    if (!data)
        return NULL;

    if (!lrpt_qpsk_data_from_symbols(data, qpsk, length)) {
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
        size_t length) {
    if (!data || !data->qpsk || (length > data->len))
        return false;

    memcpy(qpsk, data->qpsk, length);

    return true;
}

/*************************************************************************************************/

/** \endcond */
