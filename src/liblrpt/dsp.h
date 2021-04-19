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
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
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

#include <complex.h>
#include <stdint.h>

/*************************************************************************************************/

/** DSP filter object */
struct lrpt_dsp_filter__ {
    uint8_t npoles; /**< Number of poles, must be even. Max value is limited to the 252. */

    /** @{ */
    /** a and b coefficients of the filter */
    double *a;
    double *b;
    /** @} */

    /** @{ */
    /** Saved input and output values for I/Q samples */
    complex double *x;
    complex double *y;
    /** @} */

    /** @{ */
    /** Ring buffer index for I/Q samples */
    uint8_t ri;
    /** @} */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
