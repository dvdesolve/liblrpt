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
 * Decoder routines.
 *
 * This source file contains routines for performing decoding of LRPT signals.
 */

/*************************************************************************************************/

#include "decoder.h"

#include "../../include/lrpt.h"
#include "correlator.h"
#include "data.h"
#include "jpeg.h"
#include "huffman.h"
#include "packet.h"
#include "viterbi.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/** Length of soft frame */
const size_t LRPT_DECODER_SOFT_FRAME_LEN = 16384;

/** Length of hard frame */
const size_t LRPT_DECODER_HARD_FRAME_LEN = 1024;

/*************************************************************************************************/

/* lrpt_decoder_init() */
lrpt_decoder_t *lrpt_decoder_init(void) {
    /* Allocate our working decoder */
    lrpt_decoder_t *decoder = malloc(sizeof(lrpt_decoder_t));

    if (!decoder)
        return NULL;

    /* NULL-init internal objects for safe deallocation */
    decoder->corr = NULL;
    decoder->vit = NULL;
    decoder->huff = NULL;
    decoder->jpeg = NULL;
    decoder->aligned = NULL;
    decoder->decoded = NULL;
    decoder->ecced_data = NULL;
    decoder->packet_buf = NULL;

    /* TODO may be use macro/static const */
    for (size_t i = 0; i < 3; i++)
        decoder->channel_image[i] = NULL;

    /* TODO may be use batch check as in DSP, Viterbi... */
    /* Initialize correlator */
    decoder->corr = lrpt_decoder_correlator_init();

    if (!decoder->corr) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Initialize Viterbi decoder */
    decoder->vit = lrpt_decoder_viterbi_init();

    if (!decoder->vit) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Initialize Huffman decoder */
    decoder->huff = lrpt_decoder_huffman_init();

    if (!decoder->huff) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Initialize JPEG decoder */
    decoder->jpeg = lrpt_decoder_jpeg_init();

    if (!decoder->jpeg) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Allocate aligned data array */
    decoder->aligned = calloc(LRPT_DECODER_SOFT_FRAME_LEN, sizeof(uint8_t));

    if (!decoder->aligned) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Allocate decoded data array */
    decoder->decoded = calloc(LRPT_DECODER_HARD_FRAME_LEN, sizeof(uint8_t));

    if (!decoder->decoded) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Allocate array for ECCed data */
    decoder->ecced_data = calloc(LRPT_DECODER_HARD_FRAME_LEN, sizeof(uint8_t));

    if (!decoder->ecced_data) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Allocate packet buffer */
    decoder->packet_buf = calloc(2048, sizeof(uint8_t));

    if (!decoder->packet_buf) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Initialize internal state variables */
    decoder->pos = 0;
    decoder->cpos = 0;
    decoder->word = 0;
    decoder->corrv = 64;
    decoder->packet_off = 0;
    decoder->last_frame = 0;
    decoder->packet_part = false;
    decoder->ok_cnt = 0;
    decoder->total_cnt = 0;

    /* TODO use static const if possible */
    /** Set initial image dimensions */
    decoder->channel_image_size = 0;
    decoder->channel_image_width = 1568; /* TODO use named constant here. METEOR_IMAGE_WIDTH = MCU_PER_LINE * 8; MCU_PER_LINE = 196; may depend on satellite capabilities */
    decoder->prev_len = 0;

    return decoder;
}

/*************************************************************************************************/

/* lrpt_decoder_deinit() */
void lrpt_decoder_deinit(
        lrpt_decoder_t *decoder) {
    if (!decoder)
        return;

    free(decoder->packet_buf);
    free(decoder->ecced_data);
    free(decoder->decoded);
    free(decoder->aligned);
    lrpt_decoder_jpeg_deinit(decoder->jpeg);
    lrpt_decoder_huffman_deinit(decoder->huff);
    lrpt_decoder_viterbi_deinit(decoder->vit);
    lrpt_decoder_correlator_deinit(decoder->corr);
    free(decoder);
}

/*************************************************************************************************/

/* TODO return bool, report framing, write to given data buffer(s) */
/* lrpt_decoder_exec() */
void lrpt_decoder_exec(
        lrpt_decoder_t *decoder,
        uint8_t *in_buffer,
        size_t buf_len) {
    /* Go through data given */
    while (decoder->pos < buf_len) {
        if (lrpt_decoder_data_process_frame(decoder, in_buffer)) {
            lrpt_decoder_packet_parse_cvcdu(decoder, (LRPT_DECODER_HARD_FRAME_LEN - 132));

            /* TODO increase total number of successfully decoded packets */
            //ok_cnt++;

            /* TODO we can report Framing OK here */
        }
        else {
            /* TODO we can report Framing non-OK here */
        }

        /* TODO increase total number of received packets */
        //tal_cnt++;
    }

    /* TODO we can report here:
     * signal quality (sig_q)
     * total number of received packets (tot_cnt)
     * percent of successfully decoded packets (ok_cnt / tot_cnt * 100)
     */
}

/*************************************************************************************************/

/** \endcond */
