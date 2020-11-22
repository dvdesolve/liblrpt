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
 * Different utils and routines for QPSK demodulator.
 *
 * This source file contains different routines used by QPSK demodulator.
 */

/*************************************************************************************************/

#include "utils.h"

#include "../../include/lrpt.h"
#include "../liblrpt/lrpt.h"
#include "demod.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* Library defaults */
static const size_t INTLV_BRANCHES = 36;
static const size_t INTLV_DELAY = 2048;
static const size_t INTLV_BASE_LEN = INTLV_BRANCHES * INTLV_DELAY;
static const size_t INTLV_DATA_LEN = 72; /* Number of interleaved symbols (usually 72 for Meteor) */
static const size_t INTLV_SYNC_LEN = 8; /* The length of sync word (usually 8 for Meteor) */
static const size_t INTLV_SYNCDATA = INTLV_DATA_LEN + INTLV_SYNC_LEN;

static const size_t SYNCD_DEPTH = 4; /* Number of consecutive sync words to search in stream */
static const size_t SYNCD_BUF_MARGIN = SYNCD_DEPTH * INTLV_SYNCDATA;
static const size_t SYNCD_BLOCK_SIZ = (SYNCD_DEPTH + 1) * INTLV_SYNCDATA;
static const size_t SYNCD_BUF_STEP = (SYNCD_DEPTH - 1) * INTLV_SYNCDATA;

/*************************************************************************************************/

/** Returns integer square root for given value.
 *
 * \param lut Initilized lookup table for sqrt().
 * \param value Input value.
 *
 * \return Integer square root of value.
 */
static inline int8_t lut_isqrt(
        const uint8_t lut[],
        int16_t value);

/** Make byte from QPSK data stream.
 *
 * Uses hard decision technique (thresholding) to produce an 8-bit byte for given QPSK data:
 * (only 8 consecutive QPSK soft symbols are used therefore \p data length should be at least 8).
 * Usually used to find a sync word for the resynchronizing function.
 *
 * \param data Pointer to the QPSK data.
 *
 * \return Byte representation for given QPSK data.
 */
static uint8_t qpsk_to_byte(
        const uint8_t *data);

/** Find sync in the data stream.
 *
 * The sync word could be in any of 8 different orientations, so we will just look for a repeating
 * bit pattern the right distance apart to find the position of a sync word (8-bit byte, 00100111,
 * repeating every 80 symbols in stream).
 *
 * \param data Pointer to the data stream to find sync in.
 * \param block_siz Synced block size.
 * \param step Step size to look for sync (usually equals to the sum of data and interleaver).
 * \param depth Determines how deep we should search for a sync word.
 * \param[out] offset Pointer to the final offset value. Contains valid value only of search
 * was successfull.
 * \param[out] sync Pointer to the final value of sync byte.
 *
 * \return \c true if sync was found and false otherwise.
 */
static bool find_sync(
        const uint8_t *data,
        size_t block_siz,
        size_t step,
        size_t depth,
        size_t *offset,
        uint8_t *sync);

/** Perform stream resyncing.
 *
 * Interleaved QPSK symbols stream with 80 kSym/s rate contains the following pattern:
 * 00100111 <36 bits> <36 bits> 00100111 <36 bits> <36 bits>...
 * Before passing QPSK data to the decoder the sync words must be removed and the stream
 * should be stitched back together.
 *
 * \param[in,out] raw_buf Pointer to the data buffer that will be processed.
 * \param raw_siz The length of the original data.
 * \param[out] resync_siz Pointer to the resulting size of resynced buffer.
 *
 * \return \c true on successfull resyncing and \c false otherwise.
 */
static bool resync_stream(
        uint8_t *raw_buf,
        size_t raw_siz,
        size_t *resync_siz);

/*************************************************************************************************/

/* lut_isqrt() */
static inline int8_t lut_isqrt(
        const uint8_t lut[],
        int16_t value) {
    if (value >= 0)
        return (int8_t)lut[value];
    else
        return -(int8_t)lut[-value];
}

/*************************************************************************************************/

/* qpsk_to_byte() */
static uint8_t qpsk_to_byte(
        const uint8_t *data) {
    uint8_t b = 0;

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (data[i] < 128) ? 1 : 0;

        b |= bit << i;
    }

    return b;
}

/*************************************************************************************************/

/* find_sync() */
static bool find_sync(
        const uint8_t *data,
        size_t block_siz,
        size_t step,
        size_t depth,
        size_t *offset,
        uint8_t *sync) {
    int limit;
    uint8_t test;
    bool result;

    *offset = 0;
    result  = false;

    /* Leave room in buffer for look-forward */
    limit = block_siz - step * depth;

    /* Search for a sync byte at the beginning of block */
    for (size_t i = 0; i < limit; i++) {
        result = true;

        /* Assemble a sync byte candidate */
        *sync = qpsk_to_byte(&data[i]);

        /* Search ahead depth times in buffer to see if there are exactly equal sync
         * byte candidates at intervals of (sync + data = 80 syms) blocks
         */
        for (size_t j = 1; j <= depth; j++) {
            /* Break if there is a mismatch at any position */
            test = qpsk_to_byte(&data[i + j * step]);

            if (*sync != test) {
                result = false;
                break;
            }
        }

        /* If an unbroken series of matching sync byte candidates located, record the buffer
         * offset and return the result
         */
        if (result) {
            *offset = i;
            break;
        }
    }

    return result;
}

/*************************************************************************************************/

