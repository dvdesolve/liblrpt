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

        /* Return NULL only if allocation attempt has failed */
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
    /* Accept only valid data objects or plain empty objects */
    if (!data || ((data->len > 0) && !data->iq)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object is NULL or corrupted");

        return false;
    }

    /* If sizes are the same don't do anything */
    if (data->len == new_len)
        return true;

    /* In case of zero length create plain empty data object */
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
                    "Original and/or added I/Q data objects are NULL or equivalent");

        return false;
    }

    /* Silently ignore empty data */
    if (add->len == 0)
        return true;

    /* Handle oversized requests */
    if (n > (add->len - offset))
        n = (add->len - offset);

    /* Keep old length for further calculations */
    const size_t old_len = data->len;

    /* Resize original storage */
    if (!lrpt_iq_data_resize(data, old_len + n, err))
        return false;

    /* Just copy samples */
    memcpy(data->iq + old_len, add->iq + offset, sizeof(complex double) * n);

    return true;
}

/*************************************************************************************************/

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

    /* Handle oversized requests */
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
                    "I/Q source data object is NULL or empty");

        return NULL;
    }

    /* Handle oversized requests */
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

/* lrpt_iq_data_from_complex() */
bool lrpt_iq_data_from_complex(
        lrpt_iq_data_t *data,
        const complex double *samples,
        size_t offset,
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
    memcpy(data->iq, samples + offset, sizeof(complex double) * n);

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
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q samples array is NULL");

        return NULL;
    }

    /* Allocate new storage */
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(n, err);

    if (!data)
        return NULL;

    /* Convert samples */
    if (!lrpt_iq_data_from_complex(data, samples, offset, n, err)) {
        lrpt_iq_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* lrpt_iq_data_to_complex() */
bool lrpt_iq_data_to_complex(
        complex double *samples,
        const lrpt_iq_data_t *data,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !data->iq) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object is NULL or corrupted");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data->len - offset))
        n = (data->len - offset);

    /* Just copy samples */
    memcpy(samples, data->iq + offset, sizeof(complex double) * n);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_doubles() */
bool lrpt_iq_data_from_doubles(
        lrpt_iq_data_t *data,
        const double *samples,
        size_t offset,
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
    memcpy(data->iq, samples + 2 * offset, sizeof(complex double) * 2 * n);

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
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q samples array is NULL");

        return NULL;
    }

    /* Allocate new storage */
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(n, err);

    if (!data)
        return NULL;

    /* Convert samples */
    if (!lrpt_iq_data_from_doubles(data, samples, offset, n, err)) {
        lrpt_iq_data_free(data);

        return NULL;
    }

    return data;
}

/*************************************************************************************************/

/* lrpt_iq_data_to_doubles() */
bool lrpt_iq_data_to_doubles(
        double *samples,
        const lrpt_iq_data_t *data,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!data || !data->iq) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object is NULL or corrupted");

        return false;
    }

    /* Handle oversized requests */
    if (n > (data->len - offset))
        n = (data->len - offset);

    /* Just copy samples */
    memcpy(samples, data->iq + offset, sizeof(complex double) * n);

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
    rb->len = (len + 1);
    rb->iq = calloc(len + 1, sizeof(complex double));

    /* Return NULL only if allocation attempt has failed */
    if (!rb->iq) {
        lrpt_iq_rb_free(rb);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Data buffer allocation for I/Q ring buffer object has failed");

        return NULL;
    }

    /* Initially both head and tail are the same */
    rb->head = 0;
    rb->tail = 0;

    return rb;
}

/*************************************************************************************************/

/* lrpt_iq_rb_free() */
void lrpt_iq_rb_free(
        lrpt_iq_rb_t *rb) {
    if (!rb)
        return;

    free(rb->iq);
    free(rb);
}

/*************************************************************************************************/

/* lrpt_iq_rb_length() */
size_t lrpt_iq_rb_length(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return 0;

    /* Account for empty/full detection */
    return (rb->len - 1);
}

/*************************************************************************************************/

/* lrpt_iq_rb_used() */
size_t lrpt_iq_rb_used(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return 0;

    /* Save as soon as possible */
    size_t t = rb->tail;
    size_t h = rb->head; /* We're estimating largest possible used buffer size */

    if (h >= t)
        return (h - t);
    else
        return (rb->len - t + h);
}

