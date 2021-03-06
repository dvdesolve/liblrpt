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
#include <time.h>

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

    /* TODO recheck types */
    /** @{ */
    /** Position information */
    size_t pos, prev_pos;
    /** @} */

    /** @{ */
    /** Needed for correlator calls */
    uint32_t cpos, word, corrv;
    /** @} */

    /* TODO may be use macro/static const */
    uint8_t *channel_image[3]; /**< Per-channel image representation */

    /** @{ */
    /** Channel image dimensions ang length tracker */
    size_t channel_image_size, channel_image_width;
    size_t prev_len;
    /** @} */

    /** @{ */
    /** Packet counter */
    size_t ok_cnt, total_cnt;
    /** @} */

    uint8_t *aligned; /**< Correlated data */
    uint8_t *decoded; /**< Decoded data */

    /** @{ */
    /** Needed for data processor */
    uint32_t last_sync;
    uint8_t sig_q;
    bool r[4];
    uint8_t *ecced_data;
    /** @} */

    uint8_t *packet_buf; /**< Packet buffer */
    size_t packet_off; /**< Packet offset */
    uint32_t last_frame; /**< Last frame number */
    bool packet_part; /**< Is packet partial? */

    struct tm onboard_time;
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