/* resync_stream() */
static bool resync_stream(
        uint8_t *raw_buf,
        size_t raw_siz,
        size_t *resync_siz) {
    if ((raw_siz < SYNCD_BUF_MARGIN) || (raw_siz < INTLV_SYNCDATA))
        return false;

    /* Allocate temporary buffer for resyncing */
    uint8_t *src_buf = malloc(sizeof(uint8_t) * raw_siz); /* TODO use calloc */

    if (!src_buf)
        return false;

    /* Do a copy of the original data */
    memcpy(src_buf, raw_buf, raw_siz);

    *resync_siz = 0;

    size_t posn = 0;
    size_t offset = 0;
    size_t limit1 = raw_siz - SYNCD_BUF_MARGIN;
    size_t limit2 = raw_siz - INTLV_SYNCDATA;

    /* Do while there is a room in the raw buffer for the find_sync() to search for
     * sync candidates
     */
    while (posn < limit1) {
        uint8_t sync;
        /* Only search for sync if look-forward below fails to find a sync train */
        if (!find_sync(
                    &src_buf[posn],
                    SYNCD_BLOCK_SIZ,
                    INTLV_SYNCDATA,
                    SYNCD_DEPTH,
                    &offset,
                    &sync)) {
            posn += SYNCD_BUF_STEP;
            continue;
        }

        posn += offset;

        /* Do while there is room in the raw buffer to look forward for sync trains */
        while (posn < limit2) {
            /* Look ahead to prevent it losing sync on weak signal */
            bool ok = false;

            for (size_t i = 0; i < 128; i++) {
                size_t tmp = posn + i * INTLV_SYNCDATA;

                if (tmp < limit2) {
                    uint8_t test = qpsk_to_byte(&src_buf[tmp]);

                    if (sync == test) {
                        ok = true;
                        break;
                    }
                }
            }

            if (!ok)
                break;

            /* Copy the actual data after the sync train and update total number of
             * copied symbols
             */
            memcpy(&raw_buf[*resync_siz], &src_buf[posn + 8], INTLV_DATA_LEN);
            *resync_siz += INTLV_DATA_LEN;

            /* Move on to the next sync train position */
            posn += INTLV_SYNCDATA;
        }
    }

    /* Free temporary buffer */
    free(src_buf);

    return true;
}

/*************************************************************************************************/

/* lrpt_demodulator_lut_isqrt_init() */
/* TODO review */
bool lrpt_demodulator_lut_isqrt_init(
        uint8_t *lut) {
    lut = calloc(16385, sizeof(uint8_t));

    if (!lut)
        return false;

    for (uint16_t i = 0; i < 16385; i++)
        lut[i] = (uint8_t)sqrt((double)i);

    return true;
}

/*************************************************************************************************/

/* lrpt_demodulator_lut_isqrt_deinit() */
void lrpt_demodulator_lut_isqrt_deinit(
        uint8_t *lut) {
    free(lut);
}

/*************************************************************************************************/

/* lrpt_demodulator_dediffcode() */
bool lrpt_demodulator_dediffcode(
        lrpt_demodulator_t *demod,
        lrpt_qpsk_data_t *data) {
    /** \todo check for oqpsk mode explicitly */
    if (!data || data->len < 2 || (data->len % 2) != 0)
        return false;

    int8_t t1 = data->qpsk[0];
    int8_t t2 = data->qpsk[1];

    data->qpsk[0] = lut_isqrt(demod->lut_isqrt, data->qpsk[0] * demod->pr_I);
    data->qpsk[1] = lut_isqrt(demod->lut_isqrt, -(data->qpsk[1]) * demod->pr_Q);

    for (size_t i = 2; i <= (data->len - 2); i += 2) {
        int8_t x = data->qpsk[i];
        int8_t y = data->qpsk[i + 1];

        data->qpsk[i] = lut_isqrt(demod->lut_isqrt, data->qpsk[i] * t1);
        data->qpsk[i + 1] = lut_isqrt(demod->lut_isqrt, -(data->qpsk[i + 1]) * t2);

        t1 = x;
        t2 = y;
    }

    demod->pr_I = t1;
    demod->pr_Q = t2;

    return true;
}

/*************************************************************************************************/

/* lrpt_demodulator_deinterleave() */
bool lrpt_demodulator_deinterleave(
        uint8_t *raw,
        size_t raw_siz,
        uint8_t **resync,
        size_t *resync_siz) {
    /** \todo deal with data types, use internal library preferably (and for all internal functions) */
    /* Resynchronize raw data at the bottom of the raw buffer after the
     * INTLV_BRANCHES * INTLV_BASE_LEN and up to the end
     */
    if (!resync_stream(raw, raw_siz, resync_siz))
        return false;

    /* Allocate resulting buffer */
    if (*resync_siz && (*resync_siz < raw_siz)) {
        *resync = malloc(sizeof(int8_t) * *resync_siz); /* TODO use calloc */

        if (!resync)
            return false;
    }
    else
        return false;

    /* Deinterleave INTLV_BASE_LEN number of symbols, so that all symbols in raw buffer
     * up to this length are used up
     */
    for (size_t resync_idx = 0; resync_idx < *resync_siz; resync_idx++) {
        /* This is the convolutional interleaving algorithm used in reverse to deinterleave */
        size_t raw_idx = resync_idx + (resync_idx % INTLV_BRANCHES) * INTLV_BASE_LEN;

        if (raw_idx < *resync_siz)
            (*resync)[resync_idx] = raw[raw_idx];
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
