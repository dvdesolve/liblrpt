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
#include "../liblrpt/lrpt.h"
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

/* Library defaults */

/** Length of soft frame in bits (produced by convolutional encoder) */
const size_t LRPT_DECODER_SOFT_FRAME_LEN = 16384;

/** Length of hard frame in bytes (produced after Viterbi decoding, r = 1/2) */
const size_t LRPT_DECODER_HARD_FRAME_LEN = 1024;

/*************************************************************************************************/

/* lrpt_decoder_init() */
lrpt_decoder_t *lrpt_decoder_init(
        uint16_t mcus_per_line) {
    /* Allocate our working decoder */
    lrpt_decoder_t *decoder = malloc(sizeof(lrpt_decoder_t));

    if (!decoder)
        return NULL;

    /* NULL-init internal objects and arrays for safe deallocation */
    decoder->corr = NULL;
    decoder->vit = NULL;
    decoder->huff = NULL;
    decoder->jpeg = NULL;

    decoder->aligned = NULL;
    decoder->decoded = NULL;
    decoder->ecced = NULL;
    decoder->packet_buf = NULL;

    for (uint8_t i = 0; i < 6; i++)
        decoder->channel_image[i] = NULL; /* (Re)allocation will happen during progressing */

    /* Initialize internal objects */
    decoder->corr = lrpt_decoder_correlator_init(); /* Correlator */
    decoder->vit = lrpt_decoder_viterbi_init(); /* Viterbi decoder */
    decoder->huff = lrpt_decoder_huffman_init(); /* Huffman decoder */
    decoder->jpeg = lrpt_decoder_jpeg_init(); /* JPEG decoder */

    /* Allocate internal data arrays */
    decoder->aligned = calloc(LRPT_DECODER_SOFT_FRAME_LEN, sizeof(int8_t)); /* Aligned data */
    decoder->decoded = calloc(LRPT_DECODER_HARD_FRAME_LEN, sizeof(uint8_t)); /* Decoded data */
    decoder->ecced = calloc(LRPT_DECODER_HARD_FRAME_LEN, sizeof(uint8_t)); /* ECCed data */
    decoder->packet_buf = calloc(2048, sizeof(uint8_t)); /* Packet buffer */

    /* Check for allocation problems */
    if (!decoder->corr || !decoder->vit || !decoder->huff || !decoder->jpeg ||
            !decoder->aligned || !decoder->decoded || !decoder->ecced || !decoder->packet_buf) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    /* Initialize internal state variables */
    decoder->pos = 0;
    decoder->prev_pos = 0;

    decoder->corr_pos = 0;
    decoder->corr_word = 0;
    decoder->corr_val = 64;

    decoder->channel_image_size = 0;
    decoder->channel_image_width = mcus_per_line * 8; /* Each MCU is 8x8 block */
    decoder->prev_len = 0;

    decoder->ok_cnt = 0;
    decoder->tot_cnt = 0;

    decoder->sig_q = 0;

    decoder->packet_off = 0;
    decoder->last_frame = 0;
    decoder->packet_part = false;

    return decoder;
}

/*************************************************************************************************/

/* lrpt_decoder_deinit() */
void lrpt_decoder_deinit(
        lrpt_decoder_t *decoder) {
    if (!decoder)
        return;

    for (uint8_t i = 0; i < 6; i++)
        free(decoder->channel_image[i]);

    free(decoder->packet_buf);
    free(decoder->ecced);
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
        lrpt_qpsk_data_t *input,
        size_t buf_len) {
    /* Return immediately if no valid input was given */
    if (!input)
        return;

    /* Go through data given */
    while (decoder->pos < buf_len) {
        if (lrpt_decoder_data_process_frame(decoder, input->qpsk)) {
            lrpt_decoder_packet_parse_cvcdu(decoder, LRPT_DECODER_HARD_FRAME_LEN - 132);

            decoder->ok_cnt++;

            /* TODO we can report Framing OK here */
        }
        else {
            /* TODO we can report Framing non-OK here */
        }

        decoder->tot_cnt++; /* TODO this relies upon 16384-long blocks in input, should be changed */
    }

    /* TODO we can report here:
     * signal quality (sig_q)
     * total number of received packets (tot_cnt)
     * percent of successfully decoded packets (ok_cnt / tot_cnt * 100), but flag for handling 0/0 is needed
     */
}

/*************************************************************************************************/

/** \endcond */
