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
 * Deinterleaver routines.
 */

/*************************************************************************************************/

#include "deinterleaver.h"

#include "../../include/lrpt.h"
#include "../liblrpt/datatype.h"
#include "../liblrpt/error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* For more information see section "6.2 Interleaving",
 * https://www-cdn.eumetsat.int/files/2020-04/pdf_mo_ds_esa_sy_0048_iss8.pdf
 */
static const uint8_t DEINT_INTLV_BRANCHES = 36;
static const uint16_t DEINT_INTLV_DELAY = 2048;
static const uint32_t DEINT_INTLV_BASE_LEN = DEINT_INTLV_BRANCHES * DEINT_INTLV_DELAY;
static const uint8_t DEINT_INTLV_DATA_LEN = 72; /* Number of interleaved bits */
static const uint8_t DEINT_INTLV_SYNC_LEN = 8; /* The length of sync word (in bits) */
static const uint8_t DEINT_INTLV_SYNCDATA = DEINT_INTLV_DATA_LEN + DEINT_INTLV_SYNC_LEN;

static const uint8_t DEINT_SYNCD_DEPTH = 4; /* Number of consecutive sync words to search */
static const uint16_t DEINT_SYNCD_BUF_MARGIN = DEINT_SYNCD_DEPTH * DEINT_INTLV_SYNCDATA;
static const uint16_t DEINT_SYNCD_BLOCK_SIZ = (DEINT_SYNCD_DEPTH + 1) * DEINT_INTLV_SYNCDATA;
static const uint16_t DEINT_SYNCD_BUF_STEP = (DEINT_SYNCD_DEPTH - 1) * DEINT_INTLV_SYNCDATA;

/*************************************************************************************************/

/** Make byte from QPSK data stream.
 *
 * Uses hard decision technique (thresholding) to produce an 8-bit byte for given QPSK data:
 * (only 8 consecutive QPSK bytes are used therefore \p data length should be at least 8).
 * Usually used to find a sync word for the resynchronizing function.
 *
 * \param data Pointer to the QPSK data.
 *
 * \return Byte representation for given QPSK data.
 */
static uint8_t qpsk_to_byte(
        const int8_t *data);

/** Find sync in the data stream.
 *
 * The sync word could be in any of 8 different orientations, so we will just look for a repeating
 * bit pattern the right distance apart to find the position of a sync word (8-bit byte, 00100111,
 * repeating every 80 symbols in stream).
 *
 * \param data Pointer to the data stream to find sync in.
 * \param[out] offset Pointer to the final offset value. Contains valid value only if search
 * was successfull.
 * \param[out] sync Pointer to the final value of sync byte.
 *
 * \return \c true if sync was found and false otherwise.
 */
static bool find_sync(
        const int8_t *data,
        uint8_t *offset,
        uint8_t *sync);

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
static uint8_t qpsk_to_byte(
        const int8_t *data) {
    uint8_t b = 0;

    /* Assume little-endian conversion */
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (data[i] < 0) ? 0 : 1;

        b |= (bit << i);
    }

    return b;
}

/*************************************************************************************************/

