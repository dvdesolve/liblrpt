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

/* TODO recheck for population routine. That should be inside Huffman decoder object */
/* Lookup tables for AC and DC */
static size_t HUFF_AC_LUT[65536];
static size_t HUFF_DC_LUT[65536];

/*****************************************************************************/

/** Return AC Huffman code.
 *
 * \param huff Pointer to the Huffman decoder object.
 * \param w Index.
 *
 * \return AC Huffman code.
 */
static int get_ac_real(
        const lrpt_decoder_huffman_t *huff,
        uint16_t w);

/** Return DC Huffman code.
 *
 * \param w Index.
 *
 * \return DC Huffman code.
 */
static int get_dc_real(
        uint16_t w);

/*****************************************************************************/

/* TODO change w */
/* get_ac_real() */
static int get_ac_real(
        const lrpt_decoder_huffman_t *huff,
        uint16_t w) {
    for (size_t i = 0; i < huff->ac_tbl_len; i++)
        if (((w >> (16 - huff->ac_tbl[i].len)) & huff->ac_tbl[i].mask) == huff->ac_tbl[i].code)
            return i;

    return -1; /* TODO signal with another way. We can return huff->ac_tbl_len and do the check in Get_AC() */
}

/*****************************************************************************/

/* TODO change w */
/* get_dc_real() */
static int get_dc_real(
        uint16_t w) {
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
        return -1; /* TODO signal with another way. We can return 12 and do the check in Get_DC() */
}

/*************************************************************************************************/

/* lrpt_decoder_huffman_init() */
lrpt_decoder_huffman_t *lrpt_decoder_huffman_init(void) {
    /* Allocate Huffman decoder object */
    lrpt_decoder_huffman_t *huff = malloc(sizeof(lrpt_decoder_huffman_t));

    if (!huff)
        return NULL;

    /* NULL-init AC table for safe deallocation */
    huff->ac_tbl = NULL;

    /* Allocate helper vector */
    uint8_t *v = calloc(65536, sizeof(uint8_t));

    if (!v) {
        lrpt_decoder_huffman_deinit(huff);

        return NULL;
    }

    /* Populate helper vector */
    size_t p = 16;

    for (size_t i = 1; i <= 16; i++)
        for (size_t j = 0; j < HUFF_AC_Y_TBL[i - 1]; i++) {
            v[(i << 8) + j] = HUFF_AC_Y_TBL[p];
            p++;
        }

    /* Populate codes */
    uint16_t min_code[17], maj_code[17];
    uint32_t code = 0;

    for (size_t i = 1; i <= 16; i++) {
        min_code[i] = (uint16_t)code;

        for (size_t j = 1; j <= HUFF_AC_Y_TBL[i - 1]; j++)
            code++;

        maj_code[i] = (uint16_t)(code - 1 * (uint32_t)(code != 0));
        code *= 2;

        if (HUFF_AC_Y_TBL[i - 1] == 0) {
            min_code[i] = 0xFFFF;
            maj_code[i] = 0;
        }
    }

    /* Allocate AC table */
    huff->ac_tbl_len = 256;
    huff->ac_tbl = calloc(huff->ac_tbl_len, sizeof(lrpt_decoder_huffman_acdata_t));

    if (!huff->ac_tbl) {
        free(v);
        lrpt_decoder_huffman_deinit(huff);

        return NULL;
    }

    size_t n = 0;
    size_t min_valn = 1;
    size_t max_valn = 1;

    for (size_t k = 1; k <= 16; k++) {
        uint16_t min_val = min_code[min_valn];
        uint16_t max_val = maj_code[max_valn];

        for (size_t i = 0; i < (1 << k); i++)
            if (((uint16_t)i <= max_val) && ((uint16_t)i >= min_val)) {
                uint16_t size_val = v[(k << 8) + i - (int)min_val];

                huff->ac_tbl[n].run = (size_val >> 4);
                huff->ac_tbl[n].size = (size_val & 0x0F);
                huff->ac_tbl[n].len = k;
                huff->ac_tbl[n].mask = (1 << k) - 1;
                huff->ac_tbl[n].code = (uint32_t)i;

                n++;
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

    /* TODO recheck this very carefully! */
    /* Zero out newly allocated portion (if any) */
    if (n > huff->ac_tbl_len)
        memset(new_ac_tbl + huff->ac_tbl_len, 0, n - huff->ac_tbl_len);

    huff->ac_tbl = new_ac_tbl;
    huff->ac_tbl_len = n;

    /* TODO review that */
    /* Fill AC and DC tables */
    for (size_t i = 0; i < 65536; i++) {
        HUFF_AC_LUT[i] = get_ac_real(huff, (uint16_t)i);
        HUFF_DC_LUT[i] = get_dc_real((uint16_t)i);
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

    free(huff->ac_tbl);
    free(huff);
}

/*************************************************************************************************/

/* lrpt_decoder_huffman_get_ac() */
size_t lrpt_decoder_huffman_get_ac(
        uint16_t w) {
    return HUFF_AC_LUT[w];
}

/*************************************************************************************************/

/* lrpt_decoder_huffman_get_dc() */
size_t lrpt_decoder_huffman_get_dc(
        uint16_t w) {
    return HUFF_DC_LUT[w];
}

/*************************************************************************************************/

/* TODO review types */
/* lrpt_decoder_huffman_map_range() */
int lrpt_decoder_huffman_map_range(
        int cat,
        uint32_t vl) {
    int maxval = (1 << cat) - 1;

    if ((vl >> (cat - 1)) != 0)
        return vl;
    else
        return (vl - maxval);
}

/*************************************************************************************************/

/** \endcond */
