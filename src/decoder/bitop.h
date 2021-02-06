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
 * Public internal API for bit operations.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_BITOP_H
#define LRPT_DECODER_BITOP_H

/*************************************************************************************************/

#include <stdint.h>

/*************************************************************************************************/

/** Get number of bits in number.
 *
 * \param n Number to count bits for.
 *
 * \return Number of bits in number \p n.
 */
uint8_t lrpt_decoder_bitop_count(
        uint32_t n);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
