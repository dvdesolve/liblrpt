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
 * Root raised cosine filtering routines.
 *
 * This source file contains routines for performing root raised cosine filtering.
 */

/*************************************************************************************************/

#include "rrc.h"

#include <complex.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/** Calculates RRC filter coefficients for variable alpha.
 *
 * \param index Filter index.
 * \param taps Number of filter taps.
 * \param osf Ratio of sampling rate and symbol rate.
 * \param alpha Filter alpha factor.
 *
 * \return Filter coefficient.
 *
 * \note Adapted from https://www.michael-joost.de/rrcfilter.pdf
 */
static double rrc_coeff(
        uint16_t index,
        uint16_t taps,
        double osf,
        double alpha);

/*************************************************************************************************/

/* lrpt_demodulator_rrc_coeff() */
static double rrc_coeff(
        uint16_t index,
        uint16_t taps,
        double osf,
        double alpha) {
    const uint16_t order = (taps - 1) / 2;

    /* Handle the 0/0 case */
    if (order == index)
        return (1.0 - alpha + 4.0 * alpha / M_PI);

    const double t = (double)(abs(order - index)) / osf;
    const double mpt = M_PI * t;
    const double at4 = 4.0 * alpha * t;
    const double coeff = sin(mpt * (1.0 - alpha)) + at4 * cos(mpt * (1.0 + alpha));
    const double interm = mpt * ( 1.0 - at4 * at4 );

    return (coeff / interm);
}

/*************************************************************************************************/

/* lrpt_demodulator_rrc_filter_init() */
lrpt_demodulator_rrc_filter_t *lrpt_demodulator_rrc_filter_init(
        uint16_t order,
        uint8_t factor,
        double osf,
        double alpha) {
    /* Try to allocate our RRC */
    lrpt_demodulator_rrc_filter_t *rrc = malloc(sizeof(lrpt_demodulator_rrc_filter_t));

    if (!rrc)
        return NULL;

    /* NULL-init internal storage for safe deallocation */
    rrc->coeffs = NULL;
    rrc->memory = NULL;

    /* Try to allocate storage for coefficients and memory */
    const uint16_t taps = order * 2 + 1;

    rrc->count = taps;
    rrc->coeffs = calloc((size_t)taps, sizeof(double));
    rrc->idm = 0;
    rrc->memory = calloc((size_t)taps, sizeof(complex double));

    if (!rrc->coeffs || !rrc->memory) {
        lrpt_demodulator_rrc_filter_deinit(rrc);

        return NULL;
    }

    /* Compute filter coefficients */
    for (size_t i = 0; i < taps; i++)
        rrc->coeffs[i] = rrc_coeff((uint16_t)i, taps, osf * (double)factor, alpha);

    return rrc;
}

/*************************************************************************************************/

/* lrpt_demodulator_rrc_filter_deinit() */
void lrpt_demodulator_rrc_filter_deinit(
        lrpt_demodulator_rrc_filter_t *rrc) {
    if (!rrc)
        return;

    free(rrc->coeffs);
    free(rrc->memory);
    free(rrc);
}

/*************************************************************************************************/

/* lrpt_demodulator_rrc_filter_apply() */
complex double lrpt_demodulator_rrc_filter_apply(
        lrpt_demodulator_rrc_filter_t *rrc,
        complex double value) {
    /* Update the memory nodes, save input value to first node */
    rrc->memory[rrc->idm] = value;

    /* Calculate the feed-forward output */
    complex double result = 0.0;
    uint16_t idc = 0; /* Index for coefficients */

    /* Summate nodes till the end of the ring buffer */
    while (rrc->idm < (int16_t)rrc->count)
        result += rrc->memory[(rrc->idm)++] * rrc->coeffs[idc++];

    /* Go back to the beginning of the ring buffer and summate remaining nodes */
    rrc->idm = 0;

    while (idc < rrc->count)
        result += rrc->memory[(rrc->idm)++] * rrc->coeffs[idc++];

    /* Move back in the ring buffer */
    (rrc->idm)--;

    if (rrc->idm < 0)
        rrc->idm += rrc->count;

    return result;
}

/*************************************************************************************************/

/** \endcond */
