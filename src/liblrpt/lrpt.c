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

/* lrpt_iq_data_alloc() */
lrpt_iq_data_t *lrpt_iq_data_alloc(
        size_t length) {
    lrpt_iq_data_t *handle = malloc(sizeof(lrpt_iq_data_t));

    if (!handle)
        return NULL;

    /* Set requested length and allocate storage for I and Q samples if <length> is not zero */
    handle->len = length;

    if (length > 0) {
        handle->iq = calloc(length, sizeof(lrpt_iq_raw_t));

        /* Return <NULL> only if allocation attempt has failed */
        if (!handle->iq) {
            lrpt_iq_data_free(handle);

            return NULL;
        }
    }
    else
        handle->iq = NULL;

    return handle;
}

/*************************************************************************************************/

/* lrpt_iq_data_free() */
void lrpt_iq_data_free(
        lrpt_iq_data_t *handle) {
    if (!handle)
        return;

    free(handle->iq);
    free(handle);
}

/*************************************************************************************************/

/* lrpt_iq_data_length() */
size_t lrpt_iq_data_length(
        const lrpt_iq_data_t *handle) {
    return handle->len;
}

/*************************************************************************************************/

/* lrpt_iq_data_resize() */
bool lrpt_iq_data_resize(
        lrpt_iq_data_t *handle,
        size_t new_length) {
    /* We accept only valid handles or simple empty handles */
    if (!handle || ((handle->len > 0) && !handle->iq))
        return false;

    /* Is sizes are the same just return true */
    if (handle->len == new_length)
        return true;

    /* In case of zero length create empty but valid handle */
    if (new_length == 0) {
        free(handle->iq);

        handle->len = 0;
        handle->iq = NULL;
    }
    else {
        lrpt_iq_raw_t *new_iq = reallocarray(handle->iq, new_length, sizeof(lrpt_iq_raw_t));

        if (!new_iq)
            return false;
        else {
            /* Wipe out newly allocated portion */
            if (new_length > handle->len)
                memset(new_iq + handle->len, 0, new_length - handle->len);

            handle->len = new_length;
            handle->iq = new_iq;
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_doubles() */
bool lrpt_iq_data_from_doubles(
        lrpt_iq_data_t *handle,
        const double *i,
        const double *q,
        size_t length) {
    if (!handle)
        return false;

    /* Resize storage */
    if (!lrpt_iq_data_resize(handle, length))
        return false;

    /* Repack doubles into I/Q data */
    for (size_t k = 0; k < length; k++) {
        handle->iq[k].i = *(i + k);
        handle->iq[k].q = *(q + k);
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_doubles() */
lrpt_iq_data_t *lrpt_iq_data_create_from_doubles(
        const double *i,
        const double *q,
        size_t length) {
    lrpt_iq_data_t *handle = lrpt_iq_data_alloc(length);

    if (!handle)
        return NULL;

    if (!lrpt_iq_data_from_doubles(handle, i, q, length)) {
        lrpt_iq_data_free(handle);

        return NULL;
    }

    return handle;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_samples() */
bool lrpt_iq_data_from_samples(
        lrpt_iq_data_t *handle,
        const lrpt_iq_raw_t *iq,
        size_t length) {
    if (!handle)
        return false;

    /* Resize storage */
    if (!lrpt_iq_data_resize(handle, length))
        return false;

    /* Merge samples into I/Q data */
    for (size_t k = 0; k < length; k++)
        handle->iq[k] = *(iq + k);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_samples() */
lrpt_iq_data_t *lrpt_iq_data_create_from_samples(
        const lrpt_iq_raw_t *iq,
        size_t length) {
    lrpt_iq_data_t *handle = lrpt_iq_data_alloc(length);

    if (!handle)
        return NULL;

    if (!lrpt_iq_data_from_samples(handle, iq, length)) {
        lrpt_iq_data_free(handle);

        return NULL;
    }

    return handle;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_alloc() */
lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(
        size_t length) {
    lrpt_qpsk_data_t *handle = malloc(sizeof(lrpt_qpsk_data_t));

    if (!handle)
        return NULL;

    /* Set requested length and allocate storage for soft symbols if <length> is not zero */
    handle->len = length;

    if (length > 0) {
        handle->s = calloc(length, sizeof(int8_t));

        /* Return NULL only if allocation attempt has failed */
        if (!handle->s) {
            lrpt_qpsk_data_free(handle);

            return NULL;
        }
    }
    else
        handle->s = NULL;

    return handle;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_free() */
void lrpt_qpsk_data_free(
        lrpt_qpsk_data_t *handle) {
    if (!handle)
        return;

    free(handle->s);
    free(handle);
}

/*************************************************************************************************/

/* lrpt_qpsk_data_resize() */
bool lrpt_qpsk_data_resize(
        lrpt_qpsk_data_t *handle,
        size_t new_length) {
    /* We accept only valid handles or simple empty handles */
    if (!handle || ((handle->len > 0) && !handle->s))
        return false;

    /* Is sizes are the same just return true */
    if (handle->len == new_length)
        return true;

    /* In case of zero length create empty but valid handle */
    if (new_length == 0) {
        free(handle->s);

        handle->len = 0;
        handle->s = NULL;
    }
    else {
        int8_t *new_s = reallocarray(handle->s, new_length, sizeof(int8_t));

        if (!new_s)
            return false;
        else {
            /* Wipe out newly allocated portion */
            if (new_length > handle->len)
                memset(new_s + handle->len, 0, new_length - handle->len);

            handle->len = new_length;
            handle->s = new_s;
        }
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
