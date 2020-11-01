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
 * Costas' phased-locked loop routines.
 *
 * This source file contains routines for Costas' PLL functionality.
 */

/*************************************************************************************************/

#include "pll.h"

#include "../../include/lrpt.h"
#include "../liblrpt/lrpt.h"

#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/*************************************************************************************************/

/* Library defaults */
const double PLL_INIT_FREQ = 0.001; /* Initial Costas' PLL frequency */
const double PLL_DAMPING = 1.0 / M_SQRT2; /* Damping factor */

const double PLL_ERR_SCALE_QPSK = 43.0; /* Scaling factors to control error magnitude */
const double PLL_ERR_SCALE_DOQPSK = 80.0;
const double PLL_ERR_SCALE_IDOQPSK = 80.0;

const double PLL_LOCKED_ERR_SCALE = 10.0; /* Phase error scale on lock */

const double PLL_DELTA_WINSIZE = 100.0; /* Moving average window for phase errors */
const double PLL_DELTA_WINSIZE_1 = PLL_DELTA_WINSIZE - 1.0;

const double PLL_LOCKED_BW_REDUCE = 4.0; /* PLL bandwidth reduction (in lock) */

const double PLL_AVG_WINSIZE = 20000.0; /* Interpolation factor is taken into account now */
const double PLL_LOCKED_WINSIZEX = 10.0; /* Error average window size multiplier (in lock) */

const double FREQ_MAX = 0.8; /* Maximum frequency range of locked PLL */

/*************************************************************************************************/

/** Clamps a double value to the range [-\p max; \p max].
 *
 * \param x Input value.
 * \param max Range limit for clamping.
 *
 * \return Value clamped to the range [-\p max; \p max].
 */
static inline double lrpt_demodulator_pll_clamp_double(double x, double max);

/** Returns tanh() for given value.
 *
 * \param lut Initialized lookup table for tanh().
 * \param value Input value.
 *
 * \return tanh() value.
 */
static inline double lrpt_demodulator_pll_lut_tanh(const double lut[], double value);

/** (Re)computes the alpha and beta coefficients of the Costas' PLL from damping and bandwidth
 * parameters and updates/sets them in the PLL object.
 *
 * \param handle PLL object.
 * \param damping Damping factor.
 * \param bandwidth Costas' PLL bandwidth.
 */
static void lrpt_demodulator_pll_recompute_coeffs(
        lrpt_demodulator_pll_t *handle,
        double damping,
        double bandwidth);

/*************************************************************************************************/

/* lrpt_demodulator_pll_clamp_double()
 *
 * Clamps a double value to the range [-max; max].
 */
