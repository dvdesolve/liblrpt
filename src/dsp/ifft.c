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
 * Author: Tom Roberts
 * Author: Malcolm Slaney
 * Author: Dimitrios Bouras
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Integer FFT algorithm.
 */

/*************************************************************************************************/

#include "ifft.h"

#include "../../include/lrpt.h"
#include "../liblrpt/datatype.h"
#include "../liblrpt/error.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/** Fixed-point integer multiplication with scaling.
 *
 * Result will always be 16-bit long.
 *
 * \param a First integer.
 * \param b Second integer.
 *
 * \return Multiplied and scaled result.
 */
static inline int16_t int_mult(
        int16_t a,
        int16_t b);

/*************************************************************************************************/

/* int_mult() */
static inline int16_t int_mult(
        int16_t a,
        int16_t b) {
    /* Shift right one less bit */
    int32_t c = (((int32_t)a * (int32_t)b) >> 14);

    /* Last bit shifted out = rounding bit */
    b = (c & 0x01);

    /* Last shift + rounding bit */
    a = ((int16_t)(c >> 1) + b);

    return a;
}

/*************************************************************************************************/

/* lrpt_dsp_ifft_init() */
lrpt_dsp_ifft_t *lrpt_dsp_ifft_init(
        uint16_t width,
        lrpt_error_t *err) {
    /* Try to allocate IFFT object */
    lrpt_dsp_ifft_t *ifft = malloc(sizeof(lrpt_dsp_ifft_t));

    if (!ifft) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Integer FFT object allocation failed");

        return NULL;
    }

    /* NULL-init internals for safe deallocation */
    ifft->sw_lut = NULL;

    /* Width should be a power of two (hence FFT order should be whole number) */
    if ((width == 0) || ((width & (width - 1)) != 0)) {
        lrpt_dsp_ifft_deinit(ifft);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "FFT width should be a power of 2");

        return NULL;
    }

    /* Initials */
    ifft->width = width;
    ifft->order = 0;

    size_t len = 2 * width;
    ifft->len = len;

    /* Determine FFT order */
    while (width >>= 1)
        ifft->order++;

    /* Allocate integer sinewave lookup table */
    ifft->sw_lut = calloc((len * 3) / 4, sizeof(int16_t));

    if (!ifft->sw_lut) {
        lrpt_dsp_ifft_deinit(ifft);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Sinewave lookup table allocation has failed");

        return NULL;
    }

    /* Initialize sinewave lookup table */
    for (size_t i = 0; i < ((len * 3) / 4); i++)
        ifft->sw_lut[i] = (int16_t)(32767 * sin(i * 2 * M_PI / len));

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return ifft;
}

/*************************************************************************************************/

/* lrpt_dsp_ifft_deinit() */
void lrpt_dsp_ifft_deinit(
        lrpt_dsp_ifft_t *ifft) {
    if (!ifft)
        return;

    free(ifft->sw_lut);

    free(ifft);
}

/*************************************************************************************************/

/* lrpt_dsp_ifft_exec() */
void lrpt_dsp_ifft_exec(
        const lrpt_dsp_ifft_t *ifft,
        int16_t *data) {
    /* This is heavily adapted version of original IFFT algorithm by Roberts-Slaney-Bouras */

    /* Decompose time domain signal (bit reversal). This is only needed if the time domain
     * points were not already decomposed while data has being entered into these buffers.
     */
    for (uint16_t i = 1; i < (ifft->width - 1); i++) {
        uint32_t t = 0;

        /* Bit reversal of indices */
        for (uint8_t j = 0; j < ifft->order; j++) {
            t <<= 1;
            t |= (i >> j) & 0x01;
        }

        /* Fill data to locations with bit reversed indices */
        if (i < t) {
            const uint32_t a1 = 2 * i;
            const uint32_t b1 = 2 * t;
            const uint32_t a2 = a1 + 1;
            const uint32_t b2 = b1 + 1;

            const int16_t tr = data[a1];
            const int16_t ti = data[a2];

            data[a1] = data[b1];
            data[a2] = data[b2];
            data[b1] = tr;
            data[b2] = ti;
        }
    }

    /* Compute the FFT */
    uint32_t a = 1;

    /* Loop over the number of decomposition (bit reversal) steps (the FFT order) */
    for (uint8_t i = 0; i < ifft->order; i++) {
        /* Loop over sub-IFFT's */
        uint32_t b = a;
        a <<= 1;

        for (uint32_t j = 0; j < b; j++) {
            uint16_t t = ((int16_t)(0x01 << (ifft->order - 1 - i)) * j * 2);
            int16_t wr = ifft->sw_lut[t + ifft->len / 4];
            int16_t wi = -ifft->sw_lut[t];

            /* Maintain scaling to avoid overflow */
            wr >>= 1;
            wi >>= 1;

            /* Loop over each butterfly and build up the frequency domain */
            for (uint16_t k = j; k < ifft->width; k += a ) {
                const uint32_t a1 = 2 * k;
                const uint32_t b1 = a1 + 1;
                const uint32_t a2 = 2 * (k + b);
                const uint32_t b2 = a2 + 1;

                const int16_t tr = int_mult(wr, data[a2]) - int_mult(wi, data[b2]);
                const int16_t ti = int_mult(wi, data[a2]) + int_mult(wr, data[b2]);

                int16_t qr = data[a1];
                int16_t qi = data[b1];

                qr >>= 1;
                qi >>= 1;

                data[a2] = qr - tr;
                data[b2] = qi - ti;
                data[a1] = qr + tr;
                data[b1] = qi + ti;
            }
        }
    }
}

/*************************************************************************************************/

/** \endcond */
