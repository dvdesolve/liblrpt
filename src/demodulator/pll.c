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
 * Author: Davide Belloli
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Costas phase-locked loop routines.
 *
 * This source file contains routines for Costas' PLL functionality.
 */

/*************************************************************************************************/

#include "pll.h"

#include "../../include/lrpt.h"

#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/*************************************************************************************************/

static const double PLL_INIT_FREQ = 0.001; /* Initial Costas' PLL frequency */
static const double PLL_DAMPING = 1.0 / M_SQRT2; /* Damping factor */

static const double PLL_ERR_SCALE_QPSK = 43.0; /* Scaling factors to control error magnitude */
static const double PLL_ERR_SCALE_OQPSK = 80.0;

static const double PLL_LOCKED_ERR_SCALE = 10.0; /* Phase error scale on lock */

static const double PLL_DELTA_WINSIZE = 100.0; /* Moving average window for phase errors */
static const double PLL_DELTA_WINSIZE_1 = PLL_DELTA_WINSIZE - 1.0;

static const double PLL_LOCKED_BW_REDUCE = 4.0; /* PLL bandwidth reduction (in locked state) */

static const double PLL_AVG_WINSIZE = 20000.0; /* Interpolation factor is taken into account now */
static const double PLL_LOCKED_WINSIZEX = 10.0; /* Error average window size multiplier (in lock) */

static const double PLL_FREQ_MAX = 0.8; /* Maximum frequency range of locked PLL */

static const uint16_t PLL_TANH_LUT_LEN = 256; /* Size of tanh() lookup table */

/*************************************************************************************************/

/** Clamps a double value to the range [-\p max; \p max].
 *
 * \param x Input value.
 * \param max Range limit for clamping.
 *
 * \return Value clamped to the range [-\p max; \p max].
 */
static inline double clamp_double(
        double x,
        double max);

/** Returns tanh() for given value.
 *
 * \param lut Initialized lookup table for tanh().
 * \param value Input value.
 *
 * \return tanh() value.
 */
static inline double lut_tanh(
        const double lut[],
        double value);

/** (Re)computes the alpha and beta coefficients of the Costas' PLL from damping and bandwidth
 * parameters and updates/sets them in the PLL object.
 *
 * \param pll PLL object.
 * \param damping Damping factor.
 * \param bandwidth Costas' PLL bandwidth.
 */
static void recompute_coeffs(
        lrpt_demodulator_pll_t *pll,
        double damping,
        double bandwidth);

/*************************************************************************************************/

/* clamp_double() */
static inline double clamp_double(
        double x,
        double max) {
    if (x > max)
        return max;
    else if (x < -max)
        return -max;
    else
        return x;
}

/*************************************************************************************************/

/* lut_tanh() */
static inline double lut_tanh(
        const double lut[],
        double value) {
    int16_t ival = value;

    if (ival > 127)
        return 1.0;
    else if (ival < -128)
        return -1.0;
    else
        return lut[ival + 128];
}

/*************************************************************************************************/

