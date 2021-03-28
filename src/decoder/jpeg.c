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
#include <string.h>

/* DEBUG */
#include <inttypes.h>
#include <stdio.h>
/* DEBUG */

/*************************************************************************************************/

/* TODO This is the code specific for Meteor-M2 only. For more information see section "I",
 * http://planet.iitp.ru/spacecraft/meteor_m_n2_structure_2.pdf
 */
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
static const uint8_t JPEG_DC_CAT_OFFSET[12] = { 2, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9 };

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
        const int32_t *in);

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
        const int32_t *in) {
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            double s = 0;

            for (uint8_t u = 0; u < 8; u++) {
                double cxu = jpeg->alpha[u] * jpeg->cosine[x][u];
                double ss = 0;

                for (uint8_t i = 0; i < 8; i++)
                    ss += in[i * 8 + u] * jpeg->alpha[i] * jpeg->cosine[y][i];

                ss *= cxu;
                s += ss;
            }

            res[y * 8 + x] = s / 4.0;
        }
    }
}

/*************************************************************************************************/

/* fill_dqt_by_q() */
static void fill_dqt_by_q(
        uint16_t *dqt,
        uint8_t q) {
    double f;

    if ((q > 20) && (q < 50))
        f = 5000.0 / q;
    else
        f = 200.0 - 2.0 * q;

    for (uint8_t i = 0; i < 64; i++) {
        dqt[i] = round(f / 100.0 * JPEG_STD_QUANT_TBL[i]);

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

    if (jpeg->first) {
        if (mcu_id != 0)
            return false;

        jpeg->first = false;

        jpeg->prev_pck = pck_cnt;
        jpeg->first_pck = pck_cnt;

        /* TODO seems like there is some kind of mess. May be it's related to the Meteor-M2
         * specifics (http://planet.iitp.ru/spacecraft/meteor_m_n2_structure_2.pdf). In any case
         * that should be retested well in future with new spacecrafts and reviewed. It's even
         * better to pass current active APIDs by hand so we can do right things here.
         * Or may be we can decide proper APIDs on the pre-analysis...
         */
        /* Realign */
        if (apid == 65)
            jpeg->first_pck -= 14;

        if ((apid == 66) || (apid == 68))
            jpeg->first_pck -= 28;
    }

    /* Handle counter reset. For more information see section "3.2 Source Packet structure",
     * https://www-cdn.eumetsat.int/files/2020-04/pdf_mo_ds_esa_sy_0048_iss8.pdf
     */
    if (pck_cnt < jpeg->prev_pck)
        jpeg->first_pck -= 16384;

    jpeg->prev_pck = pck_cnt;

    /* 8 is MCU block size; 43 = (14 + 14 + 14 + 1) - number of partial packets for single line.
     * For more information see section "I",
     * http://planet.iitp.ru/spacecraft/meteor_m_n2_structure_2.pdf
     * and section "3.2 Source Packet structure",
     * https://www-cdn.eumetsat.int/files/2020-04/pdf_mo_ds_esa_sy_0048_iss8.pdf
     */
    jpeg->cur_y = 8 * ((pck_cnt - jpeg->first_pck) / 43);

    if ((jpeg->cur_y > jpeg->last_y) || !jpeg->progressed) {
        uint16_t channel_image_height = jpeg->cur_y + 8;

        decoder->channel_image_size = decoder->channel_image_width * channel_image_height;

        /* TODO realloc is costly. May be pre-alloc big enough array is a better idea? */
        for (uint8_t i = 0; i < 6; i++)
            decoder->channel_image[i] = /* TODO add error checking */
                reallocarray(decoder->channel_image[i],
                        decoder->channel_image_size,
                        sizeof(uint8_t));

        jpeg->progressed = true;

        /* Clear new allocation */
        size_t delta_len = decoder->channel_image_size - decoder->prev_len;

        for (uint8_t i = 0; i < 6; i++)
            memset(decoder->channel_image[i] + decoder->prev_len, 0, sizeof(uint8_t) * delta_len);

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
    for (uint8_t i = 0; i < 64; i++) {
        int32_t t = round(img_dct[i] + 128.0);

        if (t < 0)
            t = 0;

        if (t > 255)
            t = 255;

        uint16_t x = (mcu_id + m) * 8 + i % 8;
        uint16_t y = decoder->jpeg->cur_y + i / 8;
        size_t off = x + y * 1568; /* TODO should use spacecraft-dependent parameter here */

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
        decoder->channel_image[apid - 64][off] = t;
        /* DEBUG */
        fprintf(stderr, "fill_pix(): apid = %" PRIu16 "; off = %zu; t = %" PRId32 "\n", apid, off, t);
        /* DEBUG */
    }
}

/*************************************************************************************************/

/* lrpt_decoder_jpeg_init() */
lrpt_decoder_jpeg_t *lrpt_decoder_jpeg_init(void) {
    /* Allocate JPEG decoder object */
    lrpt_decoder_jpeg_t *jpeg = malloc(sizeof(lrpt_decoder_jpeg_t));

    if (!jpeg)
        return NULL;

    /* Initialize DCT tables */
    for (uint8_t y = 0; y < 8; y++)
        for (uint8_t x = 0; x < 8; x++)
            jpeg->cosine[y][x] = cos(M_PI / 16.0 * (2.0 * y + 1.0) * x);

    jpeg->alpha[0] = 1.0 / sqrt(2.0);

    for (uint8_t i = 1; i < 8; i++)
        jpeg->alpha[i] = 1.0;

    /* Set internal state variables */
    jpeg->first = true;
    jpeg->progressed = false;

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
    lrpt_decoder_bitop_writer_set(&b, p);

    if (!progress_image(decoder, apid, mcu_id, pck_cnt))
        return false; /* TODO need error reporting, but be aware of APIDs 0, 70 and mcu_id != 0 */

    uint16_t dqt[64];
    fill_dqt_by_q(dqt, q);

    int32_t prev_dc = 0;
    int32_t zdct[64];
    int32_t dct[64];
    double img_dct[64];

    /* TODO This is the code specific for Meteor-M2 only. For more information see section "I",
     * http://planet.iitp.ru/spacecraft/meteor_m_n2_structure_2.pdf
     */
    for (uint8_t m = 0; m < JPEG_MCU_PER_PACKET; m++) {
        int32_t dc_cat =
            lrpt_decoder_huffman_get_dc(decoder->huff, lrpt_decoder_bitop_peek_n_bits(&b, 16));

        if (dc_cat == -1)
            return false;

        lrpt_decoder_bitop_advance_n_bits(&b, JPEG_DC_CAT_OFFSET[dc_cat]);

        uint16_t n = lrpt_decoder_bitop_fetch_n_bits(&b, dc_cat);

        zdct[0] = lrpt_decoder_huffman_map_range(dc_cat, n) + prev_dc;
        prev_dc = zdct[0];

        uint8_t k = 1;

        while (k < 64) {
            int32_t ac =
                lrpt_decoder_huffman_get_ac(decoder->huff, lrpt_decoder_bitop_peek_n_bits(&b, 16));

            if (ac == -1)
                return false;

            uint8_t ac_len = decoder->huff->ac_tbl[ac].len;
            uint16_t ac_size = decoder->huff->ac_tbl[ac].size;
            uint16_t ac_run = decoder->huff->ac_tbl[ac].run;

            lrpt_decoder_bitop_advance_n_bits(&b, ac_len);

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
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
