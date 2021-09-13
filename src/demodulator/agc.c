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
 * Auto gain control routines.
 *
 * This source file contains routines for dealing with AGC operations.
 */

/*************************************************************************************************/

#include "agc.h"

#include <complex.h>
#include <stddef.h>
#include <stdlib.h>

/*************************************************************************************************/

/* Library defaults */
static const double AGC_WINSIZE = 65536.0; /* 1024 * 64 */
static const double AGC_WINSIZE_1 = AGC_WINSIZE - 1.0;

static const double AGC_BIAS_WINSIZE = 262144.0; /* 1024 * 256 */
static const double AGC_BIAS_WINSIZE_1 = AGC_BIAS_WINSIZE - 1.0;

const double AGC_MAX_GAIN = 20.0;

/*************************************************************************************************/

/* lrpt_demodulator_agc_init() */
lrpt_demodulator_agc_t *lrpt_demodulator_agc_init(
        double target) {
    /* Try to allocate our AGC */
    lrpt_demodulator_agc_t *agc = malloc(sizeof(lrpt_demodulator_agc_t));

    if (!agc)
        return NULL;

    /* Set default parameters */
    agc->target = target;
    agc->average = target;
    agc->gain = 1.0;
    agc->bias = 0.0;

    return agc;
}

/*************************************************************************************************/

/* lrpt_demodulator_agc_deinit() */
void lrpt_demodulator_agc_deinit(
        lrpt_demodulator_agc_t *agc) {
    free(agc);
}

/*************************************************************************************************/

/* lrpt_demodulator_agc_apply() */
complex double lrpt_demodulator_agc_apply(
        lrpt_demodulator_agc_t *agc,
        complex double sample) {
    /* Sliding window average */
    agc->bias *= AGC_BIAS_WINSIZE_1;
    agc->bias += sample;
    agc->bias /= AGC_BIAS_WINSIZE;
    sample -= agc->bias;

    /* Update the sample magnitude average */
    agc->average *= AGC_WINSIZE_1;
    agc->average += cabs(sample);
    agc->average /= AGC_WINSIZE;

    /* Apply AGC to the sample */
    agc->gain = agc->target / agc->average;

    if (agc->gain > AGC_MAX_GAIN)
        agc->gain = AGC_MAX_GAIN;

    return (sample * agc->gain);
}

/*************************************************************************************************/

/** \endcond */
