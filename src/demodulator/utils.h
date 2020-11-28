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
 * Public internal API for different utils and routines for QPSK demodulator.
 */

/*************************************************************************************************/

#ifndef LRPT_DEMODULATOR_UTILS_H
#define LRPT_DEMODULATOR_UTILS_H

/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

/** Allocates and initializes lookup table for integer square roots.
 *
 * \return Pointer to the allocated lookup table or \c NULL otherwise.
 */
uint8_t *lrpt_demodulator_lut_isqrt_init(void);

/** Frees previously allocated lookup table for integer square roots.
 *
 * \param lut Lookup table for integer square roots.
 */
void lrpt_demodulator_lut_isqrt_deinit(
        uint8_t *lut);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
