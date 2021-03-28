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
 * NOTE: source code of this module is based on xdemorse application and adapted for liblrpt
 * internal use!
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * DSP routines.
 *
 * This source file contains routines for performing different DSP tasks.
 */

/*************************************************************************************************/

#include "dsp.h"

#include "../../include/lrpt.h"
#include "lrpt.h"

#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* lrpt_dsp_filter_init() */
lrpt_dsp_filter_t *lrpt_dsp_filter_init(
        uint32_t bandwidth,
        double samplerate,
        double ripple,
        uint8_t num_poles,
        lrpt_dsp_filter_type_t type) {
    /* Try to allocate our filter object */
    lrpt_dsp_filter_t *filter = malloc(sizeof(lrpt_dsp_filter_t));

    if (!filter)
        return NULL;

    /* NULL-init internal storage for safe deallocation */
    filter->a = NULL;
    filter->b = NULL;
    filter->x = NULL;
    filter->y = NULL;

    /* Number of poles should be even and not greater than 252 to fit in uint8_t type */
    if ((num_poles > 252) || ((num_poles % 2) != 0)) {
        free(filter);

        return NULL;
    }

    filter->npoles = num_poles;
    filter->ri = 0;

    /* Allocate data and coefficients arrays */
    uint8_t n = (num_poles + 3);

    double * const ta = calloc(n, sizeof(double));
    double * const tb = calloc(n, sizeof(double));
    filter->a = calloc(n, sizeof(double));
    filter->b = calloc(n, sizeof(double));

    /* Allocate saved input and output arrays */
    n = (num_poles + 1);

    filter->x = calloc(n, sizeof(complex double));
    filter->y = calloc(n, sizeof(complex double));

    /* Check for allocation problems */
    if (!ta || !tb || !filter->a || !filter->b || !filter->x || !filter->y) {
        lrpt_dsp_filter_deinit(filter);
        free(ta);
        free(tb);

        return NULL;
    }

    /* Initialize specific coefficient arrays explicitly */
    filter->a[2] = 1.0;
    filter->b[2] = 1.0;

    /* S-domain to Z-domain conversion */
    const double t = 2.0 * tan(0.5);

    /* Cutoff frequency (as a fraction of sample rate) */
    const double w = LRPT_M_2PI * (bandwidth / 2.0 / samplerate);

    /* Low Pass to Low Pass or Low Pass to High Pass transform */
    double k;

    if (type == LRPT_DSP_FILTER_TYPE_HIGHPASS)
        k = -cos((w + 1.0) / 2.0) / cos((w - 1.0) / 2.0);
    else if (type == LRPT_DSP_FILTER_TYPE_LOWPASS)
        k = sin((1.0 - w) / 2.0) / sin((1.0 + w) / 2.0);
    else
        k = 1.0;

    /* Find coefficients for 2-pole filter for each pole pair */
    for (uint8_t i = 1; i <= (num_poles / 2); i++) {
        /* Calculate the pole location on the unit circle */
        double tmp = M_PI / (num_poles * 2.0) + M_PI * (i - 1) / num_poles;
        double rp = -cos(tmp);
        double ip =  sin(tmp);

        /* Warp from a circle to an ellipse */
        if (ripple > 0.0) {
            tmp = 100.0 / (100.0 - ripple);

            const double es = sqrt(tmp * tmp - 1.0);

            tmp = (1.0 / num_poles);

            const double vx = tmp * asinh(1.0 / es);
            double kx = tmp * acosh(1.0 / es);

            kx = cosh(kx);
            rp *= sinh(vx) / kx;
            ip *= cosh(vx) / kx;
        }

        /* S-domain to Z-domain conversion */
        const double m = rp * rp + ip * ip;
        double d = 4.0 - 4.0 * rp * t + m * t * t;
        const double xn0 = t * t / d;
        const double xn1 = 2.0 * t * t / d;
        const double xn2 = t * t / d;
        const double yn1 = (8.0 - 2.0 * m * t * t) / d;
        const double yn2 = (-4.0 - 4.0 * rp * t - m * t * t) / d;

        /* (Low Pass to Low Pass) or (Low Pass to High Pass) transform */
        d  = 1.0 + yn1 * k - yn2 * k * k;

        const double a0 = (xn0 - xn1 * k + xn2 * k * k) / d;
        double a1 = (-2.0 * xn0 * k + xn1 + xn1 * k * k - 2.0 * xn2 * k) / d;
        const double a2 = (xn0 * k * k - xn1 * k + xn2) / d;
        double b1 = (2.0 * k + yn1 + yn1 * k * k - 2.0 * yn2 * k) / d;
        const double b2 = (-k * k - yn1 * k + yn2) / d;

        if (type == LRPT_DSP_FILTER_TYPE_HIGHPASS) {
            a1 = -a1;
            b1 = -b1;
        }

        /* Add coefficients to the cascade */
        for (uint8_t j = 0; j < (num_poles + 3); j++) {
            ta[j] = filter->a[j];
            tb[j] = filter->b[j];
        }

        for (uint8_t j = 2; j < (num_poles + 3); j++) {
            filter->a[j] = a0 * ta[j] + a1 * ta[j - 1] + a2 * ta[j - 2];
            filter->b[j] = tb[j] - b1 * tb[j - 1] - b2 * tb[j - 2];
        }
    }

    /* Free temporary arrays */
    free(ta);
    free(tb);

    /* Finish combining coefficients */
    filter->b[2] = 0.0;

    for (uint8_t i = 0; i < (num_poles + 1); i++) {
        filter->a[i] =  filter->a[i + 2];
        filter->b[i] = -filter->b[i + 2];
    }

    /* Normalize the gain */
    double sa = 0.0;
    double sb = 0.0;

    for (uint8_t i = 0; i < (num_poles + 1); i++) {
        if (type == LRPT_DSP_FILTER_TYPE_LOWPASS) {
            sa += filter->a[i];
            sb += filter->b[i];
        }
        else if (type == LRPT_DSP_FILTER_TYPE_HIGHPASS) {
            sa += filter->a[i] * ((-1) ^ i);
            sb += filter->b[i] * ((-1) ^ i);
        }
    }

    const double gain = sa / (1.0 - sb);

    for (uint8_t i = 0; i < (num_poles + 1); i++)
        filter->a[i] /= gain;

    return filter;
}

