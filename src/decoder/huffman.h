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
    uint16_t run, size;
    size_t len;
    uint32_t mask, code;
} lrpt_decoder_huffman_acdata_t;

/** Huffman decoder object */
typedef struct lrpt_decoder_huffman__ {
    size_t ac_tbl_len; /**< AC table length */
    lrpt_decoder_huffman_acdata_t *ac_tbl; /**< AC table */
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
 * \param w Index.
 *
 * \return AC Huffman code.
 */
size_t lrpt_decoder_huffman_get_ac(
        uint16_t w);

/** Get DC Huffman code.
 *
 * \param w Index.
 *
 * \return DC Huffman code.
 */
size_t lrpt_decoder_huffman_get_dc(
        uint16_t w);

/** Perform mapping.
 *
 * \param cat Category.
 * \param vl Value.
 *
 * \return Mapped range value.
 */
int lrpt_decoder_huffman_map_range(
        int cat,
        uint32_t vl);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
