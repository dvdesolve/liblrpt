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

#ifndef LRPT_LIBLRPT_DSP_H
#define LRPT_LIBLRPT_DSP_H

/*************************************************************************************************/

#include "../../include/lrpt.h"

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

/* DSP filter types */
typedef enum lrpt_dsp_filter_type__ {
    LRPT_DSP_FILTER_TYPE_LOWPASS,
    LRPT_DSP_FILTER_TYPE_HIGHPASS,
    LRPT_DSP_FILTER_TYPE_BANDPASS
} lrpt_dsp_filter_type_t;

/* DSP filter object */
typedef struct lrpt_dsp_filter__ {
    /* Cutoff frequency as a fraction of sample rate */
    double cutoff;

    /* Passband ripple as a percentage */
    double ripple;

    /* Number of poles, must be even. Max value is limited to the 252 */
    uint8_t npoles;

    /* Filter type */
    lrpt_dsp_filter_type_t type;

    /* a and b coefficients of the filter */
    double *a, *b;

    /* Saved input and output values */
    double *x, *y;

    /* Ring buffer index */
    uint8_t ring_idx;

    /* I/Q samples data */
    lrpt_iq_data_t *iq_data;
} lrpt_dsp_filter_t;

/*************************************************************************************************/

lrpt_dsp_filter_t *lrpt_dsp_filter_init(
        lrpt_iq_data_t *iq_data,
        const uint32_t bandwidth,
        const double sample_rate,
        const double ripple,
        const uint8_t num_poles,
        const lrpt_dsp_filter_type_t type);
void lrpt_dsp_filter_deinit(lrpt_dsp_filter_t *handle);
bool lrpt_dsp_filter_apply(lrpt_dsp_filter_t *handle);

/*************************************************************************************************/

#endif
