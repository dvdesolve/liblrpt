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
 * Decoder routines.
 *
 * This source file contains routines for performing decoding of LRPT signals.
 */

/*************************************************************************************************/

#include "decoder.h"

#include "../../include/lrpt.h"
#include "../liblrpt/error.h"
#include "../liblrpt/lrpt.h"
#include "correlator.h"
#include "data.h"
#include "ecc.h"
#include "jpeg.h"
#include "huffman.h"
#include "packet.h"
#include "viterbi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

const size_t LRPT_DECODER_SOFT_FRAME_LEN = 16384;
const size_t LRPT_DECODER_HARD_FRAME_LEN = LRPT_DECODER_SOFT_FRAME_LEN / (2 * 8);

static const uint16_t DECODER_PACKET_BUF_LEN = 2048;

/*************************************************************************************************/

/* lrpt_decoder_init() */
lrpt_decoder_t *lrpt_decoder_init(
        lrpt_decoder_spacecraft_t sc,
        lrpt_error_t *err) {
    /* Allocate our working decoder */
    lrpt_decoder_t *decoder = malloc(sizeof(lrpt_decoder_t));

    if (!decoder) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Decoder object allocation failed");

        return NULL;
    }

    /* NULL-init internal objects and arrays for safe deallocation */
    decoder->corr = NULL;
    decoder->vit = NULL;
    decoder->huff = NULL;
    decoder->jpeg = NULL;

    decoder->aligned = NULL;
    decoder->decoded = NULL;
    decoder->ecced = NULL;
    decoder->ecc_buf = NULL;
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
    decoder->ecc_buf = calloc(ECC_BUF_LEN, sizeof(uint8_t)); /* ECC buffer */
    decoder->packet_buf = calloc(DECODER_PACKET_BUF_LEN, sizeof(uint8_t)); /* Packet buffer */

    /* Check for allocation problems */
    if (!decoder->corr || !decoder->vit || !decoder->huff || !decoder->jpeg ||
            !decoder->aligned || !decoder->decoded || !decoder->ecced || !decoder->ecc_buf ||
            !decoder->packet_buf) {
        lrpt_decoder_deinit(decoder);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Internal objects allocation for decoder object failed");

        return NULL;
    }

    /* Initialize internal state variables */
    decoder->pos = 0;

    decoder->corr_pos = 0;
    decoder->corr_word = 0;
    decoder->corr_val = 64;

    decoder->channel_image_size = 0;

    /* Each MCU is 8x8 block */
    switch (sc) {
        case LRPT_DECODER_SC_METEORM2:
            decoder->channel_image_width = (196 * 8);

            break;

        default:
            {
                lrpt_decoder_deinit(decoder);

                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                            "Spacecraft identifier is incorrect");

                return NULL;
            }

            break;
    }

    decoder->sc = sc;

    decoder->prev_len = 0;

    decoder->ok_cnt = 0;
    decoder->tot_cnt = 0;

    decoder->sig_q = 0;
    decoder->framing_ok = false;

    decoder->packet_off = 0;
    decoder->last_vcdu = 0;
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
    free(decoder->ecc_buf);
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

/* lrpt_decoder_exec() */
bool lrpt_decoder_exec(
        lrpt_decoder_t *decoder,
        lrpt_qpsk_data_t *input, /* TODO rename this to data */
        size_t buf_len,
        lrpt_error_t *err) { /* TODO may be use input data length and process it accordingly (check for 2 extra SFLs as in do_full_correlate(), e. g.) */
    /* Return immediately if no valid decoder or input was given */
    if (!decoder || !input) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Decoder object and/or QPSK data object are NULL");

        return false;
    }

    /* Go through data given */
    while (decoder->pos < buf_len) { /* TODO this may be redundant if we'll require SFL-multiples only blocks of data; also see below */
        if (lrpt_decoder_data_process_frame(decoder, input->qpsk)) {
            lrpt_decoder_packet_parse_cvcdu(decoder);

            decoder->ok_cnt++; /* TODO count frames, CVCDUs and packets separately */
            decoder->framing_ok = true;
        }
        else
            decoder->framing_ok = false;

        decoder->tot_cnt++;  /* TODO count frames, CVCDUs and packets separately */
    }

    /* TODO that should be optional and passed via parameter flag. Difference should depend on the number of data processed (may be not one frame at once) */
    decoder->pos -= LRPT_DECODER_SOFT_FRAME_LEN;

    return true;
}

/*************************************************************************************************/

/** \endcond */
