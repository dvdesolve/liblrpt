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
 * Public internal API for auto gain control routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DEMODULATOR_AGC_H
#define LRPT_DEMODULATOR_AGC_H

/*************************************************************************************************/

#include <complex.h>

/*************************************************************************************************/

extern const double AGC_MAX_GAIN;

/*************************************************************************************************/

/** AGC object */
typedef struct lrpt_demodulator_agc__ {
    double average; /**< Sliding window average */
    double gain; /**< Gain value */
    double target; /**< Target gain value */
    complex double bias; /**< Bias to apply */
} lrpt_demodulator_agc_t;

/*************************************************************************************************/

/** Allocates and initializes AGC object.
 *
 * \param target Target gain value.
 *
 * \return AGC object or \c NULL in case of error.
 */
lrpt_demodulator_agc_t *lrpt_demodulator_agc_init(
        double target);

/** Frees allocated AGC object.
 *
 * \param agc AGC object.
 */
void lrpt_demodulator_agc_deinit(
        lrpt_demodulator_agc_t *agc);

/** Applies gain to the sample.
 *
 * \param agc AGC object.
 * \param sample Input I/Q sample.
 *
 * \return Sample with gain applied.
 */
complex double lrpt_demodulator_agc_apply(
        lrpt_demodulator_agc_t *agc,
        complex double sample);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
