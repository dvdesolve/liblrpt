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
 * Author: Artem Litvinovich
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Dediffcoding routines.
 */

/*************************************************************************************************/

#include "dediffcoder.h"

#include "../../include/lrpt.h"
#include "../liblrpt/datatype.h"
#include "../liblrpt/error.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/** Returns integer square root for given value.
 *
 * \param lut Initilized lookup table for sqrt().
 * \param value Input value.
 *
 * \return Integer square root of value.
 */
static inline int8_t lut_isqrt(
        const uint8_t lut[],
        int16_t value);

/*************************************************************************************************/

/* lut_isqrt() */
static inline int8_t lut_isqrt(
        const uint8_t lut[],
        int16_t value) {
    if (value >= 0)
        return lut[value];
    else
        return -lut[-value];
}

/*************************************************************************************************/

/* lrpt_dsp_dediffcoder_init() */
lrpt_dsp_dediffcoder_t *lrpt_dsp_dediffcoder_init(
        lrpt_error_t *err) {
    /* Allocate dediffcoder object */
    lrpt_dsp_dediffcoder_t *dediff = malloc(sizeof(lrpt_dsp_dediffcoder_t));

    if (!dediff) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Dediffcoder object allocation has failed");

        return NULL;
    }

    /* Try to allocate lookup table */
    dediff->lut = calloc(16385, sizeof(uint8_t));

    if (!dediff->lut) {
        lrpt_dsp_dediffcoder_deinit(dediff);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Lookup table allocation has failed");

        return NULL;
    }

    /* Populate lookup table */
    for (uint16_t i = 0; i < 16385; i++)
        dediff->lut[i] = sqrt(i);

    dediff->pr_I = 0;
    dediff->pr_Q = 0;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return dediff;
}

/*************************************************************************************************/

/* lrpt_dsp_dediffcoder_deinit() */
void lrpt_dsp_dediffcoder_deinit(
        lrpt_dsp_dediffcoder_t *dediff) {
    if (!dediff)
        return;

    free(dediff->lut);

    free(dediff);
}

/*************************************************************************************************/

/* lrpt_dsp_dediffcoder_exec() */
bool lrpt_dsp_dediffcoder_exec(
        lrpt_dsp_dediffcoder_t *dediff,
        lrpt_qpsk_data_t *data) {
    /* Return immediately if dediffcoder and/or data are empty */
    if (!dediff || !data || data->len == 0)
        return false;

    int8_t t1 = data->qpsk[0];
    int8_t t2 = data->qpsk[1];

    data->qpsk[0] = lut_isqrt(dediff->lut, data->qpsk[0] * dediff->pr_I);
    data->qpsk[1] = lut_isqrt(dediff->lut, -(data->qpsk[1]) * dediff->pr_Q);

    for (size_t i = 2; i <= (2 * data->len - 2); i += 2) {
        int8_t x = data->qpsk[i];
        int8_t y = data->qpsk[i + 1];

        data->qpsk[i] = lut_isqrt(dediff->lut, data->qpsk[i] * t1);
        data->qpsk[i + 1] = lut_isqrt(dediff->lut, -(data->qpsk[i + 1]) * t2);

        t1 = x;
        t2 = y;
    }

    dediff->pr_I = t1;
    dediff->pr_Q = t2;

    return true;
}

/*************************************************************************************************/

/** \endcond */
