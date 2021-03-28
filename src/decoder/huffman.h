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
 * Public internal API for Huffman decoder routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_HUFFMAN_H
#define LRPT_DECODER_HUFFMAN_H

/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Decoder AC table data */
typedef struct lrpt_decoder_huffman_acdata__ {
    uint16_t run;
    uint8_t size;
    uint8_t len;
    uint16_t mask;
    uint32_t code;
} lrpt_decoder_huffman_acdata_t;

/** Huffman decoder object */
typedef struct lrpt_decoder_huffman__ {
    size_t ac_tbl_len; /**< AC table length */
    lrpt_decoder_huffman_acdata_t *ac_tbl; /**< AC table */

    int32_t *ac_lut; /**< AC lookup table */
    int32_t *dc_lut; /**< DC lookup table */
} lrpt_decoder_huffman_t;

/*************************************************************************************************/

/** Allocate and initialize Huffman decoder object.
 *
 * \return Pointer to the allocated Huffman decoder object or \c NULL in case of error.
 */
lrpt_decoder_huffman_t *lrpt_decoder_huffman_init(void);

/** Free previously allocated Huffman decoder.
 *
 * \param huff Pointer to the Huffman decoder object.
 */
void lrpt_decoder_huffman_deinit(lrpt_decoder_huffman_t *huff);

/** Get AC Huffman code.
 *
 * \param huff Pointer to the Huffman decoder object.
 * \param w Index.
 *
 * \return AC Huffman code.
 */
int32_t lrpt_decoder_huffman_get_ac(
        const lrpt_decoder_huffman_t *huff,
        uint16_t w);

/** Get DC Huffman code.
 *
 * \param huff Pointer to the Huffman decoder object.
 * \param w Index.
 *
 * \return DC Huffman code.
 */
int32_t lrpt_decoder_huffman_get_dc(
        const lrpt_decoder_huffman_t *huff,
        uint16_t w);

/** Perform mapping.
 *
 * \param cat Category.
 * \param val Value.
 *
 * \return Mapped range value.
 */
int32_t lrpt_decoder_huffman_map_range(
        uint8_t cat,
        uint16_t val);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
