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
 * JPEG decoder routines.
 *
 * This source file contains routines for performing JPEG decoding.
 */

/*************************************************************************************************/

#include "jpeg.h"

#include "../../include/lrpt.h"
#include "bitop.h"
#include "decoder.h"
#include "huffman.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/* TODO review and recheck */
static const uint8_t JPEG_MCU_PER_PACKET = 14; /**< How many MCUs are in single packet? */

/** Standard quantization table */
static const uint8_t JPEG_STD_QUANT_TBL[64] = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
};

/** Zigzag table */
static const uint8_t JPEG_ZZ_TBL[64] = {
    0 , 1 , 5 , 6 , 14, 15, 27, 28,
    2 , 4 , 7 , 13, 16, 26, 29, 42,
    3 , 8 , 12, 17, 25, 30, 41, 43,
    9 , 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

/** Offsets for DC Huffman codes */
static const int JPEG_DC_CAT_OFFSET[12] = { 2, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9 };

/*************************************************************************************************/

/** Perform fast lapped discrete cosine transform for 8x8 block.
 *
 * \param jpeg Pointer to the JPEG decoder object.
 * \param res Resulting array.
 * \param in Input DCT.
 */
static void flt_idct_8x8(
        lrpt_decoder_jpeg_t *jpeg,
        double *res,
        const double *in);

/** Fill quantization table.
 *
 * \param dqt Pointer to the dqt table.
 * \param q Quality factor value.
 */
static void fill_dqt_by_q(
        uint16_t *dqt,
        uint8_t q);

/** Advance image buffers.
 *
 * \param decoder Pointer to the decoder object.
 * \param apid APID number.
 * \param mcu_id ID of current MCU.
 * \param pck_cnt Number of packets.
 *
 * \return \c true on successfull progression and \c false otherwise.
 */
static bool progress_image(
        lrpt_decoder_t *decoder,
        uint16_t apid,
        uint8_t mcu_id,
        uint16_t pck_cnt);

/** Fill pixels.
 *
 * \param decoder Pointer to the decoder object.
 * \param img_dct Image DCT.
 * \param apid APID number.
 * \param mcu_id ID of current MCU.
 * \param m MCU index.
 */
static void fill_pix(
        lrpt_decoder_t *decoder,
        double *img_dct,
        uint16_t apid,
        uint8_t mcu_id,
        uint8_t m);

/*************************************************************************************************/

/* flt_idct_8x8() */
static void flt_idct_8x8(
        lrpt_decoder_jpeg_t *jpeg,
        double *res,
        const double *in) {
    for (uint8_t y = 0; y < 8; y++)
        for (uint8_t x = 0; x < 8; x++) {
            double s = 0;

            for (uint8_t u = 0; u < 8; u++) {
                double cxu = jpeg->alpha[u] * jpeg->cosine[x][u];

                /* TODO make loop */
                /* Unrolled to 8 */
                s += cxu * (
                        in[0 * 8 + u] * jpeg->alpha[0] * jpeg->cosine[y][0] +
                        in[1 * 8 + u] * jpeg->alpha[1] * jpeg->cosine[y][1] +
                        in[2 * 8 + u] * jpeg->alpha[2] * jpeg->cosine[y][2] +
                        in[3 * 8 + u] * jpeg->alpha[3] * jpeg->cosine[y][3] +
                        in[4 * 8 + u] * jpeg->alpha[4] * jpeg->cosine[y][4] +
                        in[5 * 8 + u] * jpeg->alpha[5] * jpeg->cosine[y][5] +
                        in[6 * 8 + u] * jpeg->alpha[6] * jpeg->cosine[y][6] +
                        in[7 * 8 + u] * jpeg->alpha[7] * jpeg->cosine[y][7]);
            }

            res[y * 8 + x] = s / 4.0;
        }
}

/*************************************************************************************************/

/* fill_dqt_by_q() */
static void fill_dqt_by_q(
        uint16_t *dqt,
        uint8_t q) {
    double f;

    if ((q > 20) && (q < 50))
        f = 5000.0 / (double)q;
    else
        f = 200.0 - 2.0 * (double)q;

    for (uint8_t i = 0; i < 64; i++) {
        dqt[i] = (uint16_t)(round(f / 100.0 * (double)JPEG_STD_QUANT_TBL[i]));

        if (dqt[i] < 1)
            dqt[i] = 1;
    }
}

/*************************************************************************************************/

/* progress_image() */
static bool progress_image(
        lrpt_decoder_t *decoder,
        uint16_t apid,
        uint8_t mcu_id,
        uint16_t pck_cnt) {
    if ((apid == 0) || (apid == 70))
        return false;

    lrpt_decoder_jpeg_t *jpeg = decoder->jpeg;

    /* TODO a bit awkward construct here; recheck how it's implemented in original medet */
    if (jpeg->first) {
        if (mcu_id != 0)
            return false;

        jpeg->first = false;

        jpeg->prev_pck = pck_cnt;
        jpeg->first_pck = pck_cnt;

        /* Realign */
        if (apid == 65)
            jpeg->first_pck -= 14;

        if ((apid == 66) || (apid == 68))
            jpeg->first_pck -= 28;
    }

    if (pck_cnt < jpeg->prev_pck)
        jpeg->first_pck -= 16384;

    jpeg->prev_pck = pck_cnt;

    /* TODO 8 is MCU block size */
    jpeg->cur_y = 8 * ((pck_cnt - jpeg->first_pck) / 43); /* TODO why 43? */

    if ((jpeg->cur_y > jpeg->last_y) || !jpeg->progressed) {
        size_t channel_image_height = jpeg->cur_y + 8;

        decoder->channel_image_size = decoder->channel_image_width * channel_image_height;

        /* TODO realloc is costly. May be pre-alloc big enough array is a better idea? */
        for (uint8_t i = 0; i < 6; i++)
            decoder->channel_image[i] = /* TODO add error checking */
                reallocarray(decoder->channel_image[i], decoder->channel_image_size, sizeof(uint8_t));

        jpeg->progressed = true;

        /* Clear new allocation */
        /* TODO may be use more advanced method like in our IO utility funcs... */
        size_t delta_len = decoder->channel_image_size - decoder->prev_len;

        for (uint8_t i = 0; i < 6; i++)
            for (size_t j = 0; j < delta_len; j++)
                decoder->channel_image[i][j + decoder->prev_len] = 0;

        decoder->prev_len = decoder->channel_image_size;
    }

    jpeg->last_y = jpeg->cur_y;

    return true;
}

/*************************************************************************************************/

/* fill_pix() */
static void fill_pix(
        lrpt_decoder_t *decoder,
        double *img_dct,
        uint16_t apid,
        uint8_t mcu_id,
        uint8_t m) {
    int x, y, off = 0;

    for (size_t i = 0; i < 64; i++) {
        int t = (int)(round(img_dct[i] + 128.0)); /* TODO recheck type */

        if (t < 0)
            t = 0;

        if (t > 255)
            t = 255;

        x = (mcu_id + m) * 8 + i % 8;
        y = decoder->jpeg->cur_y + i / 8;
        off = x + y * 1568; /* TODO should use named constant here */

        /* TODO that should be done in postprocessor later or a list of invertable APIDs should be given */
//        bool inv = false;
//        /* Invert image palette if APID matches */
//        for (size_t j = 0; j < 3; j++)
//            if (apid == rc_data.invert_palette[j])
//                inv = true;
//
//        if (inv) {
//            if( apid == rc_data.apid[RED] )
//                channel_image[RED][off]   = 255 - (uint8_t)t;
//            else if( apid == rc_data.apid[GREEN] )
//                channel_image[GREEN][off] = 255 - (uint8_t)t;
//            else if( apid == rc_data.apid[BLUE] )
//                channel_image[BLUE][off]  = 255 - (uint8_t)t;
//        }
//        else /* Normal palette */

        /* TODO signal in some kind of APID counters so we can analyze it later */
        decoder->channel_image[apid - 64][off] = (uint8_t)t;
        /* TODO stopped rechecking here; should be fine to dump images now */
    }
}

/*************************************************************************************************/

/* lrpt_decoder_jpeg_init() */
lrpt_decoder_jpeg_t *lrpt_decoder_jpeg_init(void) {
    /* Allocate JPEG decoder object */
    lrpt_decoder_jpeg_t *jpeg = malloc(sizeof(lrpt_decoder_jpeg_t));

    if (!jpeg)
        return NULL;

    /* TODO review naming */
    /* Initialize DCT tables */
    for (size_t y = 0; y < 8; y++)
        for (size_t x = 0; x < 8; x++)
            jpeg->cosine[y][x] = cos(M_PI / 16.0 * (2.0 * (double)y + 1.0) * (double)x);

    jpeg->alpha[0] = 1.0 / sqrt(2.0);

    for (size_t x = 1; x < 8; x++)
        jpeg->alpha[x] = 1.0;

    /* Set internal state variables */
    jpeg->first = true;
    jpeg->progressed = false;

    jpeg->last_mcu = 0;
    jpeg->cur_y = 0;
    jpeg->last_y = 0;
    jpeg->first_pck = 0;
    jpeg->prev_pck = 0;

    return jpeg;
}

/*************************************************************************************************/

/* lrpt_decoder_jpeg_deinit() */
void lrpt_decoder_jpeg_deinit(lrpt_decoder_jpeg_t *jpeg) {
    if (!jpeg)
        return;

    free(jpeg);
}

/*************************************************************************************************/

/* lrpt_decoder_jpeg_decode_mcus() */
bool lrpt_decoder_jpeg_decode_mcus(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        uint16_t apid,
        uint16_t pck_cnt,
        uint8_t mcu_id,
        uint8_t q) {
    lrpt_decoder_bitop_t b;
    b.p = p;
    b.pos = 0;

    if (!progress_image(decoder, apid, mcu_id, pck_cnt))
        return false; /* TODO need error reporting, but be aware of APIDs 0, 70 and mcu_id != 0 */

    uint16_t dqt[64];
    fill_dqt_by_q(dqt, q);

    double prev_dc = 0;
    uint8_t m = 0;

    double zdct[64]; /* TODO why it is of double type? */
    double dct[64];
    double img_dct[64];

    while (m < JPEG_MCU_PER_PACKET) {
        int dc_cat = lrpt_decoder_huffman_get_dc(decoder->huff, lrpt_decoder_bitop_peek_n_bits(&b, 16));

        if (dc_cat == -1) /* TODO recheck for -1 case */
            return false; /* TODO need error reporting */

        b.pos += JPEG_DC_CAT_OFFSET[dc_cat];
        uint32_t n = lrpt_decoder_bitop_fetch_n_bits(&b, dc_cat);

        zdct[0] = lrpt_decoder_huffman_map_range(dc_cat, n) + prev_dc;
        prev_dc = zdct[0];

        uint8_t k = 1;

        while (k < 64) {
            int ac = lrpt_decoder_huffman_get_ac(decoder->huff, (uint16_t)(lrpt_decoder_bitop_peek_n_bits(&b, 16))); /* TODO recheck casts */

            if (ac == -1) /* TODO recheck for -1 case */
                return false; /* TODO need error reporting */

            size_t ac_len = decoder->huff->ac_tbl[ac].len;
            uint16_t ac_size = decoder->huff->ac_tbl[ac].size;
            uint16_t ac_run = decoder->huff->ac_tbl[ac].run;

            b.pos += ac_len;

            if ((ac_run == 0) && (ac_size == 0)) {
                for (uint8_t i = k; i < 64; i++)
                    zdct[i] = 0;

                break;
            }

            for (uint16_t i = 0; i < ac_run; i++) {
                zdct[k] = 0;
                k++;
            }

            if (ac_size != 0) {
                n = lrpt_decoder_bitop_fetch_n_bits(&b, ac_size);
                zdct[k] = lrpt_decoder_huffman_map_range(ac_size, n);
            }
            else if (ac_run == 15)
                zdct[k] = 0;

            k++;
        }

        for (uint8_t i = 0; i < 64; i++)
            dct[i] = zdct[JPEG_ZZ_TBL[i]] * dqt[i];

        flt_idct_8x8(decoder->jpeg, img_dct, dct);
        fill_pix(decoder, img_dct, apid, mcu_id, m);

        m++;
    }

    /* TODO at this step image can be updated in realtime. State vars: channel_image, apid, cur_y */

    return true;
}

/*************************************************************************************************/

/** \endcond */
