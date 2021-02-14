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
 * Data handling routines.
 *
 * This source file contains routines for performing data manipulation for LRPT signals.
 */

/*************************************************************************************************/

#include "data.h"

#include "../../include/lrpt.h"
#include "decoder.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*************************************************************************************************/

/** Fix packet.
 *
 * \param data Pointer to the aligned data.
 * \param len Length of the aligned data.
 * \param shift Correlator word.
 */
static void fix_packet(
        void *data,
        int len,
        uint32_t shift);

/* TODO may be we shouldn't pass aligned directly because it's already in decoder object */
/** Correlate next frame.
 *
 * \param decoder Pointer to the decoder object.
 * \param raw Raw data array.
 * \param aligned Aligned data array.
 */
static void do_next_correlate(
        lrpt_decoder_t *decoder,
        uint8_t *raw,
        uint8_t *aligned);

/*************************************************************************************************/

/* fix_packet() */
static void fix_packet(
        void *data, /* TODO use correct types */
        int len,
        uint32_t shift) {
    int8_t *d;
    int8_t b;

    d = (int8_t *)data; /* TODO strange cast */

    switch (shift) {
        case 4:
            for (size_t i = 0; i < (len / 2); i++) { /* TODO optimize here by len = len/2 */
                b = d[i * 2 + 0];
                d[i * 2 + 0] = d[i * 2 + 1];
                d[i * 2 + 1] = b;
            }

            break;

        case 5:
            for (size_t i = 0; i < (len / 2); i++)
                d[i * 2 + 0] = -d[i * 2 + 0];

            break;

        case 6:
            for (size_t i = 0; i < (len / 2); i++) {
                b = d[i * 2 + 0];
                d[i * 2 + 0] = -d[i * 2 + 1];
                d[i * 2 + 1] = -b;
            }

            break;

        case 7:
            for (size_t i = 0; i < len / 2; i++)
                d[i * 2 + 1] = -d[i * 2 + 1];

            break;
    }
}

/*************************************************************************************************/

/* do_next_correlate() */
static void do_next_correlate(
        lrpt_decoder_t *decoder,
        uint8_t *raw,
        uint8_t *aligned) {
    decoder->cpos = 0;
    memmove(aligned, &(raw[decoder->pos]), LRPT_DECODER_SOFT_FRAME_LEN);
    decoder->prev_pos = decoder->pos;
    decoder->pos += LRPT_DECODER_SOFT_FRAME_LEN;

    fix_packet(aligned, LRPT_DECODER_SOFT_FRAME_LEN, decoder->word);
}

/*************************************************************************************************/

static bool decode_frame(
        lrpt_decoder_t *decoder,
        uint8_t *aligned) {
    uint8_t ecc_buf[256];
    uint32_t temp;

    Vit_Decode(&(decoder->vit), aligned, decoder->decoded);

    temp =
        ((uint32_t)decoder->decoded[3] << 24) +
        ((uint32_t)decoder->decoded[2] << 16) +
        ((uint32_t)decoder->decoded[1] << 8) +
        (uint32_t)decoder->decoded[0];
    decoder->last_sync = temp;

    /* TODO use correct type */
    decoder->sig_q = (int)(round(100.0 - Vit_Get_Percent_BER(&(decoder->vit))));

    /* You can flip all bits in a packet and get a correct ECC anyway. Check for that case */
    if (Bitop_CountBits(decoder->last_sync ^ 0xE20330E5) <
            Bitop_CountBits(decoder->last_sync ^ 0x1DFCCF1A)) {
        for (size_t j = 0; j < LRPT_DECODER_HARD_FRAME_LEN; j++)
            decoder->decoded[j] ^= 0xFF;
        temp =
            ((uint32_t)decoder->decoded[3] << 24) +
            ((uint32_t)decoder->decoded[2] << 16) +
            ((uint32_t)decoder->decoded[1] <<  8) +
            (uint32_t)decoder->decoded[0];
        decoder->last_sync = temp;
    }

    for (size_t j = 0; j < (HARD_FRAME_LEN - 4); j++)
        decoder->decoded[4 + j] ^= prand[j % 255];

    for (size_t j = 0; j < 4; j++) {
        Ecc_Deinterleave( &(decoder->decoded[4]), ecc_buf, j, 4 );
        decoder->r[j] = Ecc_Decode( ecc_buf, 0 );
        Ecc_Interleave( ecc_buf, decoder->ecced_data, j, 4 );
    }

    return (decoder->r[0] && decoder->r[1] && decoder->r[2] && decoder->r[3]);
}

/*************************************************************************************************/

/* lrpt_decoder_data_process_frame() */
bool lrpt_decoder_data_process_frame(
        lrpt_decoder_t *decoder,
        uint8_t *data) {
    bool ok = false;

    if (decoder->cpos == 0) {
        do_next_correlate(decoder, data, decoder->aligned);
        ok = decode_frame(decoder, decoder->aligned);

        if (!ok)
            decoder->pos -= LRPT_DECODER_SOFT_FRAME_LEN;
    }

    if (!ok) {
        do_full_correlate(decoder, data, decoder->aligned);
        ok = decode_frame(decoder, decoder->aligned);
    }

    return ok;
}

/*************************************************************************************************/

/** \endcond */
