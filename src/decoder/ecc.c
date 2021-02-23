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
 * Error correction coding routines.
 *
 * This source file contains routines for performing error correction operations.
 */

/*************************************************************************************************/

#include "ecc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*************************************************************************************************/

static const uint8_t ECC_ALPHA_TBL[256] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x87, 0x89, 0x95, 0xAD, 0xDD, 0x3D, 0x7A, 0xF4,
    0x6F, 0xDE, 0x3B, 0x76, 0xEC, 0x5F, 0xBE, 0xFB,
    0x71, 0xE2, 0x43, 0x86, 0x8B, 0x91, 0xA5, 0xCD,
    0x1D, 0x3A, 0x74, 0xE8, 0x57, 0xAE, 0xDB, 0x31,
    0x62, 0xC4, 0x0F, 0x1E, 0x3C, 0x78, 0xF0, 0x67,
    0xCE, 0x1B, 0x36, 0x6C, 0xD8, 0x37, 0x6E, 0xDC,
    0x3F, 0x7E, 0xFC, 0x7F, 0xFE, 0x7B, 0xF6, 0x6B,
    0xD6, 0x2B, 0x56, 0xAC, 0xDF, 0x39, 0x72, 0xE4,
    0x4F, 0x9E, 0xBB, 0xF1, 0x65, 0xCA, 0x13, 0x26,
    0x4C, 0x98, 0xB7, 0xE9, 0x55, 0xAA, 0xD3, 0x21,
    0x42, 0x84, 0x8F, 0x99, 0xB5, 0xED, 0x5D, 0xBA,
    0xF3, 0x61, 0xC2, 0x03, 0x06, 0x0C, 0x18, 0x30,
    0x60, 0xC0, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0,
    0x47, 0x8E, 0x9B, 0xB1, 0xE5, 0x4D, 0x9A, 0xB3,
    0xE1, 0x45, 0x8A, 0x93, 0xA1, 0xC5, 0x0D, 0x1A,
    0x34, 0x68, 0xD0, 0x27, 0x4E, 0x9C, 0xBF, 0xF9,
    0x75, 0xEA, 0x53, 0xA6, 0xCB, 0x11, 0x22, 0x44,
    0x88, 0x97, 0xA9, 0xD5, 0x2D, 0x5A, 0xB4, 0xEF,
    0x59, 0xB2, 0xE3, 0x41, 0x82, 0x83, 0x81, 0x85,
    0x8D, 0x9D, 0xBD, 0xFD, 0x7D, 0xFA, 0x73, 0xE6,
    0x4B, 0x96, 0xAB, 0xD1, 0x25, 0x4A, 0x94, 0xAF,
    0xD9, 0x35, 0x6A, 0xD4, 0x2F, 0x5E, 0xBC, 0xFF,
    0x79, 0xF2, 0x63, 0xC6, 0x0B, 0x16, 0x2C, 0x58,
    0xB0, 0xE7, 0x49, 0x92, 0xA3, 0xC1, 0x05, 0x0A,
    0x14, 0x28, 0x50, 0xA0, 0xC7, 0x09, 0x12, 0x24,
    0x48, 0x90, 0xA7, 0xC9, 0x15, 0x2A, 0x54, 0xA8,
    0xD7, 0x29, 0x52, 0xA4, 0xCF, 0x19, 0x32, 0x64,
    0xC8, 0x17, 0x2E, 0x5C, 0xB8, 0xF7, 0x69, 0xD2,
    0x23, 0x46, 0x8C, 0x9F, 0xB9, 0xF5, 0x6D, 0xDA,
    0x33, 0x66, 0xCC, 0x1F, 0x3E, 0x7C, 0xF8, 0x77,
    0xEE, 0x5B, 0xB6, 0xEB, 0x51, 0xA2, 0xC3, 0x00
};

