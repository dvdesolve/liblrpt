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
 * Public internal API for decoder routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_DECODER_H
#define LRPT_DECODER_DECODER_H

/*************************************************************************************************/

#include "../../include/lrpt.h"
#include "correlator.h"
#include "jpeg.h"
#include "huffman.h"
#include "viterbi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Length of soft frame in bits (produced by convolutional encoder, r = 1/2) */
extern const size_t DECODER_SOFT_FRAME_LEN;

/** Length of hard frame in bytes (produced by Viterbi decoder, rate = 2) */
extern const size_t DECODER_HARD_FRAME_LEN;

/*************************************************************************************************/

/** Decoder object */
struct lrpt_decoder__ {
    lrpt_decoder_spacecraft_t sc; /**< Spacecraft */

    lrpt_decoder_correlator_t *corr; /**< Correlator */
    lrpt_decoder_viterbi_t *vit; /**< Viterbi decoder */
    lrpt_decoder_huffman_t *huff; /**< Huffman decoder */
    lrpt_decoder_jpeg_t *jpeg; /**< JPEG decoder */

    int8_t *aligned; /**< Aligned data after correlation */
    uint8_t *decoded; /**< Decoded data */
    uint8_t *ecced; /**< ECCed data */
    uint8_t *ecc_buf; /**< Buffer for ECC processing */

    size_t pos; /**< Decoder position */

    /** @{ */
    /** Correlator state variables (for shortcut access) */
    uint16_t corr_val;
    size_t corr_pos;
    uint8_t corr_word;
    /** @} */

    lrpt_image_t *image; /**< Per-channel image representation for all possible APIDs (64-69) */
    size_t pxls_count[6]; /**< Current pixels count for each APID */

    /** @{ */
    /** Image dimensions */
    size_t channel_image_width, channel_image_height;
    /** @} */

    size_t onboard_time; /**< Onboard time in msec (Meteor-M2 series only) */

    /** @{ */
    /** Frame counters (only for stats) */
    size_t frm_ok_cnt, frm_tot_cnt;
    /** @} */

    size_t cvcdu_cnt; /**< CVCDU counter */
    size_t pck_cnt; /**< Packet counter */

    /** @{ */
    /** Used by data link layer processor */
    uint32_t last_sync;
    uint8_t sig_q;
    bool r[4];
    bool framing_ok;
    /** @} */

    /** @{ */
    /** Used by packet link layer processor */
    uint8_t *packet_buf;
    uint16_t packet_off;
    uint32_t last_vcdu;
    bool packet_part;
    /** @} */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
