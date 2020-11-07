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
        double sample_rate,
        double ripple,
        uint8_t num_poles,
        lrpt_dsp_filter_type_t type) {
    /* Try to allocate our handle */
    lrpt_dsp_filter_t *handle = malloc(sizeof(lrpt_dsp_filter_t));

    if (!handle)
        return NULL;

    /* NULL-init internal storage for safe deallocation */
    handle->a = NULL;
    handle->b = NULL;
    handle->x_i = NULL;
    handle->y_i = NULL;
    handle->x_q = NULL;
    handle->y_q = NULL;

    /* Initialize filter parameters */
    handle->cutoff = (double)(bandwidth / 2) / sample_rate;
    handle->ripple = ripple;

    /* Number of poles should be even and not greater than 252 to fit in uint8_t type */
    if ((num_poles > 252) || ((num_poles % 2) == 1)) {
        free(handle);

        return NULL;
    }

    handle->npoles = num_poles;
    handle->type = type;
    handle->ri_i = 0;
    handle->ri_q = 0;

    /* Allocate data and coefficients arrays */
    size_t n = (size_t)(num_poles + 3);

    double * const ta = calloc(n, sizeof(double));
    double * const tb = calloc(n, sizeof(double));
    handle->a = calloc(n, sizeof(double));
    handle->b = calloc(n, sizeof(double));

    /* Allocate saved input and output arrays */
    n = (size_t)(num_poles + 1);

    handle->x_i = calloc(n, sizeof(double));
    handle->y_i = calloc(n, sizeof(double));
    handle->x_q = calloc(n, sizeof(double));
    handle->y_q = calloc(n, sizeof(double));

    /* Check for allocation problems */
    if (!ta || !tb || !handle->a || !handle->b ||
            !handle->x_i || !handle->y_i || !handle->x_q || !handle->y_q) {
        lrpt_dsp_filter_deinit(handle);
        free(ta);
        free(tb);

        return NULL;
    }

    /* Initialize specific coefficient arrays explicitly */
    handle->a[2] = 1.0;
    handle->b[2] = 1.0;

    /* S-domain to Z-domain conversion */
    const double t = 2.0 * tan(0.5);

    /* Cutoff frequency */
    const double w = LRPT_M_2PI * handle->cutoff;

    /* Low Pass to Low Pass or Low Pass to High Pass transform */
    double k;

    if (type == LRPT_DSP_FILTER_TYPE_HIGHPASS)
        k = -cos((w + 1.0) / 2.0) / cos((w - 1.0) / 2.0);
    else if (type == LRPT_DSP_FILTER_TYPE_LOWPASS)
        k = sin((1.0 - w) / 2.0) / sin((1.0 + w) / 2.0);
    else
        k = 1.0;

    /* Find coefficients for 2-pole filter for each pole pair */
    for (uint8_t p = 1; p <= (num_poles / 2); p++) {
        /* Calculate the pole location on the unit circle */
        double tmp =
            M_PI / (double)num_poles / 2.0 + (double)(p - 1) * M_PI / (double)num_poles;
        double rp = -cos(tmp);
        double ip =  sin(tmp);

        /* Warp from a circle to an ellipse */
        if (ripple > 0.0) {
            tmp = 100.0 / (100.0 - ripple);

            const double es = sqrt(tmp * tmp - 1.0);

            tmp = 1.0 / (double)num_poles;

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
        for (uint8_t i = 0; i < (num_poles + 3); i++) {
            ta[i] = handle->a[i];
            tb[i] = handle->b[i];
        }

        for (uint8_t i = 2; i < (num_poles + 3); i++) {
            handle->a[i] = a0 * ta[i] + a1 * ta[i - 1] + a2 * ta[i - 2];
            handle->b[i] =      tb[i] - b1 * tb[i - 1] - b2 * tb[i - 2];
        }
    }

    /* Free temporary arrays */
    free(ta);
    free(tb);

    /* Finish combining coefficients */
    handle->b[2] = 0.0;

    for (uint8_t i = 0; i < (num_poles + 1); i++) {
        handle->a[i] =  handle->a[i + 2];
        handle->b[i] = -handle->b[i + 2];
    }

    /* Normalize the gain */
    double sa = 0.0;
    double sb = 0.0;

    for (uint8_t i = 0; i < (num_poles + 1); i++) {
        if (type == LRPT_DSP_FILTER_TYPE_LOWPASS) {
            sa += handle->a[i];
            sb += handle->b[i];
        }
        else if (type == LRPT_DSP_FILTER_TYPE_HIGHPASS) {
            sa += handle->a[i] * (double)((-1) ^ i);
            sb += handle->b[i] * (double)((-1) ^ i);
        }
    }

    const double gain = sa / (1.0 - sb);

    for (uint8_t i = 0; i < (num_poles + 1); i++)
        handle->a[i] /= gain;

    return handle;
}

/*************************************************************************************************/

/* lrpt_dsp_filter_deinit() */
void lrpt_dsp_filter_deinit(
        lrpt_dsp_filter_t *handle) {
    if (!handle)
        return;

    free(handle->a);
    free(handle->b);
    free(handle->x_i);
    free(handle->y_i);
    free(handle->x_q);
    free(handle->y_q);
    free(handle);
}

/*************************************************************************************************/

/* lrpt_dsp_filter_apply() */
bool lrpt_dsp_filter_apply(
        lrpt_dsp_filter_t *handle,
        lrpt_iq_data_t *iq_data) {
    /* Return immediately if handle is empty */
    if (!handle)
        return false;

    /* For convenient access purposes */
    const uint8_t npp1 = handle->npoles + 1;
    const size_t len = iq_data->len;
    lrpt_iq_raw_t * const samples = iq_data->iq;

    /* I samples first, then Q */
    for (size_t k = 0; k < 2; k++) {
        /* Filter samples in the buffer */
        for (size_t buf_idx = 0; buf_idx < len; buf_idx++) {
            double *cur_s;
            double *cur_x;
            double *cur_y;
            uint8_t *cur_ri;

            if (k == 0) {
                cur_s = &(samples[buf_idx].i);
                cur_x = handle->x_i;
                cur_y = handle->y_i;
                cur_ri = &(handle->ri_i);
            }
            else {
                cur_s = &(samples[buf_idx].q);
                cur_x = handle->x_q;
                cur_y = handle->y_q;
                cur_ri = &(handle->ri_q);
            }

            /* Calculate and save filtered samples */
            double yn0 = *cur_s * handle->a[0];

            for (uint8_t idx = 1; idx < npp1; idx++) {
                double y;

                /* Summate contribution of past input samples */
                y = handle->a[idx];
                y   *= cur_x[*cur_ri];
                yn0 += y;

                /* Summate contribution of past output samples */
                y    = handle->b[idx];
                y   *= cur_y[*cur_ri];
                yn0 += y;

                /* Advance ring buffers index */
                (*cur_ri)++;

                if (*cur_ri >= npp1)
                    *cur_ri = 0;
            }

            /* Save new yn0 output to y ring buffer */
            cur_y[*cur_ri] = yn0;

            /* Save current input sample to x ring buffer */
            cur_x[*cur_ri] = *cur_s;

            /* Return filtered samples */
            *cur_s = yn0;
        }
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