/*************************************************************************************************/

/* lrpt_iq_rb_avail() */
size_t lrpt_iq_rb_avail(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return 0;

    return (rb->len - 1 - lrpt_iq_rb_used(rb));
}

/*************************************************************************************************/

/* lrpt_iq_rb_is_empty() */
bool lrpt_iq_rb_is_empty(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return false;

    return (lrpt_iq_rb_used(rb) == 0);
}

/*************************************************************************************************/

/* lrpt_iq_rb_is_full() */
bool lrpt_iq_rb_is_full(
        const lrpt_iq_rb_t *rb) {
    if (!rb)
        return false;

    return (lrpt_iq_rb_avail(rb) == 0);
}

/*************************************************************************************************/

/* lrpt_iq_rb_pop() */
bool lrpt_iq_rb_pop(
        lrpt_iq_rb_t *rb,
        lrpt_iq_data_t *data,
        size_t n,
        lrpt_error_t *err) {
    if (!rb || !data) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q ring buffer object and/or I/Q data object are NULL");

        return false;
    }

    /* Handle oversized requests */
    if (lrpt_iq_rb_used(rb) < n)
        n = lrpt_iq_rb_used(rb);

    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "No I/Q data to pop");

        return false;
    }

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, n, err))
        return false;

    if (rb->tail < rb->head) /* Not wrapped data */
        memcpy(data->iq, rb->iq + rb->tail, sizeof(complex double) * n);
    else { /* Wrapped data */
        if ((rb->tail + n) < rb->len) /* Contiguous chunk */
            memcpy(data->iq, rb->iq + rb->tail, sizeof(complex double) * n);
        else { /* Non-contiguous chunk */
            const size_t tn = (rb->len - rb->tail);

            memcpy(data->iq, rb->iq + rb->tail, sizeof(complex double) * tn); /* Till the end */
            memcpy(data->iq + tn, rb->iq, sizeof(complex double) * (n - tn)); /* From the start */
        }
    }

    /* Advance tail position */
    rb->tail = ((rb->tail + n) % rb->len);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_rb_push() */