/* recompute_coeffs() */
static void recompute_coeffs(
        lrpt_demodulator_pll_t *pll,
        double damping,
        double bandwidth) {
    const double bw2 = bandwidth * bandwidth;
    const double denom = (1.0 + 2.0 * damping * bandwidth + bw2);

    pll->alpha = (4.0 * damping * bandwidth) / denom;
    pll->beta = (4.0 * bw2) / denom;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_init() */
lrpt_demodulator_pll_t *lrpt_demodulator_pll_init(
        double bandwidth,
        double locked_threshold,
        double unlocked_threshold,
        bool offset) {
    /* Try to allocate our PLL */
    lrpt_demodulator_pll_t *pll = malloc(sizeof(lrpt_demodulator_pll_t));

    if (!pll)
        return NULL;

    /* NULL-init internal storage for safe deallocation */
    pll->lut_tanh = NULL;

    /* Allocate lookup table for tanh() */
    pll->lut_tanh = calloc(PLL_TANH_LUT_LEN, sizeof(double));

    if (!pll->lut_tanh) {
        lrpt_demodulator_pll_deinit(pll);

        return NULL;
    }

    /* Populate lookup table for tanh() */
    for (uint16_t i = 0; i < PLL_TANH_LUT_LEN; i++)
        pll->lut_tanh[i] = tanh((int16_t)i - 128); /* Cast is needed to avoid promotion */

    /* Set default parameters */
    pll->nco_freq = PLL_INIT_FREQ;
    pll->nco_phase = 0.0;

    recompute_coeffs(pll, PLL_DAMPING, bandwidth);
    pll->damping = PLL_DAMPING;
    pll->bw = bandwidth;

    /* Set up thresholds for PLL hysteresis feature */
    if (
            (unlocked_threshold == 0.0) ||
            (locked_threshold == 0.0) ||
            (unlocked_threshold <= locked_threshold)) {
        lrpt_demodulator_pll_deinit(pll);

        return NULL;
    }

    pll->pll_locked = locked_threshold;
    pll->pll_unlocked = unlocked_threshold;

    pll->locked = false;

    /* Needed to cut off stray locks at startup */
    pll->moving_average = 1.0e6;

    /* Error scaling depends on modulation mode */
    pll->err_scale = (offset) ? PLL_ERR_SCALE_OQPSK : PLL_ERR_SCALE_QPSK;

    /* Initialize internal variables for phase correction routine */
    pll->avg_winsize = PLL_AVG_WINSIZE;
    pll->avg_winsize_1 = PLL_AVG_WINSIZE - 1.0;
    pll->delta = 0.0;

    return pll;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_deinit() */
void lrpt_demodulator_pll_deinit(
        lrpt_demodulator_pll_t *pll) {
    if (!pll)
        return;

    free(pll->lut_tanh);
    free(pll);
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_mix() */
complex double lrpt_demodulator_pll_mix(
        lrpt_demodulator_pll_t *pll,
        complex double sample) {
    const complex double nco_out = cexp(-I * pll->nco_phase);
    const complex double retval = sample * nco_out;

    pll->nco_phase += pll->nco_freq;
    pll->nco_phase = fmod(pll->nco_phase, 2 * M_PI);

    return retval;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_delta() */
double lrpt_demodulator_pll_delta(
        const lrpt_demodulator_pll_t *pll,
        complex double sample,
        complex double cosample) {
    return (
            (lut_tanh(pll->lut_tanh, creal(sample)) * cimag(sample)) -
            (lut_tanh(pll->lut_tanh, cimag(cosample)) * creal(cosample))) /
        pll->err_scale;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_correct_phase() */
void lrpt_demodulator_pll_correct_phase(
        lrpt_demodulator_pll_t *pll,
        double error,
        uint8_t interp_factor) {
    error = clamp_double(error, 1.0);

    pll->moving_average *= pll->avg_winsize_1;
    pll->moving_average += fabs(error);
    pll->moving_average /= pll->avg_winsize;

    pll->nco_phase += pll->alpha * error;
    pll->nco_phase = fmod(pll->nco_phase, 2 * M_PI);

    /* Calculate sliding window average of phase error */
    if (pll->locked)
        error /= PLL_LOCKED_ERR_SCALE;

    pll->delta *= PLL_DELTA_WINSIZE_1;
    pll->delta += pll->beta * error;
    pll->delta /= PLL_DELTA_WINSIZE;
    pll->nco_freq += pll->delta;

    /* Detect whether the PLL is locked, and decrease the bandwidth if it is */
    if (!pll->locked && (pll->moving_average < pll->pll_locked)) {
        recompute_coeffs(
                pll,
                pll->damping,
                pll->bw / PLL_LOCKED_BW_REDUCE);
        pll->locked = true;

        pll->avg_winsize = PLL_AVG_WINSIZE * PLL_LOCKED_WINSIZEX / interp_factor;
        pll->avg_winsize_1 = pll->avg_winsize - 1.0;
    }
    else if (pll->locked && (pll->moving_average > pll->pll_unlocked)) {
        recompute_coeffs(
                pll,
                pll->damping,
                pll->bw);
        pll->locked = false;

        pll->avg_winsize = PLL_AVG_WINSIZE / interp_factor;
        pll->avg_winsize_1 = pll->avg_winsize - 1.0;
    }

    /* Limit frequency to a sensible range */
    if ((pll->nco_freq <= -PLL_FREQ_MAX) || (pll->nco_freq >= PLL_FREQ_MAX))
        pll->nco_freq = 0.0;
}

/*************************************************************************************************/

/** \endcond */
