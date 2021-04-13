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
 * Public internal API for Costas' phased-locked loop routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DEMODULATOR_PLL_H
#define LRPT_DEMODULATOR_PLL_H

/*************************************************************************************************/

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

/** PLL object */
typedef struct lrpt_demodulator_pll__ {
    /** @{ */
    /** Numerically-controlled oscillator parameters */
    double nco_phase, nco_freq;
    /** @} */

    /** @{ */
    /** Internal state parameters */
    double alpha, beta;
    double damping, bw;
    /** @} */

    /** @{ */
    /** Used for error managing */
    double moving_average;
    double err_scale;
    /** @} */

    bool locked; /**< Locked state indicator */

    double *lut_tanh; /**< Lookup table for tanh() */

    /** @{ */
    /** Used in phase error correction */
    double avg_winsize, avg_winsize_1, delta;
    /** @} */

    /** @{ */
    /* Used as thresholds for providing lock hysteresis */
    double pll_locked, pll_unlocked;
    /** @} */
} lrpt_demodulator_pll_t;

/*************************************************************************************************/

/** Allocates and initializes PLL object.
 *
 * \param bandwidth Costas' PLL bandwidth.
 * \param locked_threshold Locking threshold.
 * \param unlocked_threshold Unlocking threshold.
 * \param offset Offsetted QPSK modulation mode.
 *
 * \warning \p locked_threshold should be strictly lesser than \p unlocked_threshold!
 *
 * \return PLL object or \c NULL in case of error.
 */
lrpt_demodulator_pll_t *lrpt_demodulator_pll_init(
        double bandwidth,
        double locked_threshold,
        double unlocked_threshold,
        bool offset);

/** Frees previously allocated PLL object.
 *
 * \param pll PLL object.
 */
void lrpt_demodulator_pll_deinit(
        lrpt_demodulator_pll_t *pll);

/** Performs mixing of a sample with PLL NCO frequency.
 *
 * \param pll PLL object.
 * \param sample I/Q sample.
 *
 * \return I/Q sample mixed with NCO frequency.
 */
complex double lrpt_demodulator_pll_mix(
        lrpt_demodulator_pll_t *pll,
        complex double sample);

/** Computes the delta phase value to use when correcting the NCO frequency.
 *
 * \param pll PLL object.
 * \param sample I/Q sample.
 * \param cosample I/Q co-sample.
 *
 * \return Delta phase value.
 */
double lrpt_demodulator_pll_delta(
        const lrpt_demodulator_pll_t *pll,
        complex double sample,
        complex double cosample);

/** Corrects the phase angle of the Costas' PLL.
 *
 * \param pll PLL object.
 * \param error Error value.
 * \param interp_factor Interpolation factor.
 */
void lrpt_demodulator_pll_correct_phase(
        lrpt_demodulator_pll_t *pll,
        double error,
        uint8_t interp_factor);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
