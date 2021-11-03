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
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Public internal API for performing integer FFT.
 */

/*************************************************************************************************/

#ifndef LRPT_DSP_IFFT_H
#define LRPT_DSP_IFFT_H

/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Integer FFT object */
struct lrpt_dsp_ifft__ {
    uint16_t width; /**< FFT width */
    uint8_t order; /**< FFT order */

    size_t len; /**< FFT data length */
    int16_t *sw_lut; /**< Integer sinewave lookup table */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