/*************************************************************************************************/

/* lrpt_dsp_filter_deinit() */
void lrpt_dsp_filter_deinit(
        lrpt_dsp_filter_t *filter) {
    if (!filter)
        return;

    free(filter->a);
    free(filter->b);
    free(filter->x);
    free(filter->y);
    free(filter);
}

/*************************************************************************************************/

/* lrpt_dsp_filter_apply() */
bool lrpt_dsp_filter_apply(
        lrpt_dsp_filter_t *filter,
        lrpt_iq_data_t *data) {
    /* Return immediately if filter is empty */
    if (!filter)
        return false;

    /* For convenient access purposes */
    const uint8_t npp1 = filter->npoles + 1;
    complex double * const samples = data->iq;

    /* Filter samples in the buffer */
    for (size_t i = 0; i < data->len; i++) {
        complex double *cur_s = samples + i;

        /* Calculate and save filtered samples */
        complex double yn0 = *cur_s * filter->a[0];

        for (uint8_t j = 1; j < npp1; j++) {
            /* Summate contribution of past input samples */
            yn0 += filter->x[filter->ri] * filter->a[j];

            /* Summate contribution of past output samples */
            yn0 += filter->y[filter->ri] * filter->b[j];

            /* Advance ring buffers index */
            filter->ri++;

            if (filter->ri >= npp1)
                filter->ri = 0;
        }

        /* Save new yn0 output to y ring buffer */
        filter->y[filter->ri] = yn0;

        /* Save current input sample to x ring buffer */
        filter->x[filter->ri] = *cur_s;

        /* Return filtered samples */
        *cur_s = yn0;
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