bool lrpt_iq_rb_push(
        lrpt_iq_rb_t *rb,
        const lrpt_iq_data_t *data,
        size_t offset,
        size_t n,
        lrpt_error_t *err) {
    if (!rb || !data || !data->iq) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q ring buffer object and/or I/Q data object are NULL or corrupted");

        return false;
    }

    /* Silently ignore empty data */
    if ((data->len == 0) || (n == 0))
        return true;

    /* Handle oversized requests */
    if (n > (data->len - offset))
        n = (data->len - offset);

    if ((lrpt_iq_rb_avail(rb) < n)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Not enough space in I/Q ring buffer object for push");

        return false;
    }

    if (rb->head < rb->tail) /* Wrapped data */
        memcpy(rb->iq + rb->head, data->iq + offset, sizeof(complex double) * n);
    else { /* Not wrapped data */
        if ((rb->head + n) < rb->len) /* Contiguous chunk */
            memcpy(rb->iq + rb->head, data->iq + offset, sizeof(complex double) * n);
        else { /* Non-contiguous chunk */
            const size_t tn = (rb->len - rb->head);

            memcpy(rb->iq + rb->head, data->iq + offset, sizeof(complex double) * tn); /* Till the end */
            memcpy(rb->iq, data->iq + offset + tn, sizeof(complex double) * (n - tn)); /* From the start */
        }
    }

    /* Advance head position */
    rb->head = ((rb->head + n) % rb->len);

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
                    "QPSK ring buffer object allocation failed");

        return NULL;
    }

    /* Set requested length (reserve 1 element for full/empty detection) and allocate storage
     * for QPSK bytes if length is not zero
     */
    rb->len = (len + 1);
    rb->qpsk = calloc(2 * (len + 1), sizeof(int8_t));

    /* Return NULL only if allocation attempt has failed */
    if (!rb->qpsk) {
        lrpt_qpsk_rb_free(rb);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Data buffer allocation for QPSK ring buffer object failed");

        return NULL;
    }

    /* Initially both head and tail are the same */
    rb->head = 0;
    rb->tail = 0;

    return rb;
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_free() */
void lrpt_qpsk_rb_free(
        lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return;

    free(rb->qpsk);
    free(rb);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_length() */
size_t lrpt_qpsk_rb_length(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return 0;

    return (rb->len - 1);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_used() */
size_t lrpt_qpsk_rb_used(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return 0;

    /* Save as soon as possible */
    size_t t = rb->tail;
    size_t h = rb->head; /* We're estimating largest possible used buffer size */

    if (h >= t)
        return (h - t);
    else
        return (rb->len - t + h);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_avail() */
size_t lrpt_qpsk_rb_avail(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return 0;

    return (rb->len - 1 - lrpt_qpsk_rb_used(rb));
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_is_empty() */
bool lrpt_qpsk_rb_is_empty(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return false;

    return (lrpt_qpsk_rb_used(rb) == 0);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_is_full() */
bool lrpt_qpsk_rb_is_full(
        const lrpt_qpsk_rb_t *rb) {
    if (!rb)
        return false;

    return (lrpt_qpsk_rb_avail(rb) == 0);
}

/*************************************************************************************************/

/* lrpt_qpsk_rb_pop() */
bool lrpt_qpsk_rb_pop(
        lrpt_qpsk_rb_t *rb,
        lrpt_qpsk_data_t *data,
        size_t n,
        lrpt_error_t *err) {
    if (!rb || !data) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK ring buffer object and/or QPSK data object are NULL");

        return false;
    }

    if (lrpt_qpsk_rb_used(rb) < n)
        n = lrpt_qpsk_rb_used(rb);

    if (n == 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "No QPSK data to pop");

        return false;
    }

    if (!lrpt_qpsk_data_resize(data, n, err))
        return false;

    if (rb->tail < rb->head) /* Not wrapped data */
        memcpy(data->qpsk, rb->qpsk + 2 * rb->tail, sizeof(int8_t) * 2 * n);
    else { /* Wrapped data */
        if ((rb->tail + n) < rb->len) /* Contiguous chunk */
            memcpy(data->qpsk, rb->qpsk + 2 * rb->tail, sizeof(int8_t) * 2 * n);
        else { /* Non-contiguous chunk */
            const size_t tn = (rb->len - rb->tail);

            memcpy(data->qpsk, rb->qpsk + 2 * rb->tail, sizeof(int8_t) * 2 * tn); /* Till the end */
            memcpy(data->qpsk + 2 * tn, rb->qpsk, sizeof(int8_t) * 2 * (n - tn)); /* From the start */
        }
    }

    /* Advance tail position */
    rb->tail = ((rb->tail + n) % rb->len);

    return true;
}

/*************************************************************************************************/

/* TODO add offset */
/* lrpt_qpsk_rb_push() */
bool lrpt_qpsk_rb_push(
        lrpt_qpsk_rb_t *rb,
        const lrpt_qpsk_data_t *data,
        size_t n,
        lrpt_error_t *err) {
    if (!rb || !data || !data->qpsk) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK ring buffer object and/or QPSK data object are NULL or corrupted");

        return false;
    }

    if ((data->len == 0) || (n == 0))
        return true;

    if (data->len < n)
        n = data->len;

    if ((lrpt_qpsk_rb_avail(rb) < n)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Not enough space in QPSK ring buffer object for push");

        return false;
    }

    if (rb->head < rb->tail) /* Wrapped data */
        memcpy(rb->qpsk + 2 * rb->head, data->qpsk, sizeof(int8_t) * 2 * n);
    else { /* Not wrapped data */
        if ((rb->head + n) < rb->len) /* Contiguous chunk */
            memcpy(rb->qpsk + 2 * rb->head, data->qpsk, sizeof(int8_t) * 2 * n);
        else { /* Non-contiguous chunk */
            const size_t tn = (rb->len - rb->head);

            memcpy(rb->qpsk + 2 * rb->head, data->qpsk, sizeof(int8_t) * 2 * tn); /* Till the end */
            memcpy(rb->qpsk, data->qpsk + 2 * tn, sizeof(int8_t) * 2 * (n - tn)); /* From the start */
        }
    }

    /* Advance head position */
    rb->head = ((rb->head + n) % rb->len);

    return true;
}

/*************************************************************************************************/

/** \endcond */