static const uint8_t ECC_IDX_TBL[256] = {
    255, 0  , 1  , 99 , 2  , 198, 100, 106,
    3  , 205, 199, 188, 101, 126, 107, 42 ,
    4  , 141, 206, 78 , 200, 212, 189, 225,
    102, 221, 127, 49 , 108, 32 , 43 , 243,
    5  , 87 , 142, 232, 207, 172, 79 , 131,
    201, 217, 213, 65 , 190, 148, 226, 180,
    103, 39 , 222, 240, 128, 177, 50 , 53 ,
    109, 69 , 33 , 18 , 44 , 13 , 244, 56 ,
    6  , 155, 88 , 26 , 143, 121, 233, 112,
    208, 194, 173, 168, 80 , 117, 132, 72 ,
    202, 252, 218, 138, 214, 84 , 66 , 36 ,
    191, 152, 149, 249, 227, 94 , 181, 21 ,
    104, 97 , 40 , 186, 223, 76 , 241, 47 ,
    129, 230, 178, 63 , 51 , 238, 54 , 16 ,
    110, 24 , 70 , 166, 34 , 136, 19 , 247,
    45 , 184, 14 , 61 , 245, 164, 57 , 59 ,
    7  , 158, 156, 157, 89 , 159, 27 , 8  ,
    144, 9  , 122, 28 , 234, 160, 113, 90 ,
    209, 29 , 195, 123, 174, 10 , 169, 145,
    81 , 91 , 118, 114, 133, 161, 73 , 235,
    203, 124, 253, 196, 219, 30 , 139, 210,
    215, 146, 85 , 170, 67 , 11 , 37 , 175,
    192, 115, 153, 119, 150, 92 , 250, 82 ,
    228, 236, 95 , 74 , 182, 162, 22 , 134,
    105, 197, 98 , 254, 41 , 125, 187, 204,
    224, 211, 77 , 140, 242, 31 , 48 , 220,
    130, 171, 231, 86 , 179, 147, 64 , 216,
    52 , 176, 239, 38 , 55 , 12 , 17 , 68 ,
    111, 120, 25 , 154, 71 , 116, 167, 193,
    35 , 83 , 137, 251, 20 , 93 , 248, 151,
    46 , 75 , 185, 96 , 15 , 237, 62 , 229,
    246, 135, 165, 23 , 58 , 163, 60 , 183
};

/*************************************************************************************************/

/* lrpt_decoder_ecc_interleave() */
void lrpt_decoder_ecc_interleave(
        const uint8_t *data,
        uint8_t *output,
        size_t pos,
        size_t n) {
    /* TODO recheck if we should use 256 instead of 255 */
    for (size_t i = 0; i < 255; i++)
        output[i * n + pos] = data[i];
}

/*************************************************************************************************/

/* lrpt_decoder_ecc_deinterleave() */
void lrpt_decoder_ecc_deinterleave(
        const uint8_t *data,
        uint8_t *output,
        size_t pos,
        size_t n) {
    /* TODO recheck if we should use 256 instead of 255 */
    for (size_t i = 0; i < 255; i++)
        output[i] = data[i * n + pos];
}

/*************************************************************************************************/