/* find_sync() */
static bool find_sync(
        const int8_t *data,
        uint8_t *offset,
        uint8_t *sync) {
    *offset = 0;
    bool result = false;

    /* Search for a sync byte at the beginning of block */
    for (
            uint8_t i = 0;
            i < (DEINT_SYNCD_BLOCK_SIZ - DEINT_INTLV_SYNCDATA * DEINT_SYNCD_DEPTH);
            i++) {
        result = true;

        /* Assemble a sync byte candidate */
        *sync = qpsk_to_byte(data + i);

        /* Search ahead DEINT_SYNCD_DEPTH times in buffer to see if there are exactly equal sync
         * byte candidates at intervals of (sync + data = 80 syms) blocks
         */
        for (uint8_t j = 1; j <= DEINT_SYNCD_DEPTH; j++) {
            /* Break if there is a mismatch at any position */
            uint8_t test = qpsk_to_byte(data + i + j * DEINT_INTLV_SYNCDATA);

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
    if (((2 * data->len) < DEINT_SYNCD_BUF_MARGIN) || ((2 * data->len) < DEINT_INTLV_SYNCDATA))
        return false;

    /* Allocate temporary buffer for resyncing */
    int8_t *tmp_buf = calloc(2 * data->len, sizeof(int8_t));

    if (!tmp_buf)
        return false;

    /* Do a copy of the original data */
    memcpy(tmp_buf, data->qpsk, sizeof(int8_t) * 2 * data->len);

    size_t resync_size = 0;
    size_t posn = 0;
    uint8_t offset = 0;
    size_t limit1 = 2 * data->len - DEINT_SYNCD_BUF_MARGIN;
    size_t limit2 = 2 * data->len - DEINT_INTLV_SYNCDATA;

    /* Do while there is a room in the raw buffer for the find_sync() to search for
     * sync candidates
     */
    while (posn < limit1) {
        uint8_t sync;

        /* Only search for sync if look-forward below fails to find a sync train */
        if (!find_sync(tmp_buf + posn, &offset, &sync)) {
            posn += DEINT_SYNCD_BUF_STEP;

            continue;
        }

        posn += offset;

        /* Do while there is a room in the raw buffer to look forward for sync trains */
        while (posn < limit2) {
            /* Look ahead to prevent it losing sync on a weak signal */
            bool ok = false;

            for (uint8_t i = 0; i < 128; i++) {
                size_t tmp = posn + i * DEINT_INTLV_SYNCDATA;

                if (tmp < limit2) {
                    uint8_t test = qpsk_to_byte(tmp_buf + tmp);

                    if (sync == test) {
                        ok = true;

                        break;
                    }
                }
            }

            if (!ok)
                break;

            /* Copy the actual data after the sync train (8 bits) and update total number of
             * copied symbols
             */
            memcpy(
                    data->qpsk + resync_size,
                    tmp_buf + posn + 8,
                    sizeof(int8_t) * DEINT_INTLV_DATA_LEN);
            resync_size += DEINT_INTLV_DATA_LEN;

            /* Move on to the next sync train position */
            posn += DEINT_INTLV_SYNCDATA;
        }
    }

    /* Free temporary buffer */
    free(tmp_buf);

    if (!lrpt_qpsk_data_resize(data, resync_size / 2, NULL))
        return false;

    return true;
}

/*************************************************************************************************/

/* lrpt_dsp_deinterleaver_init() */
lrpt_dsp_deinterleaver_t *lrpt_dsp_deinterleaver_init(
        lrpt_error_t *err) {
    /* Allocate deinterleaver object */
    lrpt_dsp_deinterleaver_t *deintlv = malloc(sizeof(lrpt_dsp_deinterleaver_t));

    if (!deintlv) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Deinterleaver object allocation has failed");

        return NULL;
    }

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return deintlv;
}

/*************************************************************************************************/

/* lrpt_dsp_deinterleaver_deinit() */
void lrpt_dsp_deinterleaver_deinit(
        lrpt_dsp_deinterleaver_t *deintlv) {
    free(deintlv);
}

/*************************************************************************************************/

/* lrpt_dsp_deinterleaver_exec() */
bool lrpt_dsp_deinterleaver_exec(
        lrpt_dsp_deinterleaver_t *deintlv,
        lrpt_qpsk_data_t *data,
        lrpt_error_t *err) {
    if (!data) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_INVOBJ,
                    "QPSK data object is NULL");

        return false;
    }

    size_t old_size = data->len;
    int8_t *res_buf = NULL;

    /* Resynchronize raw data at the bottom of the raw buffer after the
     * DEINT_INTLV_BRANCHES * DEINT_INTLV_BASE_LEN and up to the end
     */
    if (!resync_stream(data)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                    "Can't resynchronize QPSK data stream");

        return false;
    }

    /* Allocate resulting buffer */
    if ((data->len > 0) && (data->len < old_size)) {
        res_buf = calloc(2 * data->len, sizeof(int8_t));

        if (!res_buf) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Deinterleaved data buffer allocation has failed");

            return false;
        }
    }
    else {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                    "Resynchronized data length is incorrect");

        return false;
    }

    /* Perform convolutional deinterleaving */
    /* https://en.wikipedia.org/wiki/Burst_error-correcting_code#Convolutional_interleaver */
    for (size_t i = 0; i < (2 * data->len); i++) {
        /* Offset by half a message to include leading and trailing fuzz */
        int64_t pos =
            i +
            (DEINT_INTLV_BRANCHES - 1) * DEINT_INTLV_DELAY -
            (i % DEINT_INTLV_BRANCHES) * DEINT_INTLV_BASE_LEN +
            (DEINT_INTLV_BRANCHES / 2) * DEINT_INTLV_BASE_LEN;

        if ((pos >= 0) && (pos < (2 * data->len)))
            res_buf[pos] = data->qpsk[i];
    }

    /* Reassign pointers */
    free(data->qpsk);
    data->qpsk = res_buf;

    if (err)
        lrpt_error_set(err, LRPT_ERR_LVL_NONE, LRPT_ERR_CODE_NONE, NULL);

    return true;
}

/*************************************************************************************************/

/** \endcond */
