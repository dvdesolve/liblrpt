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
#include "jpeg.h"
#include "huffman.h"
#include "viterbi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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

    /* TODO may be use macro/static const */
    for (size_t i = 0; i < 3; i++)
        decoder->channel_image[i] = NULL;

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

    /* Initialize internal state variables */
    decoder->pos = 0;
    decoder->cpos = 0;
    decoder->word = 0;
    decoder->corrv = 64;

    /* TODO use static const */
    /** Set initial image dimensions */
    decoder->channel_image_size = 0;
    decoder->channel_image_width = 1568;

    decoder->ok_cnt = 0;
    decoder->total_cnt = 0;

    return decoder;
}

/*************************************************************************************************/

/* lrpt_decoder_deinit() */
void lrpt_decoder_deinit(
        lrpt_decoder_t *decoder) {
    if (!decoder)
        return;

    lrpt_decoder_jpeg_deinit(decoder->jpeg);
    lrpt_decoder_huffman_deinit(decoder->huff);
    lrpt_decoder_viterbi_deinit(decoder->vit);
    lrpt_decoder_correlator_deinit(decoder->corr);
    free(decoder);
}

/*************************************************************************************************/

//void lrpt_decoder_exec(
//        lrpt_decoder_t *decoder,
//        uint8_t *in_buffer,
//        size_t buf_len) {
//    /* Go through data given */
//    while (decoder->pos < buf_len) {
//        bool ok = Mtd_One_Frame(decoder, in_buffer);
//
//        if (ok) {
//            Parse_Cvcdu(decoder->ecced_data, HARD_FRAME_LEN - 132);
//
//            /* TODO increase total number of successfully decoded packets */
//            //ok_cnt++;
//
//            /* TODO we can report Framing OK here */
//        }
//        else {
//            /* TODO we can report Framing non-OK here */
//        }
//
//        /* TODO increase total number of received packets */
//        //tal_cnt++;
//    }
//
//    /* TODO we can report here:
//     * signal quality (sig_q)
//     * total number of received packets (tot_cnt)
//     * percent of successfully decoded packets (ok_cnt / tot_cnt * 100)
//     */
//}

/*************************************************************************************************/

/** \endcond */
