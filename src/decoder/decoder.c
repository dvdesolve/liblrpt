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

const size_t DECODER_SOFT_FRAME_LEN = 16384;
const size_t DECODER_HARD_FRAME_LEN = DECODER_SOFT_FRAME_LEN / (2 * 8);

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

    /* Initialize internal objects */
    decoder->corr = lrpt_decoder_correlator_init(); /* Correlator */
    decoder->vit = lrpt_decoder_viterbi_init(); /* Viterbi decoder */
    decoder->huff = lrpt_decoder_huffman_init(); /* Huffman decoder */
    decoder->jpeg = lrpt_decoder_jpeg_init(); /* JPEG decoder */
    decoder->image = lrpt_image_alloc(0, 12000, NULL); /* LRPT image object */

    /* Allocate internal data arrays */
    decoder->aligned = calloc(DECODER_SOFT_FRAME_LEN, sizeof(int8_t)); /* Aligned data */
    decoder->decoded = calloc(DECODER_HARD_FRAME_LEN, sizeof(uint8_t)); /* Decoded data */
    decoder->ecced = calloc(DECODER_HARD_FRAME_LEN, sizeof(uint8_t)); /* ECCed data */
    decoder->ecc_buf = calloc(ECC_BUF_LEN, sizeof(uint8_t)); /* ECC buffer */
    decoder->packet_buf = calloc(DECODER_PACKET_BUF_LEN, sizeof(uint8_t)); /* Packet buffer */

    /* Check for allocation problems */
    if (!decoder->corr || !decoder->vit || !decoder->huff || !decoder->jpeg || !decoder->image ||
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

    if (!lrpt_image_set_width(decoder->image, decoder->channel_image_width, err)) {
        lrpt_decoder_deinit(decoder);

        return NULL;
    }

    decoder->sc = sc;

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


    free(decoder->packet_buf);
    free(decoder->ecc_buf);
    free(decoder->ecced);
    free(decoder->decoded);
    free(decoder->aligned);

    lrpt_image_free(decoder->image);
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
        const lrpt_qpsk_data_t *data,
        size_t *syms_proc,
        lrpt_error_t *err) {
    /* Return immediately if no valid decoder or input was given */
    if (!decoder || !data || (data->len < (3 * DECODER_SOFT_FRAME_LEN / 2))) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Decoder object and/or QPSK data object are NULL or QPSK data contains less then 3x soft frame length symbols");

        return false;
    }

    /* Count up SFLs */
    size_t n_sfls = 0;

    /* Go through data given keeping at least 2 SFLs for forward correlation */
    while (decoder->pos < (data->len * 2 - 2 * DECODER_SOFT_FRAME_LEN)) {
        if (lrpt_decoder_data_process_frame(decoder, data->qpsk)) {
            lrpt_decoder_packet_parse_cvcdu(decoder);

            decoder->ok_cnt++; /* TODO count frames, CVCDUs and packets separately */
            decoder->framing_ok = true;
        }
        else
            decoder->framing_ok = false;

        decoder->tot_cnt++;  /* TODO count frames, CVCDUs and packets separately */
        n_sfls++;
    }

    decoder->pos -=
        (n_sfls == 0) ? DECODER_SOFT_FRAME_LEN : (n_sfls * DECODER_SOFT_FRAME_LEN);

    /* Return number of processed QPSK symbols (1 QPSK symbol is 2 bits in lenth) */
    if (syms_proc)
        *syms_proc = (n_sfls * DECODER_SOFT_FRAME_LEN / 2);

    return true;
}

/*************************************************************************************************/

/* lrpt_decoder_sfl() */
size_t lrpt_decoder_sfl(void) {
    return DECODER_SOFT_FRAME_LEN;
}

/*************************************************************************************************/

/* lrpt_decoder_hfl() */
size_t lrpt_decoder_hfl(void) {
    return DECODER_HARD_FRAME_LEN;
}

/*************************************************************************************************/

/** \endcond */
