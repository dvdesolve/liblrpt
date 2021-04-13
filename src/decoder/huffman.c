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
 * Huffman decoder routines.
 *
 * This source file contains routines for performing Huffman decoding.
 */

/*************************************************************************************************/

#include "huffman.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* List of AC numbers of Y component */
static const uint8_t HUFF_AC_Y_TBL[178] = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125,
    1  , 2  , 3  , 0  , 4  , 17 , 5  , 18 , 33 ,
    49 , 65 , 6  , 19 , 81 , 97 , 7  , 34 , 113,
    20 , 50 , 129, 145, 161, 8  , 35 , 66 , 177,
    193, 21 , 82 , 209, 240, 36 , 51 , 98 , 114,
    130, 9  , 10 , 22 , 23 , 24 , 25 , 26 , 37 ,
    38 , 39 , 40 , 41 , 42 , 52 , 53 , 54 , 55 ,
    56 , 57 , 58 , 67 , 68 , 69 , 70 , 71 , 72 ,
    73 , 74 , 83 , 84 , 85 , 86 , 87 , 88 , 89 ,
    90 , 99 , 100, 101, 102, 103, 104, 105, 106,
    115, 116, 117, 118, 119, 120, 121, 122, 131,
    132, 133, 134, 135, 136, 137, 138, 146, 147,
    148, 149, 150, 151, 152, 153, 154, 162, 163,
    164, 165, 166, 167, 168, 169, 170, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 194, 195,
    196, 197, 198, 199, 200, 201, 202, 210, 211,
    212, 213, 214, 215, 216, 217, 218, 225, 226,
    227, 228, 229, 230, 231, 232, 233, 234, 241,
    242, 243, 244, 245, 246, 247, 248, 249, 250
};

static const size_t HUFF_ACDC_TBL_LEN = 65536; /**< AC and DC tables length */

/*****************************************************************************/

/** Return AC Huffman code.
 *
 * \param huff Pointer to the Huffman decoder object.
 * \param w Index.
 *
 * \return AC Huffman code or \c -1 in case of error.
 */
static int32_t get_ac_real(
        const lrpt_decoder_huffman_t *huff,
        uint16_t w);

/** Return DC Huffman code.
 *
 * \param w Index.
 *
 * \return DC Huffman code or \c -1 in case of error.
 */
static int32_t get_dc_real(
        uint16_t w);

/*****************************************************************************/

/* get_ac_real() */
static int32_t get_ac_real(
        const lrpt_decoder_huffman_t *huff,
        uint16_t w) {
    for (size_t i = 0; i < huff->ac_tbl_len; i++)
        if (((w >> (16 - huff->ac_tbl[i].len)) & huff->ac_tbl[i].mask) == huff->ac_tbl[i].code)
            return i;

    return -1;
}

/*****************************************************************************/

/* get_dc_real() */
static int32_t get_dc_real(
        uint16_t w) {
    /* Standard Codecs: Image Compression to Advanced Video Coding (IET Telecommunications Series),
     * M. Ghanbari. Appendix B, table B.1
     */
    if ((w >> 14) == 0)
        return 0;

    switch (w >> 13) {
        case 2:
            return 1;

            break;

        case 3:
            return 2;

            break;

        case 4:
            return 3;

            break;

        case 5:
            return 4;

            break;

        case 6:
            return 5;

            break;
    }

    if ((w >> 12) == 0x0E)
        return 6;
    else if ((w >> 11) == 0x1E)
        return 7;
    else if ((w >> 10) == 0x3E)
        return 8;
    else if ((w >> 9) == 0x7E)
        return 9;
    else if ((w >> 8) == 0xFE)
        return 10;
    else if ((w >> 7) == 0x01FE)
        return 11;
    else
        return -1;
}

/*************************************************************************************************/

