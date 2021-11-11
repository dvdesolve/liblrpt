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

#include "datatype.h"

#include "../../include/lrpt.h"
#include "error.h"

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* lrpt_iq_data_alloc() */
lrpt_iq_data_t *lrpt_iq_data_alloc(
        size_t len,
        lrpt_error_t *err) {
    lrpt_iq_data_t *data = malloc(sizeof(lrpt_iq_data_t));

    if (!data) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/Q data object allocation has failed");

        return NULL;
    }

    /* Set requested length and allocate storage for I/Q samples if length is not zero */
    data->len = len;

    if (len > 0) {
        data->iq = calloc(len, sizeof(complex double));

        if (!data->iq) {
            lrpt_iq_data_free(data);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Data buffer allocation for I/Q data object has failed");

            return NULL;
        }
    }
    else
        data->iq = NULL;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return data;
}

/*************************************************************************************************/

/* lrpt_iq_data_free() */
inline void lrpt_iq_data_free(
        lrpt_iq_data_t *data) {
    if (!data)
        return;

    free(data->iq);
    free(data);
}

/*************************************************************************************************/

/* lrpt_iq_data_length() */
inline size_t lrpt_iq_data_length(
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
    if (!data || ((data->len > 0) && !data->iq)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "I/Q data object is NULL or corrupted");

        return false;
    }

    /* If sizes are the same don't do anything */
    if (data->len == new_len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_PARAM,
                    "New length of I/Q data object equals to the old one");

        return true;
    }

    /* In case of zero length create empty I/Q data object */
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
                        "Data buffer reallocation for I/Q data object has failed");

            return false;
        }
        else {
            /* Zero out newly allocated part of I/Q data array */
            if (new_len > data->len)
                memset(new_iq + data->len, 0, sizeof(complex double) * (new_len - data->len));

            data->len = new_len;
            data->iq = new_iq;
        }
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_append() */
bool lrpt_iq_data_append(
        lrpt_iq_data_t *data_dest,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination I/Q data object is NULL");

        return false;
    }

    if (!data_src || ((data_src->len > 0) && !data_src->iq)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source I/Q data object is NULL or corrupted");

        return false;
    }

    if (data_dest == data_src) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Source and destination I/Q data objects are the same");

        return false;
    }

    /* Ignore empty source I/Q data objects */
    if (data_src->len == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "Source I/Q data object is empty");

        return true;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source I/Q data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Keep old length for further copying */
    const size_t old_len = data_dest->len;

    /* Resize destination storage */
    if (!lrpt_iq_data_resize(data_dest, old_len + n, err))
        return false;

    /* Just copy samples */
    memcpy(data_dest->iq + old_len, data_src->iq + offset, sizeof(complex double) * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_iq() */
bool lrpt_iq_data_from_iq(
        lrpt_iq_data_t *data_dest,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination I/Q data object is NULL");

        return false;
    }

    if (!data_src || ((data_src->len > 0) && !data_src->iq)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source I/Q data object is NULL or corrupted");

        return false;
    }

    if (data_dest == data_src) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Source and destination I/Q data objects are the same");

        return false;
    }

    /* Ignore empty source I/Q data objects */
    if (data_src->len == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "Source I/Q data object is empty");

        return true;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source I/Q data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Resize destination storage */
    if (!lrpt_iq_data_resize(data_dest, n, err))
        return false;

    /* Just copy samples */
    memcpy(data_dest->iq, data_src->iq + offset, sizeof(complex double) * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_iq() */
lrpt_iq_data_t *lrpt_iq_data_create_from_iq(
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_src || ((data_src->len > 0) && !data_src->iq) || (data_src->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source I/Q data object is NULL, corrupted or empty");

        return NULL;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source I/Q data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return NULL;
    }

    /* Allocate storage */
    lrpt_iq_data_t *data_dest = lrpt_iq_data_alloc(n, err);

    if (!data_dest)
        return NULL;

    /* Convert samples */
    if (!lrpt_iq_data_from_iq(data_dest, data_src, offset, n, err)) {
        lrpt_iq_data_free(data_dest);

        return NULL;
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return data_dest;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_complex() */
bool lrpt_iq_data_from_complex(
        lrpt_iq_data_t *data_dest,
        const complex double *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination I/Q data object is NULL");

        return false;
    }

    if (!samples) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source complex I/Q samples array is NULL");

        return false;
    }

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Resize storage */
    if (!lrpt_iq_data_resize(data_dest, n, err))
        return false;

    /* Just copy samples */
    memcpy(data_dest->iq, samples + offset, sizeof(complex double) * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_complex() */
lrpt_iq_data_t *lrpt_iq_data_create_from_complex(
        const complex double *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!samples) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source complex I/Q samples array is NULL");

        return NULL;
    }

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return NULL;
    }

    /* Allocate storage */
    lrpt_iq_data_t *data_dest = lrpt_iq_data_alloc(n, err);

    if (!data_dest)
        return NULL;

    /* Convert samples */
    if (!lrpt_iq_data_from_complex(data_dest, samples, offset, n, err)) {
        lrpt_iq_data_free(data_dest);

        return NULL;
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return data_dest;
}

/*************************************************************************************************/

/* lrpt_iq_data_to_complex() */
bool lrpt_iq_data_to_complex(
        complex double *samples,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_src || ((data_src->len > 0) && !data_src->iq) || (data_src->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source I/Q data object is NULL, corrupted or empty");

        return false;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source I/Q data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Just copy samples */
    memcpy(samples, data_src->iq + offset, sizeof(complex double) * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_doubles() */
bool lrpt_iq_data_from_doubles(
        lrpt_iq_data_t *data_dest,
        const double *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination I/Q data object is NULL");

        return false;
    }

    if (!samples) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source double-valued I/Q samples array is NULL");

        return false;
    }

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Resize storage */
    if (!lrpt_iq_data_resize(data_dest, n, err))
        return false;

    /* Just copy samples */
    memcpy(data_dest->iq, samples + 2 * offset, sizeof(double) * 2 * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_doubles() */
lrpt_iq_data_t *lrpt_iq_data_create_from_doubles(
        const double *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!samples) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source double-valued I/Q samples array is NULL");

        return NULL;
    }

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return NULL;
    }

    /* Allocate storage */
    lrpt_iq_data_t *data_dest = lrpt_iq_data_alloc(n, err);

    if (!data_dest)
        return NULL;

    /* Convert samples */
    if (!lrpt_iq_data_from_doubles(data_dest, samples, offset, n, err)) {
        lrpt_iq_data_free(data_dest);

        return NULL;
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return data_dest;
}

/*************************************************************************************************/

/* lrpt_iq_data_to_doubles() */
bool lrpt_iq_data_to_doubles(
        double *samples,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_src || ((data_src->len > 0) && !data_src->iq) || (data_src->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source I/Q data object is NULL, corrupted or empty");

        return false;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source I/Q data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Just copy samples */
    memcpy(samples, data_src->iq + offset, sizeof(double) * 2 * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_rb_alloc() */
lrpt_iq_rb_t *lrpt_iq_rb_alloc(
        size_t len,
        lrpt_error_t *err) {
    if (len == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Can't create empty I/Q ring buffer object");

        return NULL;
    }

    lrpt_iq_rb_t *rb = malloc(sizeof(lrpt_iq_rb_t));

    if (!rb) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/Q ring buffer object allocation has failed");

        return NULL;
    }

    /* Set requested length (reserve 1 element for full/empty detection) and allocate storage
     * for I/Q samples if length is not zero
     */
    rb->len = len + 1;
    rb->iq = calloc(len + 1, sizeof(complex double));

    if (!rb->iq) {
        lrpt_iq_rb_free(rb);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Data buffer allocation for I/Q ring buffer object has failed");

        return NULL;
    }

    /* Initially both head and tail are pointing to the same element */
    rb->head = 0;
    rb->tail = 0;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return rb;
}

/*************************************************************************************************/

/* lrpt_iq_rb_free() */
inline void lrpt_iq_rb_free(
        lrpt_iq_rb_t *rb) {
    if (!rb)
        return;

    free(rb->iq);
    free(rb);
}

/*************************************************************************************************/

/* lrpt_iq_rb_length() */
inline size_t lrpt_iq_rb_length(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return 0;

    /* Account for empty/full detection */
    return (rb->len - 1);
}

/*************************************************************************************************/

/* lrpt_iq_rb_used() */
inline size_t lrpt_iq_rb_used(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return 0;

    /* Save as soon as possible, so we'll get largest assessment of used size */
    size_t t = rb->tail;
    size_t h = rb->head;

    if (h >= t)
        return (h - t);
    else
        return (rb->len - t + h);
}

/*************************************************************************************************/

/* lrpt_iq_rb_avail() */
inline size_t lrpt_iq_rb_avail(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return 0;

    /* Account for empty/full detection */
    return (rb->len - 1 - lrpt_iq_rb_used(rb));
}

/*************************************************************************************************/

/* lrpt_iq_rb_is_empty() */
inline bool lrpt_iq_rb_is_empty(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return false;

    return (lrpt_iq_rb_used(rb) == 0);
}

/*************************************************************************************************/

/* lrpt_iq_rb_is_full() */
inline bool lrpt_iq_rb_is_full(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return false;

    return (lrpt_iq_rb_avail(rb) == 0);
}

/*************************************************************************************************/

/* lrpt_iq_rb_pop() */
bool lrpt_iq_rb_pop(
        lrpt_iq_rb_t *rb,
        lrpt_iq_data_t *data_dest,
        size_t n,
        lrpt_error_t *err) {
    if (!rb) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "I/Q ring buffer object is NULL");

        return false;
    }

    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination I/Q data object is NULL");

        return false;
    }

    /* Handle oversized requests */
    if (lrpt_iq_rb_used(rb) < n)
        n = lrpt_iq_rb_used(rb);

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Resize storage */
    if (!lrpt_iq_data_resize(data_dest, n, err))
        return false;

    if (rb->tail < rb->head) /* Not wrapped data */
        memcpy(data_dest->iq, rb->iq + rb->tail, sizeof(complex double) * n);
    else { /* Wrapped data */
        if ((rb->tail + n) < rb->len) /* Contiguous chunk */
            memcpy(data_dest->iq, rb->iq + rb->tail, sizeof(complex double) * n);
        else { /* Non-contiguous chunk */
            const size_t tn = rb->len - rb->tail;

            /* Till the end */
            memcpy(data_dest->iq, rb->iq + rb->tail, sizeof(complex double) * tn);

            /* From the start */
            memcpy(data_dest->iq + tn, rb->iq, sizeof(complex double) * (n - tn));
        }
    }

    /* Advance tail position */
    rb->tail = (rb->tail + n) % rb->len;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_rb_push() */
bool lrpt_iq_rb_push(
        lrpt_iq_rb_t *rb,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!rb) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "I/Q ring buffer object is NULL");

        return false;
    }

    if (!data_src || ((data_src->len > 0) && !data_src->iq)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source I/Q data object is NULL or corrupted");

        return false;
    }

    /* Ignore empty source I/Q data objects */
    if (data_src->len == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "Source I/Q data object is empty");

        return true;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    if ((lrpt_iq_rb_avail(rb) < n)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Not enough space in I/Q ring buffer object for push");

        return false;
    }

    if (rb->head < rb->tail) /* Wrapped data */
        memcpy(rb->iq + rb->head, data_src->iq + offset, sizeof(complex double) * n);
    else { /* Not wrapped data */
        if ((rb->head + n) < rb->len) /* Contiguous chunk */
            memcpy(rb->iq + rb->head, data_src->iq + offset, sizeof(complex double) * n);
        else { /* Non-contiguous chunk */
            const size_t tn = rb->len - rb->head;

            /* Till the end */
            memcpy(rb->iq + rb->head, data_src->iq + offset, sizeof(complex double) * tn);

            /* From the start */
            memcpy(rb->iq, data_src->iq + offset + tn, sizeof(complex double) * (n - tn));
        }
    }

    /* Advance head position */
    rb->head = (rb->head + n) % rb->len;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

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
                    "QPSK data object allocation has failed");

        return NULL;
    }

    /* Set requested length and allocate storage for symbols if length is not zero */
    data->len = len;

    if (len > 0) {
        /* Twice a length because 1 symbol consists of two bytes */
        data->qpsk = calloc(2 * len, sizeof(int8_t));

        if (!data->qpsk) {
            lrpt_qpsk_data_free(data);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Data buffer allocation for QPSK data object has failed");

            return NULL;
        }
    }
    else
        data->qpsk = NULL;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return data;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_free() */
