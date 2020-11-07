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
 * Public internal API for DSP routines.
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_DSP_H
#define LRPT_LIBLRPT_DSP_H

/*************************************************************************************************/

#include "../../include/lrpt.h"

#include <stdint.h>

/*************************************************************************************************/

/** DSP filter object */
struct lrpt_dsp_filter__ {
    double cutoff; /**< Cutoff frequency as a fraction of sample rate */
    double ripple; /**< Passband ripple as a percentage */

    uint8_t npoles; /**< Number of poles, must be even. Max value is limited to the 252. */
    lrpt_dsp_filter_type_t type; /**< Filter type */

    /** @{ */
    /** a and b coefficients of the filter */
    double *a, *b;
    /** @} */

    /** @{ */
    /** Saved input and output values for both I and Q samples */
    double *x_i, *y_i, *x_q, *y_q;
    /** @} */

    /** @{ */
    /** Ring buffer indices for both I and Q samples */
    uint8_t ri_i, ri_q;
    /** @} */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