static inline double lrpt_demodulator_pll_clamp_double(double x, double max) {
    if (x > max)
        return max;
    else if (x < -max)
        return -max;
    else
        return x;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_lut_tanh()
 *
 * Returns tanh() for given value.
 */
static inline double lrpt_demodulator_pll_lut_tanh(const double lut[], double value) {
    int16_t ival = (int16_t)value;

    if (ival > 127)
        return 1.0;
    else if (ival < -128)
        return -1.0;
    else
        return lut[ival + 128];
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_recompute_coeffs()
 *
 * (Re)computes the alpha and beta coefficients of the Costas' PLL from damping and bandwidth
 * parameters and updates/sets them in the PLL object.
 */
static void lrpt_demodulator_pll_recompute_coeffs(
        lrpt_demodulator_pll_t *handle,
        double damping,
        double bandwidth) {
    const double bw2 = bandwidth * bandwidth;
    const double denom = (1.0 + 2.0 * damping * bandwidth + bw2);

    handle->alpha = (4.0 * damping * bandwidth) / denom;
    handle->beta = (4.0 * bw2) / denom;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_init()
 *
 * Allocates and initializes PLL object.
 */
lrpt_demodulator_pll_t *lrpt_demodulator_pll_init(
        double bandwidth,
        double threshold,
        lrpt_demodulator_mode_t mode) {
    /* Try to allocate our handle */
    lrpt_demodulator_pll_t *handle = malloc(sizeof(lrpt_demodulator_pll_t));

    if (!handle)
        return NULL;

    /* Populate lookup table for tanh() */
    for (size_t i = 0; i < 256; i++)
        handle->lut_tanh[i] = tanh((double)(i - 128));

    /* Set default parameters */
    handle->nco_freq = PLL_INIT_FREQ;
    handle->nco_phase = 0.0;

    lrpt_demodulator_pll_recompute_coeffs(handle, PLL_DAMPING, bandwidth);
    handle->damping = PLL_DAMPING;
    handle->bw = bandwidth;

    /* Set up thresholds for PLL hysteresis feature */
    /* TODO may be we should use more customizable way to provide hysteresis feature of PLL lock */
    handle->pll_locked = threshold;
    handle->pll_unlocked = 1.03 * threshold;

    /* TODO previously it allowed to reset of avg_winsize in Costas_Resync()
     * if receiving is stopped and restarted while PLL is locked */
    /*handle->locked = false;*/

    /* Needed to cut off stray locks at startup */
    handle->moving_average = 1.0e6;

    /* Error scaling depends on modulation mode */
    switch (mode) {
        case LRPT_DEMODULATOR_MODE_QPSK:
            handle->err_scale = PLL_ERR_SCALE_QPSK;

            break;

        case LRPT_DEMODULATOR_MODE_DOQPSK:
            handle->err_scale = PLL_ERR_SCALE_DOQPSK;

            break;

        case LRPT_DEMODULATOR_MODE_IDOQPSK:
            handle->err_scale = PLL_ERR_SCALE_IDOQPSK;

            break;

        /* All other modes are unsupported */
        default:
            lrpt_demodulator_pll_deinit(handle);

            return NULL;

            break;
    }

    /* Initialize internal variables for phase correction routine */
    handle->avg_winsize = PLL_AVG_WINSIZE;
    handle->avg_winsize_1 = PLL_AVG_WINSIZE - 1.0;
    handle->delta = 0.0;

    return handle;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_deinit()
 *
 * Frees previously allocated PLL object.
 */
void lrpt_demodulator_pll_deinit(lrpt_demodulator_pll_t *handle) {
    free(handle);
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_mix()
 *
 * Performs mixing of a sample with PLL NCO frequency.
 */
complex double lrpt_demodulator_pll_mix(
        lrpt_demodulator_pll_t *handle,
        complex double sample) {
    const complex double nco_out = cexp(-(complex double)I * handle->nco_phase);
    const complex double retval = sample * nco_out;

    handle->nco_phase += handle->nco_freq;
    handle->nco_phase = fmod(handle->nco_phase, LRPT_M_2PI);

    return retval;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_delta()
 *
 * Computes the delta phase value to use when correcting the NCO frequency.
 */
double lrpt_demodulator_pll_delta(
        const lrpt_demodulator_pll_t *handle,
        complex double sample,
        complex double cosample) {
    double error;

    error =
        (lrpt_demodulator_pll_lut_tanh(handle->lut_tanh, creal(sample)) * cimag(sample)) -
        (lrpt_demodulator_pll_lut_tanh(handle->lut_tanh, cimag(cosample)) * creal(cosample));
    error /= handle->err_scale;

    return error;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_correct_phase()
 *
 * Corrects the phase angle of the Costas' PLL.
 */
void lrpt_demodulator_pll_correct_phase(
        lrpt_demodulator_pll_t *handle,
        double error,
        uint8_t interp_factor) {
    error = lrpt_demodulator_pll_clamp_double(error, 1.0);

    handle->moving_average *= handle->avg_winsize_1;
    handle->moving_average += fabs(error);
    handle->moving_average /= handle->avg_winsize;

    handle->nco_phase += handle->alpha * error;
    handle->nco_phase = fmod(handle->nco_phase, LRPT_M_2PI);

    /* Calculate sliding window average of phase error */
    if (handle->locked)
        error /= PLL_LOCKED_ERR_SCALE;

    handle->delta *= PLL_DELTA_WINSIZE_1;
    handle->delta += handle->beta * error;
    handle->delta /= PLL_DELTA_WINSIZE;
    handle->nco_freq += handle->delta;

    /* Detect whether the PLL is locked, and decrease the bandwidth if it is */
    if (!handle->locked && (handle->moving_average < handle->pll_locked)) {
        lrpt_demodulator_pll_recompute_coeffs(
                handle,
                handle->damping,
                handle->bw / PLL_LOCKED_BW_REDUCE);
        handle->locked = true;

        handle->avg_winsize = PLL_AVG_WINSIZE * PLL_LOCKED_WINSIZEX / (double)interp_factor;
        handle->avg_winsize_1 = handle->avg_winsize - 1.0;

        /* TODO we can report PLL lock here */
    }
    else if (handle->locked && (handle->moving_average > handle->pll_unlocked)) {
        lrpt_demodulator_pll_recompute_coeffs(
                handle,
                handle->damping,
                handle->bw);
        handle->locked = false;

        handle->avg_winsize = PLL_AVG_WINSIZE / (double)interp_factor;
        handle->avg_winsize_1 = handle->avg_winsize - 1.0;

        /* TODO we can report PLL unlock, framing loss (may be not) and zero signal quality here */
    }

    /* Limit frequency to a sensible range */
    if ((handle->nco_freq <= -FREQ_MAX) || (handle->nco_freq >= FREQ_MAX))
        handle->nco_freq = 0.0;
}

/*************************************************************************************************/

/** \endcond */