inline void lrpt_qpsk_data_free(
        lrpt_qpsk_data_t *data) {
    if (!data)
        return;

    free(data->qpsk);
    free(data);
}

/*************************************************************************************************/

/* lrpt_qpsk_data_length() */
inline size_t lrpt_qpsk_data_length(
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
    if (!data || ((data->len > 0) && !data->qpsk)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "QPSK data object is NULL or corrupted");

        return false;
    }

    /* Is sizes are the same just don't do anything */
    if (data->len == new_len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_PARAM,
                    "New length of QPSK data object equals to the old one");

        return true;
    }

    /* In case of zero length create empty QPSK data object */
    if (new_len == 0) {
        free(data->qpsk);

        data->len = 0;
        data->qpsk = NULL;
    }
    else {
        /* Twice a length because 1 symbol consists of two bytes */
        int8_t *new_s = reallocarray(data->qpsk, 2 * new_len, sizeof(int8_t));

        if (!new_s) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Data buffer reallocation for QPSK data object has failed");

            return false;
        }
        else {
            /* Zero out newly allocated part of QPSK data array */
            if (new_len > data->len)
                memset(new_s + 2 * data->len, 0, sizeof(int8_t) * 2 * (new_len - data->len));

            data->len = new_len;
            data->qpsk = new_s;
        }
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_append() */
bool lrpt_qpsk_data_append(
        lrpt_qpsk_data_t *data_dest,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination QPSK data object is NULL");

        return false;
    }

    if (!data_src || ((data_src->len > 0) && !data_src->qpsk)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source QPSK data object is NULL or corrupted");

        return false;
    }

    if (data_dest == data_src) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Source and destination QPSK data objects are the same");

        return false;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source QPSK data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Keep old length for further copying */
    const size_t old_len = data_dest->len;

    /* Resize destination storage */
    if (!lrpt_qpsk_data_resize(data_dest, old_len + n, err))
        return false;

    /* Just copy symbols */
    memcpy(data_dest->qpsk + 2 * old_len, data_src->qpsk + 2 * offset, sizeof(int8_t) * 2 * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_from_qpsk() */
bool lrpt_qpsk_data_from_qpsk(
        lrpt_qpsk_data_t *data_dest,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination QPSK data object is NULL");

        return false;
    }

    if (!data_src || ((data_src->len > 0) && !data_src->qpsk)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source QPSK data object is NULL or corrupted");

        return false;
    }

    if (data_dest == data_src) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Source and destination QPSK data objects are the same");

        return false;
    }

    /* Ignore empty source QPSK data objects */
    if (data_src->len == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "Source QPSK data object is empty");

        return true;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source QPSK data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Resize destination storage */
    if (!lrpt_qpsk_data_resize(data_dest, n, err))
        return false;

    /* Just copy symbols */
    memcpy(data_dest->qpsk, data_src->qpsk + 2 * offset, sizeof(int8_t) * 2 * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_create_from_qpsk() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_qpsk(
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_src || ((data_src->len > 0) && !data_src->qpsk) || (data_src->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source QPSK data object is NULL, corrupted or empty");

        return NULL;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source QPSK data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return NULL;
    }

    /* Allocate storage */
    lrpt_qpsk_data_t *data_dest = lrpt_qpsk_data_alloc(n, err);

    if (!data_dest)
        return NULL;

    /* Convert symbols */
    if (!lrpt_qpsk_data_from_qpsk(data_dest, data_src, offset, n, err)) {
        lrpt_qpsk_data_free(data_dest);

        return NULL;
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return data_dest;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_from_soft() */
bool lrpt_qpsk_data_from_soft(
        lrpt_qpsk_data_t *data_dest,
        const int8_t *symbols,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination QPSK data object is NULL");

        return false;
    }

    if (!symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source soft QPSK symbols array is NULL");

        return false;
    }

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data_dest, n, err))
        return false;

    /* Just copy symbols */
    memcpy(data_dest->qpsk, symbols + 2 * offset, sizeof(int8_t) * 2 * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_create_from_soft() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_soft(
        const int8_t *symbols,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source soft QPSK symbols array is NULL");

        return NULL;
    }

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return NULL;
    }

    /* Allocate storage */
    lrpt_qpsk_data_t *data_dest = lrpt_qpsk_data_alloc(n, err);

    if (!data_dest)
        return NULL;

    /* Convert symbols */
    if (!lrpt_qpsk_data_from_soft(data_dest, symbols, offset, n, err)) {
        lrpt_qpsk_data_free(data_dest);

        return NULL;
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return data_dest;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_to_soft() */
bool lrpt_qpsk_data_to_soft(
        int8_t *symbols,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_src || ((data_src->len > 0) && !data_src->qpsk) || (data_src->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source QPSK data object is NULL, corrupted or empty");

        return false;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source QPSK data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Just copy symbols */
    memcpy(symbols, data_src->qpsk + 2 * offset, sizeof(int8_t) * 2 * n);

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_from_hard() */
bool lrpt_qpsk_data_from_hard(
        lrpt_qpsk_data_t *data_dest,
        const unsigned char *symbols,
        size_t n,
        lrpt_error_t *err) {
    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination QPSK data object is NULL");

        return false;
    }

    if (!symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source hard QPSK symbols array is NULL");

        return false;
    }

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data_dest, n, err))
        return false;

    /* Convert hard symbols to soft and store them in QPSK data object */
    size_t i = 0;
    uint8_t j = 0;

    while (i < n) {
        const unsigned char b = (symbols[i / 4] >> (7 - j)) & 0x01;

        data_dest->qpsk[2 * i + j % 2] = (b == 0x01) ? 127 : -127;
        j = (j + 1) % 8;

        if ((j % 2) == 0)
            i++;
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_create_from_hard() */
lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_hard(
        const unsigned char *symbols,
        size_t n,
        lrpt_error_t *err) {
    if (!symbols) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source hard QPSK symbols array is NULL");

        return NULL;
    }

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return NULL;
    }

    /* Allocate storage */
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(n, err);

    if (!data)
        return NULL;

    /* Convert symbols */
    if (!lrpt_qpsk_data_from_hard(data, symbols, n, err)) {
        lrpt_qpsk_data_free(data);

        return NULL;
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return data;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_to_hard() */
bool lrpt_qpsk_data_to_hard(
        unsigned char *symbols,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data_src || ((data_src->len > 0) && !data_src->qpsk) || (data_src->len == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source QPSK data object is NULL, corrupted or empty");

        return false;
    }

    /* Check for offset correctness */
    if (offset >= data_src->len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Offset exceeds source QPSK data length");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Convert symbols */
    size_t i = 0;
    uint8_t j = 0;
    unsigned char b = 0x00;

    while (i < n) {
        if (data_src->qpsk[2 * offset + 2 * i + j % 2] >= 0)
            b |= 1 << (7 - j);

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

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_alloc() */
lrpt_qpsk_rb_t *lrpt_qpsk_rb_alloc(
        size_t len,
        lrpt_error_t *err) {
    if (len == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Can't create empty QPSK ring buffer object");

        return NULL;
    }

    lrpt_qpsk_rb_t *rb = malloc(sizeof(lrpt_qpsk_rb_t));

    if (!rb) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "QPSK ring buffer object allocation has failed");

        return NULL;
    }

    /* Set requested length (reserve 1 element for full/empty detection) and allocate storage
     * for QPSK symbols if length is not zero
     */
    rb->len = len + 1;
    rb->qpsk = calloc(2 * (len + 1), sizeof(int8_t));

    /* Return NULL only if allocation attempt has failed */
    if (!rb->qpsk) {
        lrpt_qpsk_rb_free(rb);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Data buffer allocation for QPSK ring buffer object has failed");

        return NULL;
    }

    /* Initially both head and tail are the same */
    rb->head = 0;
    rb->tail = 0;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return rb;
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_free() */
inline void lrpt_qpsk_rb_free(
        lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return;

    free(rb->qpsk);
    free(rb);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_length() */
inline size_t lrpt_qpsk_rb_length(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return 0;

    /* Account for empty/full detection */
    return (rb->len - 1);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_used() */
inline size_t lrpt_qpsk_rb_used(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return 0;

    /* Save as soon as possible, so we'll get largest assessment of used size */
    size_t t = rb->tail;
    size_t h = rb->head;

    if (h >= t)
        return (h - t);
    else
        return (rb->len - t + h);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_avail() */
inline size_t lrpt_qpsk_rb_avail(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return 0;

    /* Account for empty/full detection */
    return (rb->len - 1 - lrpt_qpsk_rb_used(rb));
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_is_empty() */
inline bool lrpt_qpsk_rb_is_empty(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return false;

    return (lrpt_qpsk_rb_used(rb) == 0);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_is_full() */
inline bool lrpt_qpsk_rb_is_full(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return false;

    return (lrpt_qpsk_rb_avail(rb) == 0);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_pop() */
bool lrpt_qpsk_rb_pop(
        lrpt_qpsk_rb_t *rb,
        lrpt_qpsk_data_t *data_dest,
        size_t n,
        lrpt_error_t *err) {
    if (!rb) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "QPSK ring buffer object is NULL");

        return false;
    }

    if (!data_dest) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Destination QPSK data object is NULL");

        return false;
    }

    /* Handle oversized requests */
    if (lrpt_qpsk_rb_used(rb) < n)
        n = lrpt_qpsk_rb_used(rb);

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data_dest, n, err))
        return false;

    if (rb->tail < rb->head) /* Not wrapped data */
        memcpy(data_dest->qpsk, rb->qpsk + 2 * rb->tail, sizeof(int8_t) * 2 * n);
    else { /* Wrapped data */
        if ((rb->tail + n) < rb->len) /* Contiguous chunk */
            memcpy(data_dest->qpsk, rb->qpsk + 2 * rb->tail, sizeof(int8_t) * 2 * n);
        else { /* Non-contiguous chunk */
            const size_t tn = rb->len - rb->tail;

            /* Till the end */
            memcpy(data_dest->qpsk, rb->qpsk + 2 * rb->tail, sizeof(int8_t) * 2 * tn);

            /* From the start */
            memcpy(data_dest->qpsk + 2 * tn, rb->qpsk, sizeof(int8_t) * 2 * (n - tn));
        }
    }

    /* Advance tail position */
    rb->tail = (rb->tail + n) % rb->len;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_push() */
bool lrpt_qpsk_rb_push(
        lrpt_qpsk_rb_t *rb,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!rb) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "QPSK ring buffer object is NULL");

        return false;
    }

    if (!data_src || ((data_src->len > 0) && !data_src->qpsk)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "Source QPSK data object is NULL or corrupted");

        return false;
    }

    /* Ignore empty source QPSK data objects */
    if (data_src->len == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "Source QPSK data object is empty");

        return true;
    }

    /* Handle oversized requests */
    if (n > (data_src->len - offset))
        n = data_src->len - offset;

    /* Just finish when nothing to do */
    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_INFO, LRPT_ERR_CODE_NODATA,
                    "No data to process");

        return true;
    }

    if ((lrpt_qpsk_rb_avail(rb) < n)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Not enough space in QPSK ring buffer object for push");

        return false;
    }

    if (rb->head < rb->tail) /* Wrapped data */
        memcpy(rb->qpsk + 2 * rb->head, data_src->qpsk + 2 * offset, sizeof(int8_t) * 2 * n);
    else { /* Not wrapped data */
        if ((rb->head + n) < rb->len) /* Contiguous chunk */
            memcpy(rb->qpsk + 2 * rb->head, data_src->qpsk + 2 * offset, sizeof(int8_t) * 2 * n);
        else { /* Non-contiguous chunk */
            const size_t tn = rb->len - rb->head;

            /* Till the end */
            memcpy(rb->qpsk + 2 * rb->head, data_src->qpsk + 2 * offset, sizeof(int8_t) * 2 * tn);

            /* From the start */
            memcpy(rb->qpsk, data_src->qpsk + 2 * offset + 2 * tn, sizeof(int8_t) * 2 * (n - tn));
        }
    }

    /* Advance head position */
    rb->head = (rb->head + n) % rb->len;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/** \endcond */
