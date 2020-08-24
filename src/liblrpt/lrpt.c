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

/*************************************************************************************************/

#include "lrpt.h"

#include "../../include/lrpt.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/* Allocate storage for raw I/Q data */
lrpt_iq_data_t *lrpt_iq_data_alloc(const size_t length) {
    lrpt_iq_data_t *handle = (lrpt_iq_data_t *)malloc(sizeof(lrpt_iq_data_t));

    if (!handle)
        return NULL;
    
    handle->len = length;

    handle->i = (double *)calloc(length, sizeof(double));
    handle->q = (double *)calloc(length, sizeof(double));

    if (!handle->i || !handle->q) {
        free(handle->i);
        free(handle->q);
        free(handle);

        return NULL;
    }

    return handle;
}

/*************************************************************************************************/

/* Free storage of I/Q data */
void lrpt_iq_data_free(lrpt_iq_data_t *handle) {
    if (!handle)
        return;

    free(handle->i);
    free(handle->q);
    free(handle);
}

/*************************************************************************************************/

/* Allocate storage for QPSK soft symbols data */
lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(const size_t length) {
    lrpt_qpsk_data_t *handle = (lrpt_qpsk_data_t *)malloc(sizeof(lrpt_qpsk_data_t));

    if (!handle)
        return NULL;

    handle->len = length;

    handle->s = (int8_t *)calloc(length, sizeof(int8_t));

    if (!handle->s) {
        free(handle->s);
        free(handle);

        return NULL;
    }

    return handle;
}

/*************************************************************************************************/

/* Resize QPSK soft symbols storage */
bool lrpt_qpsk_data_resize(lrpt_qpsk_data_t *handle, const size_t new_length) {
    int8_t *new_s = realloc(handle->s, new_length);

    if (!new_s)
        return false;
    else {
        handle->len = new_length;
        handle->s = new_s;

        return true;
    }
}

/*************************************************************************************************/

/* Free storage of QPSK soft symbols data */
void lrpt_qpsk_data_free(lrpt_qpsk_data_t *handle) {
    if (!handle)
        return;

    free(handle->s);
    free(handle);
}
