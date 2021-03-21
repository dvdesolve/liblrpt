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
#include "ecc.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*************************************************************************************************/

/* TODO randomization polynomial */
static const uint8_t DECODER_PRAND_TBL[255] = {
    0xFF, 0x48, 0x0E, 0xC0, 0x9A, 0x0D, 0x70, 0xBC,
    0x8E, 0x2C, 0x93, 0xAD, 0xA7, 0xB7, 0x46, 0xCE,
    0x5A, 0x97, 0x7D, 0xCC, 0x32, 0xA2, 0xBF, 0x3E,
    0x0A, 0x10, 0xF1, 0x88, 0x94, 0xCD, 0xEA, 0xB1,
    0xFE, 0x90, 0x1D, 0x81, 0x34, 0x1A, 0xE1, 0x79,
    0x1C, 0x59, 0x27, 0x5B, 0x4F, 0x6E, 0x8D, 0x9C,
    0xB5, 0x2E, 0xFB, 0x98, 0x65, 0x45, 0x7E, 0x7C,
    0x14, 0x21, 0xE3, 0x11, 0x29, 0x9B, 0xD5, 0x63,
    0xFD, 0x20, 0x3B, 0x02, 0x68, 0x35, 0xC2, 0xF2,
    0x38, 0xB2, 0x4E, 0xB6, 0x9E, 0xDD, 0x1B, 0x39,
    0x6A, 0x5D, 0xF7, 0x30, 0xCA, 0x8A, 0xFC, 0xF8,
    0x28, 0x43, 0xC6, 0x22, 0x53, 0x37, 0xAA, 0xC7,
    0xFA, 0x40, 0x76, 0x04, 0xD0, 0x6B, 0x85, 0xE4,
    0x71, 0x64, 0x9D, 0x6D, 0x3D, 0xBA, 0x36, 0x72,
    0xD4, 0xBB, 0xEE, 0x61, 0x95, 0x15, 0xF9, 0xF0,
    0x50, 0x87, 0x8C, 0x44, 0xA6, 0x6F, 0x55, 0x8F,
    0xF4, 0x80, 0xEC, 0x09, 0xA0, 0xD7, 0x0B, 0xC8,
    0xE2, 0xC9, 0x3A, 0xDA, 0x7B, 0x74, 0x6C, 0xE5,
    0xA9, 0x77, 0xDC, 0xC3, 0x2A, 0x2B, 0xF3, 0xE0,
    0xA1, 0x0F, 0x18, 0x89, 0x4C, 0xDE, 0xAB, 0x1F,
    0xE9, 0x01, 0xD8, 0x13, 0x41, 0xAE, 0x17, 0x91,
    0xC5, 0x92, 0x75, 0xB4, 0xF6, 0xE8, 0xD9, 0xCB,
    0x52, 0xEF, 0xB9, 0x86, 0x54, 0x57, 0xE7, 0xC1,
    0x42, 0x1E, 0x31, 0x12, 0x99, 0xBD, 0x56, 0x3F,
    0xD2, 0x03, 0xB0, 0x26, 0x83, 0x5C, 0x2F, 0x23,
    0x8B, 0x24, 0xEB, 0x69, 0xED, 0xD1, 0xB3, 0x96,
    0xA5, 0xDF, 0x73, 0x0C, 0xA8, 0xAF, 0xCF, 0x82,
    0x84, 0x3C, 0x62, 0x25, 0x33, 0x7A, 0xAC, 0x7F,
    0xA4, 0x07, 0x60, 0x4D, 0x06, 0xB8, 0x5E, 0x47,
    0x16, 0x49, 0xD6, 0xD3, 0xDB, 0xA3, 0x67, 0x2D,
    0x4B, 0xBE, 0xE6, 0x19, 0x51, 0x5F, 0x9F, 0x05,
    0x08, 0x78, 0xC4, 0x4A, 0x66, 0xF5, 0x58
};

static const uint16_t DECODER_CORRELATION_MIN = 45; /**< Threshold for correlation */

/*************************************************************************************************/

/** Fix packet.
 *
 * \param data Pointer to the aligned data.
 * \param len Length of the aligned data.
 * \param shift Correlator word.
 */
static void fix_packet(
        int8_t *data,
        size_t len,
        uint8_t shift);

/** Correlate next frame.
 *
 * \param decoder Pointer to the decoder object.
 * \param raw Raw data array.
 */
static void do_next_correlate(
        lrpt_decoder_t *decoder,
        int8_t *data);

/** Perform full correlation.
 *
 * \param decoder Pointer to the decoder object.
 * \param raw Raw data array.
 */
static void do_full_correlate(
        lrpt_decoder_t *decoder,
        int8_t *data);

/** Decode frame.
 *
 * \param decoder Pointer to the decoder object.
 *
 * \return \c true on successfull decoding and \c false otherwise.
 */
static bool decode_frame(
        lrpt_decoder_t *decoder);

/*************************************************************************************************/

/* fix_packet() */
static void fix_packet(
        int8_t *data,
        size_t len,
        uint8_t shift) {
    len /= 2; /* Useful while swapping elements */

    switch (shift) {
        case 4:
            for (size_t i = 0; i < len; i++) {
                /* Swap adjacent elements */
                int8_t b = data[i * 2 + 0];
                data[i * 2 + 0] = data[i * 2 + 1];
                data[i * 2 + 1] = b;
            }

            break;

        case 5:
            for (size_t i = 0; i < len; i++)
                /* Invert odd elements */
                data[i * 2 + 0] = -data[i * 2 + 0];

            break;

        case 6:
            for (size_t i = 0; i < len; i++) {
                /* Swap and invert adjacent elements */
                int8_t b = data[i * 2 + 0];
                data[i * 2 + 0] = -data[i * 2 + 1];
                data[i * 2 + 1] = -b;
            }

            break;

        case 7:
            for (size_t i = 0; i < len; i++)
                /* Invert even elements */
                data[i * 2 + 1] = -data[i * 2 + 1];

            break;
    }
}

