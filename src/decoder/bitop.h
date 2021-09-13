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
 * Public internal API for bit operations.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_BITOP_H
#define LRPT_DECODER_BITOP_H

/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Bit I/O object */
typedef struct lrpt_decoder_bitop__ {
    uint8_t *p; /**< Data */

    size_t pos; /**< Position */
    uint8_t cur; /**< Data at current position */
    uint8_t cur_len; /**< Current length */
} lrpt_decoder_bitop_t;

/*************************************************************************************************/

/** Get number of bits in number.
 *
 * \param n Number to count bits for.
 *
 * \return Number of bits in number \p n.
 */
uint8_t lrpt_decoder_bitop_count(
        uint32_t n);

/** Set initial state for bit writer.
 *
 * \param w Pointer to the bit I/O object.
 * \param bytes Pointer to the data array.
 */
void lrpt_decoder_bitop_writer_set(
        lrpt_decoder_bitop_t *w,
        uint8_t *bytes);

/** Reverse bit list order.
 *
 * \param w Pointer to the bit writer object.
 * \param l Array of bytes to reverse bits in.
 * \param len Length of given bytes array.
 */
void lrpt_decoder_bitop_writer_reverse(
        lrpt_decoder_bitop_t *w,
        uint8_t *l,
        size_t len);

/** Peek \p n bits from bit I/O object.
 *
 * \param b Pointer to the bit I/O object.
 * \param n Number of bits to peek.
 *
 * \return Specified bits number as 4-byte integer.
 */
uint32_t lrpt_decoder_bitop_peek_n_bits(
        lrpt_decoder_bitop_t *b,
        uint8_t n);

/** Fetch \p n bits from bit I/O object.
 *
 * \param b Pointer to the bit I/O object.
 * \param n Number of bits to fetch.
 *
 * \return Specified bits number as 4-bytes integer.
 */
uint32_t lrpt_decoder_bitop_pop_n_bits(
        lrpt_decoder_bitop_t *b,
        uint8_t n);

/** Advance bit I/O object by \p n bits.
 *
 * \param b Pointer to the bit I/O object.
 * \param n Number of bits to advance bit I/O object to.
 */
void lrpt_decoder_bitop_advance_n_bits(
        lrpt_decoder_bitop_t *b,
        uint8_t n);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