/* lrpt_decoder_ecc_decode() */
bool lrpt_decoder_ecc_decode(
        uint8_t *data,
        size_t pad) {
    uint8_t s[32];

    for (size_t i = 0; i < 32; i++)
        s[i] = data[0];

    for (size_t j = 1; j < (255 - pad); j++)
        for (size_t i = 0; i < 32; i++)
            if (s[i] == 0)
                s[i] = data[j];
            else
                s[i] = data[j] ^ ECC_ALPHA_TBL[(ECC_IDX_TBL[s[i]] + (112 + i) * 11) % 255];

    uint8_t syn_error = 0;

    for (size_t i = 0; i < 32; i++) {
        syn_error |= s[i];
        s[i] = ECC_IDX_TBL[s[i]];
    }

    if (syn_error == 0)
        return true;

    /* Prepare lambda array */
    uint8_t lambda[33];
    memset(lambda, 0, 33);
    lambda[0] = 1;

    uint8_t b[33], t[33];

    for (size_t i = 0; i < 33; i++)
        b[i] = ECC_IDX_TBL[lambda[i]];

    uint8_t r = 1;
    uint8_t el = 0;

    /* TODO may be use for loop (the same is true for other whiles) */
    while (r <= 32) {
        uint8_t discr_r = 0;

        for (size_t i = 0; i < r; i++)
            if ((lambda[i] != 0) && (s[r - i - 1] != 255))
                discr_r ^= ECC_ALPHA_TBL[(ECC_IDX_TBL[lambda[i]] + s[r - i - 1]) % 255];

        discr_r = ECC_IDX_TBL[discr_r];

        if (discr_r == 255) {
            memmove((b + 1), b, 32);
            b[0] = 255;
        }
        else {
            t[0] = lambda[0];

            for (size_t i = 0; i < 32; i++) {
                if (b[i] != 255)
                    t[i + 1] = lambda[i + 1] ^ ECC_ALPHA_TBL[(discr_r + b[i]) % 255];
                else
                    t[i + 1] = lambda[i + 1];
            }

            if ((2 * el) <= (r - 1)) {
                el = r - el;

                for (size_t i = 0; i < 32; i++) {
                    if (lambda[i] == 0)
                        b[i] = 255;
                    else /* TODO suspicious part here, may be overflow */
                        b[i] = (ECC_IDX_TBL[lambda[i]] - discr_r + 255) % 255;
                }
            }
            else {
                memmove((b +1), b, 32);
                b[0] = 255;
            }

            memmove(lambda, t, 33);
        }

        r++;
    }

    size_t deg_lambda = 0;

    for (size_t i = 0; i < 33; i++) {
        lambda[i] = ECC_IDX_TBL[lambda[i]];

        if (lambda[i] != 255)
            deg_lambda = i;
    }

    uint8_t reg[33];
    memmove((reg + 1), (lambda + 1), 32);

    uint8_t root[32], loc[32];
    size_t result = 0; /* Holds amount of errors fixed */
    size_t n = 1;
    size_t k = 115;

    while (true) {
        if (n > 255)
            break;

        uint8_t q = 1;

        for (size_t i = deg_lambda; i >= 1; i--) {
            if (reg[i] != 255) {
                reg[i] = (reg[i] + i) % 255; /* TODO suspicious cast to uint8_t was here */
                q ^= ECC_ALPHA_TBL[reg[i]];
            }
        }

        if (q != 0) {
            n++;
            k = (k + 116) % 255;

            continue;
        }

        /* TODO do we need that casts? */
        root[result] = (uint8_t)n;
        loc[result] = (uint8_t)k;
        result++;

        if (result == deg_lambda)
            break;

        n++;
        k = (k + 116) % 255;
    }

    if (deg_lambda != result)
        return false;

    uint8_t omega[33];

    size_t deg_omega = deg_lambda - 1;

    for (size_t i = 0; i <= deg_omega; i++) {
        uint8_t tmp = 0;

        for (size_t j = i; j >= 0; j--)
            if ((s[i - j] != 255) && (lambda[j] != 255))
                tmp ^= ECC_ALPHA_TBL[(s[i - j] + lambda[j]) % 255];

        omega[i] = ECC_IDX_TBL[tmp];
    }

    for (size_t i = result - 1; i >= 0; i--) {
        uint8_t num1 = 0;

        for (size_t j = deg_omega; j >= 0; j--)
            if (omega[j] != 255)
                num1 ^= ECC_ALPHA_TBL[(omega[j] + j * root[i]) % 255];

        uint8_t num2 = ECC_ALPHA_TBL[(root[i] * 111 + 255) % 255];
        uint8_t den = 0;

        size_t l;

        if (deg_lambda < 31)
            l = deg_lambda;
        else
            l = 31;

        l &= ~1;

        while (true) {
            if (!(l >= 0))
                break;

            if (lambda[l + 1] != 255)
                den ^= ECC_ALPHA_TBL[(lambda[l + 1] + l * root[i]) % 255];

            l -= 2;
        }

        if ((num1 != 0) && (loc[i] >= pad))
            data[loc[i] - pad] ^=
                ECC_ALPHA_TBL[(ECC_IDX_TBL[num1] + ECC_IDX_TBL[num2] +
                        255 - ECC_IDX_TBL[den]) % 255];
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
