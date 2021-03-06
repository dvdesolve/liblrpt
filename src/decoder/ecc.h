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
 * Author: Artem Litvinovich
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Public internal API for error correction coding routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_ECC_H
#define LRPT_DECODER_ECC_H

/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

extern const uint8_t ECC_BUF_LEN; /**< ECC buffer length */

/*************************************************************************************************/

/** Perform ECC interleaving.
 *
 * \param input Input data.
 * \param output Resulting data.
 * \param pos Starting position.
 * \param n The length of data to be interleaved.
 */
void lrpt_decoder_ecc_interleave(
        const uint8_t *input,
        uint8_t *output,
        uint8_t pos,
        uint8_t n);

/** Perform ECC deinterleaving.
 *
 * \param input Input data.
 * \param output Resulting data.
 * \param pos Starting position.
 * \param n The length of data to be deinterleaved.
 */
void lrpt_decoder_ecc_deinterleave(
        const uint8_t *input,
        uint8_t *output,
        uint8_t pos,
        uint8_t n);

/** Perform ECC decoding.
 *
 * \param data Data to be decoded.
 * \param pad Padding.
 *
 * \return \c true on successfull decoding and \c false otherwise.
 */
bool lrpt_decoder_ecc_decode(
        uint8_t *data,
        uint8_t pad);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
