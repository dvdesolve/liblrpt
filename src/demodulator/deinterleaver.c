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
 * Deinterleaver routines.
 *
 * This source file contains deinterleaver routines used by QPSK demodulator.
 */

/*************************************************************************************************/

#include "deinterleaver.h"

#include "../../include/lrpt.h"
#include "../liblrpt/lrpt.h"

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
static unsigned char qpsk_to_byte(
        const int8_t *data);

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
        const int8_t *data,
        size_t block_siz,
        size_t step,
        size_t depth,
        size_t *offset,
        unsigned char *sync);

/** Perform stream resyncing.
 *
 * Interleaved QPSK symbols stream with 80 kSym/s rate contains the following pattern:
 * 00100111 <36 bits> <36 bits> 00100111 <36 bits> <36 bits>...
 * Before passing QPSK data to the decoder the sync words must be removed and the stream
 * should be stitched back together.
 *
 * \param[in,out] data Pointer to the QPSK data storage.
 *
 * \return \c true on successfull resyncing and \c false otherwise.
 */
static bool resync_stream(
        lrpt_qpsk_data_t *data);

/*************************************************************************************************/

/* qpsk_to_byte() */
static unsigned char qpsk_to_byte(
        const int8_t *data) {
    unsigned char b = 0;

    for (uint8_t i = 0; i < 8; i++) {
        unsigned char bit = (data[i] < 0) ? 0 : 1;

        b |= bit << i;
    }

    return b;
}

/*************************************************************************************************/

/* find_sync() */
static bool find_sync(
        const int8_t *data,
        size_t block_siz,
        size_t step,
        size_t depth,
        size_t *offset,
        unsigned char *sync) {
    int limit;
    unsigned char test;
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
        lrpt_qpsk_data_t *data) {
    if ((data->len < SYNCD_BUF_MARGIN) || (data->len < INTLV_SYNCDATA))
        return false;

    /* Allocate temporary buffer for resyncing */
    int8_t *tmp_buf = calloc(data->len, 1);

    if (!tmp_buf)
        return false;

    /* Do a copy of the original data */
    memcpy(tmp_buf, data->qpsk, data->len);

    size_t resync_siz = 0;
    size_t posn = 0;
    size_t offset = 0;
    size_t limit1 = data->len - SYNCD_BUF_MARGIN;
    size_t limit2 = data->len - INTLV_SYNCDATA;

    /* Do while there is a room in the raw buffer for the find_sync() to search for
     * sync candidates
     */
    while (posn < limit1) {
        unsigned char sync;
        /* Only search for sync if look-forward below fails to find a sync train */
        if (!find_sync(
                    &tmp_buf[posn],
                    SYNCD_BLOCK_SIZ,
                    INTLV_SYNCDATA,
                    SYNCD_DEPTH,
                    &offset,
                    &sync)) {
            posn += SYNCD_BUF_STEP;
            continue;
        }

        posn += offset;

        /* Do while there is a room in the raw buffer to look forward for sync trains */
        while (posn < limit2) {
            /* Look ahead to prevent it losing sync on a weak signal */
            bool ok = false;

            for (size_t i = 0; i < 128; i++) {
                size_t tmp = posn + i * INTLV_SYNCDATA;

                if (tmp < limit2) {
                    unsigned char test = qpsk_to_byte(&tmp_buf[tmp]);

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
            memcpy(data->qpsk + resync_siz, &tmp_buf[posn + 8], INTLV_DATA_LEN);
            resync_siz += INTLV_DATA_LEN;

            /* Move on to the next sync train position */
            posn += INTLV_SYNCDATA;
        }
    }

    /* Free temporary buffer */
    free(tmp_buf);

    if (!lrpt_qpsk_data_resize(data, resync_siz))
        return false;

    return true;
}

/*************************************************************************************************/

/* lrpt_deinterleaver_exec() */
bool lrpt_deinterleaver_exec(
        lrpt_qpsk_data_t *data) {
    size_t old_size = data->len;
    int8_t *res_buf = NULL;

    /* Resynchronize raw data at the bottom of the raw buffer after the
     * INTLV_BRANCHES * INTLV_BASE_LEN and up to the end
     */
    if (!resync_stream(data))
        return false;

    /* Allocate resulting buffer */
    if ((data->len > 0) && (data->len < old_size)) {
        res_buf = calloc(data->len, 1);

        if (!res_buf)
            return false;
    }
    else
        return false;

    /* Perform convolutional deinterleaving. Please refer to the
     * https://en.wikipedia.org/wiki/Burst_error-correcting_code#Convolutional_interleaver
     */
    for (size_t i = 0; i < data->len; i++) {
        /* Offset by half a message to include leading and trailing fuzz */
        int64_t pos =
            i +
            (INTLV_BRANCHES - 1) * INTLV_DELAY -
            (i % INTLV_BRANCHES) * INTLV_BASE_LEN +
            (INTLV_BRANCHES / 2) * INTLV_BASE_LEN;

        if ((pos >= 0) && (pos < data->len))
            res_buf[pos] = data->qpsk[i];
    }

    /* Reassign pointers */
    free(data->qpsk);
    data->qpsk = res_buf;

    /* For some reason original code (above) was changed to the following, however it yields
     * trimmed data. I'll keep it here for the history.
     */
    /*for (size_t resync_idx = 0; resync_idx < *resync_siz; resync_idx++) {
        size_t raw_idx = resync_idx + (resync_idx % INTLV_BRANCHES) * INTLV_BASE_LEN;

        if (raw_idx < *resync_siz)
            (*resync)[resync_idx] = raw[raw_idx];
    }*/

    return true;
}

/*************************************************************************************************/

/** \endcond */
