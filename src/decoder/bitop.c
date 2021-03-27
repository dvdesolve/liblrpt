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
 * Bit operation routines.
 *
 * This source file contains routines for performing bit operation routines.
 */

/*************************************************************************************************/

#include "bitop.h"

#include <stdint.h>

/*************************************************************************************************/

/** Number of bits in numbers in [0..255] range */
static const uint8_t BITOP_BITCNT_TBL[256] = {
    0, 1, 1, 2, 1, 2, 2, 3,
    1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7,
    5, 6, 6, 7, 6, 7, 7, 8
};

/*************************************************************************************************/

/** Return N-th byte in data array.
 *
 * \param l Bytes array.
 * \param n Index.
 *
 * \return Byte in position \p n inside array \p l.
 */
static inline uint8_t nth_byte(
        const uint8_t *l,
        int8_t n);

/*************************************************************************************************/

/* nth_byte() */
static inline uint8_t nth_byte(
        const uint8_t *l,
        int8_t n) {
    return *(l + n);
}

/*************************************************************************************************/

/* lrpt_decoder_bitop_count() */
uint8_t lrpt_decoder_bitop_count(
        uint32_t n) {
    return (BITOP_BITCNT_TBL[n & 0xFF] +
            BITOP_BITCNT_TBL[(n >> 8) & 0xFF] +
            BITOP_BITCNT_TBL[(n >> 16) & 0xFF] +
            BITOP_BITCNT_TBL[(n >> 24) & 0xFF]);
}

/*************************************************************************************************/

/* lrpt_decoder_bitop_writer_set() */
void lrpt_decoder_bitop_writer_set(
        lrpt_decoder_bitop_t *w,
        uint8_t *bytes) {
    w->p = bytes;

    w->pos = 0;
    w->cur = 0;
    w->cur_len = 0;
}

/*************************************************************************************************/

/* lrpt_decoder_bitop_writer_reverse() */
void lrpt_decoder_bitop_writer_reverse(
        lrpt_decoder_bitop_t *w,
        uint8_t *l,
        size_t len) {
    uint8_t *bytes = w->p;
    size_t byte_index = w->pos;
    uint16_t b;

    l += (len - 1);

    if (w->cur_len != 0) {
        uint8_t close_len = 8 - w->cur_len; /* close_len in [1; 8] range now */

        if (close_len >= len)
            close_len = len;

        b = w->cur;

        for (uint8_t i = 0; i < close_len; i++) {
            b |= l[0];
            b <<= 1;
            l -= 1;
        }

        len -= close_len;

        if ((w->cur_len + close_len) == 8) {
            b >>= 1;
            bytes[byte_index] = b;

            byte_index++;
        }
        else {
            w->cur = b;
            w->cur_len += close_len;

            return;
        }
    }

    const size_t full_bytes = len / 8;

    for (size_t i = 0; i < full_bytes; i++) {
        bytes[byte_index] = (uint8_t)(
                (nth_byte(l, 0) << 7) | (nth_byte(l, -1) << 6) |
                (nth_byte(l, -2) << 5) | (nth_byte(l, -3) << 4) |
                (nth_byte(l, -4) << 3) | (nth_byte(l, -5) << 2) |
                (nth_byte(l, -6) << 1) | nth_byte(l, -7));

        byte_index++;
        l -= 8;
    }

    len -= 8 * full_bytes; /* len in [0; 7] range now */

    b = 0;

    for (uint8_t i = 0; i < len; i++) {
        b |= l[0];
        b <<= 1;
        l--;
    }

    w->cur = b;
    w->pos = byte_index;
    w->cur_len = len; /* cur_len in [0; 7] range now */
}

/*************************************************************************************************/

/* lrpt_decoder_bitop_peek_n_bits() */
uint32_t lrpt_decoder_bitop_peek_n_bits(
        lrpt_decoder_bitop_t *b,
        uint8_t n) {
    uint32_t result = 0;

    for (uint8_t i = 0; i < n; i++) {
        const size_t p = (b->pos + i);
        const uint8_t bit = (b->p[p >> 3] >> (7 - (p & 0x07))) & 0x01;

        result = (result << 1) | bit;
    }

    return result;
}

/*************************************************************************************************/

/* lrpt_decoder_bitop_fetch_n_bits() */
uint32_t lrpt_decoder_bitop_fetch_n_bits(
        lrpt_decoder_bitop_t *b,
        uint8_t n) {
    uint32_t result = lrpt_decoder_bitop_peek_n_bits(b, n);
    b->pos += n;

    return result;
}

/*************************************************************************************************/

/** \endcond */