/*************************************************************************************************/

/* do_next_correlate() */
static void do_next_correlate(
        lrpt_decoder_t *decoder,
        int8_t *data) {
    /* Just copy new part of data to the aligned buffer */
    memcpy(decoder->aligned, (data + decoder->pos), LRPT_DECODER_SOFT_FRAME_LEN);

    /* Advance decoder position */
    decoder->prev_pos = decoder->pos;
    decoder->pos += LRPT_DECODER_SOFT_FRAME_LEN;

    fix_packet(decoder->aligned, LRPT_DECODER_SOFT_FRAME_LEN, decoder->corr_word);
}

/*************************************************************************************************/

/* do_full_correlate() */
static void do_full_correlate(
        lrpt_decoder_t *decoder,
        int8_t *data) {
    decoder->corr_word = lrpt_decoder_correlator_correlate(
            decoder->corr, (data + decoder->pos), LRPT_DECODER_SOFT_FRAME_LEN);
    decoder->corr_pos = decoder->corr->position[decoder->corr_word];
    decoder->corr_val = decoder->corr->correlation[decoder->corr_word];

    /* If low correlation observed just copy new part of data to the aligned buffer */
    if (decoder->corr_val < DECODER_CORRELATION_MIN) {
        memcpy(decoder->aligned, (data + decoder->pos), LRPT_DECODER_SOFT_FRAME_LEN);

        /* Advance decoder position by a quarter of soft frame length */
        decoder->prev_pos = decoder->pos;
        decoder->pos += (LRPT_DECODER_SOFT_FRAME_LEN / 4);
    }
    else { /* Otherwise we just combine data from two sections into aligned array */
        /* TODO should be careful when no consequent data is present; otherwise we'll be accessing memory areas which are not ours */
        memcpy(decoder->aligned, (data + decoder->pos + decoder->corr_pos),
                (LRPT_DECODER_SOFT_FRAME_LEN - decoder->corr_pos));
        memcpy((decoder->aligned + LRPT_DECODER_SOFT_FRAME_LEN - decoder->corr_pos),
                (data + decoder->pos + LRPT_DECODER_SOFT_FRAME_LEN), decoder->corr_pos);

        /* Advance decoder position */
        decoder->prev_pos = (decoder->pos + decoder->corr_pos);
        decoder->pos += (LRPT_DECODER_SOFT_FRAME_LEN + decoder->corr_pos);

        fix_packet(decoder->aligned, LRPT_DECODER_SOFT_FRAME_LEN, decoder->corr_word);
    }
}

/*************************************************************************************************/

static bool decode_frame(
        lrpt_decoder_t *decoder) {
    lrpt_decoder_viterbi_decode(decoder->vit, decoder->corr, decoder->aligned, decoder->decoded);

    uint32_t tmp =
        ((uint32_t)decoder->decoded[3] << 24) +
        ((uint32_t)decoder->decoded[2] << 16) +
        ((uint32_t)decoder->decoded[1] << 8) +
        (uint32_t)decoder->decoded[0];
    decoder->last_sync = tmp;

    /* Estimate signal quality */
    decoder->sig_q = 100 - lrpt_decoder_viterbi_ber_percent(decoder->vit);

    /* TODO use named consts */
    /* You can flip all bits in a packet and get a correct ECC anyway. Check for that case */
    if (lrpt_decoder_bitop_count(decoder->last_sync ^ 0xE20330E5) <
            lrpt_decoder_bitop_count(decoder->last_sync ^ 0x1DFCCF1A)) {
        for (size_t i = 0; i < LRPT_DECODER_HARD_FRAME_LEN; i++)
            decoder->decoded[i] ^= 0xFF;

        tmp =
            ((uint32_t)decoder->decoded[3] << 24) +
            ((uint32_t)decoder->decoded[2] << 16) +
            ((uint32_t)decoder->decoded[1] << 8) +
            (uint32_t)decoder->decoded[0];
        decoder->last_sync = tmp;
    }

    /* 4 is an offset because of sync word */
    for (size_t i = 0; i < (LRPT_DECODER_HARD_FRAME_LEN - 4); i++)
        decoder->decoded[4 + i] ^= DECODER_PRAND_TBL[i % 255];

    uint8_t ecc_buf[256]; /* TODO review if it's ok to use static array here */

    for (uint8_t i = 0; i < 4; i++) {
        lrpt_decoder_ecc_deinterleave((decoder->decoded + 4), ecc_buf, i, 4);
        decoder->r[i] = lrpt_decoder_ecc_decode(ecc_buf, 0);
        lrpt_decoder_ecc_interleave(ecc_buf, decoder->ecced, i, 4);
    }

    return (decoder->r[0] && decoder->r[1] && decoder->r[2] && decoder->r[3]);
}

/*************************************************************************************************/

/* lrpt_decoder_data_process_frame() */
bool lrpt_decoder_data_process_frame(
        lrpt_decoder_t *decoder,
        int8_t *data) {
    bool ok = false;

    /* Try to correlate next block */
    if (decoder->corr_pos == 0) {
        do_next_correlate(decoder, data);
        ok = decode_frame(decoder);

        /* In case of failed decoding attempt jump one frame back in data buffer */
        if (!ok)
            decoder->pos -= LRPT_DECODER_SOFT_FRAME_LEN;
    }

    if (!ok) {
        do_full_correlate(decoder, data);
        ok = decode_frame(decoder);
    }

    return ok;
}

/*************************************************************************************************/

/** \endcond */
