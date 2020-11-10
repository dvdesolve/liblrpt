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
 * Public internal API for demodulation routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DEMODULATOR_DEMOD_H
#define LRPT_DEMODULATOR_DEMOD_H

/*************************************************************************************************/

#include "agc.h"
#include "pll.h"
#include "rrc.h"

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Demodulator object */
struct lrpt_demodulator__ {
    lrpt_demodulator_agc_t *agc; /**< AGC object */
    lrpt_demodulator_pll_t *pll; /**< PLL object */
    lrpt_demodulator_rrc_filter_t *rrc; /**< RRC filter object */

    uint32_t sym_rate; /**< Symbol rate */
    double sym_period; /**< Symbol period */

    uint8_t interp_factor; /**< Interpolation factor */

    uint8_t *lut_isqrt; /**< Integer sqrt() lookup table */

    bool (*demod_func)( /**< Demodulator function */
            lrpt_demodulator_t *,
            complex double,
            int8_t *);

    /** @{ */
    /** Used by QPSK demodulator functions */
    double resync_offset;
    complex double before, middle;
    complex double inphase;
    double prev_I;
    size_t buf_idx;
    /** @} */

    /* TODO debug only */
    int8_t *out_buffer;
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
