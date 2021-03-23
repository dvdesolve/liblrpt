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
 * Public internal API for decoder routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_DECODER_H
#define LRPT_DECODER_DECODER_H

/*************************************************************************************************/

#include "correlator.h"
#include "jpeg.h"
#include "huffman.h"
#include "viterbi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

extern const size_t LRPT_DECODER_SOFT_FRAME_LEN;
extern const size_t LRPT_DECODER_HARD_FRAME_LEN;

/*************************************************************************************************/

/** Decoder object */
struct lrpt_decoder__ {
    lrpt_decoder_correlator_t *corr; /**< Correlator */
    lrpt_decoder_viterbi_t *vit; /**< Viterbi decoder */
    lrpt_decoder_huffman_t *huff; /**< Huffman decoder */
    lrpt_decoder_jpeg_t *jpeg; /**< JPEG decoder */

    int8_t *aligned; /**< Aligned data after correlation */
    uint8_t *decoded; /**< Decoded data */
    uint8_t *ecced; /**< ECCed data */
    uint8_t *ecc_buf; /**< Buffer for ECC processing */

    size_t pos; /**< Position information */

    /** @{ */
    /** Correlator state variables (for shortcut) */
    uint16_t corr_val;
    size_t corr_pos;
    uint8_t corr_word;
    /** @} */

    /* TODO we should use special data type here because of future image manipulation */
    uint8_t *channel_image[6]; /**< Per-channel image representation for all six APIDs (64-69) */

    /** @{ */
    /** Image dimensions */
    size_t channel_image_size, channel_image_width;
    size_t prev_len;
    /** @} */

    /** @{ */
    /** Packet counters (only for stats) */
    size_t ok_cnt, tot_cnt;
    /** @} */

    /** @{ */
    /** Used by data link layer processor */
    uint32_t last_sync;
    uint8_t sig_q;
    bool r[4];
    /** @} */

    /** @{ */
    /** Used by packet link layer processor */
    uint8_t *packet_buf;
    size_t packet_off;
    uint32_t last_frame;
    bool packet_part;
    /** @} */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