/* lrpt_decoder_huffman_init() */
lrpt_decoder_huffman_t *lrpt_decoder_huffman_init(void) {
    /* Allocate Huffman decoder object */
    lrpt_decoder_huffman_t *huff = malloc(sizeof(lrpt_decoder_huffman_t));

    if (!huff)
        return NULL;

    /* NULL-init internal tables for safe deallocation */
    huff->ac_tbl = NULL;
    huff->ac_lut = NULL;
    huff->dc_lut = NULL;

    /* Allocate helper vector */
    uint8_t *v = calloc(65536, sizeof(uint8_t));

    if (!v) {
        lrpt_decoder_huffman_deinit(huff);

        return NULL;
    }

    /* Populate helper vector */
    uint8_t p = 16;

    for (uint8_t i = 1; i <= 16; i++)
        for (uint8_t j = 0; j < HUFF_AC_Y_TBL[i - 1]; j++) {
            v[(i << 8) + j] = HUFF_AC_Y_TBL[p];
            p++;
        }

    /* Populate codes */
    uint16_t min_code[17], maj_code[17];
    uint32_t code = 0;

    for (uint8_t i = 1; i <= 16; i++) {
        min_code[i] = code;

        if (HUFF_AC_Y_TBL[i - 1] >= 1)
            code += HUFF_AC_Y_TBL[i - 1];

        maj_code[i] = (code - ((code != 0) ? 1 : 0));
        code *= 2;

        if (HUFF_AC_Y_TBL[i - 1] == 0) {
            min_code[i] = 0xFFFF;
            maj_code[i] = 0;
        }
    }

    /* Allocate AC table */
    huff->ac_tbl_len = 256;
    huff->ac_tbl = calloc(huff->ac_tbl_len, sizeof(lrpt_decoder_huffman_acdata_t));

    /* Allocate lookup tables */
    huff->ac_lut = calloc(HUFF_ACDC_TBL_LEN, sizeof(int32_t));
    huff->dc_lut = calloc(HUFF_ACDC_TBL_LEN, sizeof(int32_t));

    if (!huff->ac_tbl || !huff->ac_lut || !huff->dc_lut) {
        free(v);
        lrpt_decoder_huffman_deinit(huff);

        return NULL;
    }

    size_t n = 0;
    uint8_t min_valn = 1;
    uint8_t max_valn = 1;

    for (uint8_t i = 1; i <= 16; i++) {
        uint16_t min_val = min_code[min_valn];
        uint16_t max_val = maj_code[max_valn];

        for (uint32_t j = 0; j < (1 << i); j++) {
            if ((j <= max_val) && (j >= min_val)) {
                uint16_t size_val = v[(i << 8) + j - min_val];

                huff->ac_tbl[n].run = (size_val >> 4);
                huff->ac_tbl[n].size = (size_val & 0x0F);
                huff->ac_tbl[n].len = i;
                huff->ac_tbl[n].mask = ((1 << i) - 1);
                huff->ac_tbl[n].code = j;

                n++;
            }
        }

        min_valn++;
        max_valn++;
    }

    /* Reallocate AC table */
    lrpt_decoder_huffman_acdata_t *new_ac_tbl =
        reallocarray(huff->ac_tbl, n, sizeof(lrpt_decoder_huffman_acdata_t));

    if (!new_ac_tbl) {
        free(v);
        lrpt_decoder_huffman_deinit(huff);

        return NULL;
    }

    /* Zero out newly allocated portion (if any) */
    if (n > huff->ac_tbl_len)
        memset(new_ac_tbl + huff->ac_tbl_len, 0,
                sizeof(lrpt_decoder_huffman_acdata_t) * (n - huff->ac_tbl_len));

    huff->ac_tbl = new_ac_tbl;
    huff->ac_tbl_len = n;

    /* Fill up AC and DC tables */
    for (size_t i = 0; i < HUFF_ACDC_TBL_LEN; i++) {
        huff->ac_lut[i] = get_ac_real(huff, i);
        huff->dc_lut[i] = get_dc_real(i);
    }

    /* Free helper vector */
    free(v);

    return huff;
}

/*************************************************************************************************/

/* lrpt_decoder_huffman_deinit() */
void lrpt_decoder_huffman_deinit(lrpt_decoder_huffman_t *huff) {
    if (!huff)
        return;

    free(huff->dc_lut);
    free(huff->ac_lut);
    free(huff->ac_tbl);
    free(huff);
}

/*************************************************************************************************/

/* lrpt_decoder_huffman_get_ac() */
int32_t lrpt_decoder_huffman_get_ac(
        const lrpt_decoder_huffman_t *huff,
        uint16_t w) {
    return huff->ac_lut[w];
}

/*************************************************************************************************/

/* lrpt_decoder_huffman_get_dc() */
int32_t lrpt_decoder_huffman_get_dc(
        const lrpt_decoder_huffman_t *huff,
        uint16_t w) {
    return huff->dc_lut[w];
}

/*************************************************************************************************/

/* lrpt_decoder_huffman_map_range() */
int32_t lrpt_decoder_huffman_map_range(
        uint8_t cat,
        uint16_t val) {
    uint16_t maxval = ((1 << cat) - 1);

    if ((val >> (cat - 1)) != 0)
        return val;
    else
        return (val - maxval);
}

/*************************************************************************************************/

/** \endcond */
