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
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * DSP routines.
 *
 * Various DSP routines.
 */

/*************************************************************************************************/

#include "dsp.h"

#include "../../include/lrpt.h"
#include "error.h"
#include "lrpt.h"

#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* For more information see section "6.2 Interleaving",
 * https://www-cdn.eumetsat.int/files/2020-04/pdf_mo_ds_esa_sy_0048_iss8.pdf
 */
static const uint8_t INTLV_BRANCHES = 36;
static const uint16_t INTLV_DELAY = 2048;
static const uint32_t INTLV_BASE_LEN = INTLV_BRANCHES * INTLV_DELAY;
static const uint8_t INTLV_DATA_LEN = 72; /* Number of interleaved symbols */
static const uint8_t INTLV_SYNC_LEN = 8; /* The length of sync word */
static const uint8_t INTLV_SYNCDATA = INTLV_DATA_LEN + INTLV_SYNC_LEN;

static const uint8_t SYNCD_DEPTH = 4; /* Number of consecutive sync words to search in stream */
static const uint16_t SYNCD_BUF_MARGIN = SYNCD_DEPTH * INTLV_SYNCDATA;
static const uint16_t SYNCD_BLOCK_SIZ = (SYNCD_DEPTH + 1) * INTLV_SYNCDATA;
static const uint16_t SYNCD_BUF_STEP = (SYNCD_DEPTH - 1) * INTLV_SYNCDATA;

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
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resyncing and \c false otherwise.
 */
static bool resync_stream(
        lrpt_qpsk_data_t *data,
        lrpt_error_t *err);

/*************************************************************************************************/

/* lut_isqrt() */
static inline int8_t lut_isqrt(
        const uint8_t lut[],
        int16_t value) {
    if (value >= 0)
        return lut[value];
    else
        return -lut[-value];
}

/*************************************************************************************************/

/* qpsk_to_byte() */
static uint8_t qpsk_to_byte(
        const int8_t *data) {
    uint8_t b = 0;

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (data[i] < 0) ? 0 : 1;

        b |= bit << i;
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
    bool result  = false;

    /* Search for a sync byte at the beginning of block */
    for (uint8_t i = 0; i < (SYNCD_BLOCK_SIZ - INTLV_SYNCDATA * SYNCD_DEPTH); i++) {
        result = true;

        /* Assemble a sync byte candidate */
        *sync = qpsk_to_byte(data + i);

        /* Search ahead SYNCD_DEPTH times in buffer to see if there are exactly equal sync
         * byte candidates at intervals of (sync + data = 80 syms) blocks
         */
        for (uint8_t j = 1; j <= SYNCD_DEPTH; j++) {
            /* Break if there is a mismatch at any position */
            uint8_t test = qpsk_to_byte(data + i + j * INTLV_SYNCDATA);

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
        lrpt_qpsk_data_t *data,
        lrpt_error_t *err) {
    if ((data->len < SYNCD_BUF_MARGIN) || (data->len < INTLV_SYNCDATA)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATACORR,
                    "Data length for resync is incorrect");

        return false;
    }

    /* Allocate temporary buffer for resyncing */
    int8_t *tmp_buf = calloc(data->len, sizeof(int8_t));

    if (!tmp_buf) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Temporary resync buffer allocation failed");

        return false;
    }

    /* Do a copy of the original data */
    memcpy(tmp_buf, data->qpsk, sizeof(int8_t) * data->len);

    size_t resync_siz = 0;
    size_t posn = 0;
    uint8_t offset = 0;
    size_t limit1 = data->len - SYNCD_BUF_MARGIN;
    size_t limit2 = data->len - INTLV_SYNCDATA;

    /* Do while there is a room in the raw buffer for the find_sync() to search for
     * sync candidates
     */
    while (posn < limit1) {
        uint8_t sync;

        /* Only search for sync if look-forward below fails to find a sync train */
        if (!find_sync(
                    tmp_buf + posn,
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

            for (uint8_t i = 0; i < 128; i++) {
                size_t tmp = posn + i * INTLV_SYNCDATA;

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

            /* Copy the actual data after the sync train and update total number of
             * copied symbols
             */
            memcpy(data->qpsk + resync_siz, tmp_buf + posn + 8, sizeof(int8_t) * INTLV_DATA_LEN);
            resync_siz += INTLV_DATA_LEN;

            /* Move on to the next sync train position */
            posn += INTLV_SYNCDATA;
        }
    }

    /* Free temporary buffer */
    free(tmp_buf);

    if (!lrpt_qpsk_data_resize(data, resync_siz, err))
        return false;

    return true;
}

/*************************************************************************************************/

/* lrpt_dsp_filter_init() */
lrpt_dsp_filter_t *lrpt_dsp_filter_init(
        uint32_t bandwidth,
        uint32_t samplerate,
        double ripple,
        uint8_t num_poles,
        lrpt_dsp_filter_type_t type,
        lrpt_error_t *err) {
    /* Try to allocate our filter object */
    lrpt_dsp_filter_t *filter = malloc(sizeof(lrpt_dsp_filter_t));

    if (!filter) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "DSP filter object allocation failed");

        return NULL;
    }

    /* NULL-init internal storage for safe deallocation */
    filter->a = NULL;
    filter->b = NULL;
    filter->x = NULL;
    filter->y = NULL;

    /* Number of poles should be even and not greater than 252 to fit in uint8_t type */
    if ((num_poles > 252) || ((num_poles % 2) != 0)) {
        lrpt_dsp_filter_deinit(filter);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "DSP filter number of poles is incorrect");

        return NULL;
    }

    filter->npoles = num_poles;
    filter->ri = 0;

    /* Allocate data and coefficients arrays */
    uint8_t n = (num_poles + 3);

    double * const ta = calloc(n, sizeof(double));
    double * const tb = calloc(n, sizeof(double));
    filter->a = calloc(n, sizeof(double));
    filter->b = calloc(n, sizeof(double));

    /* Allocate saved input and output arrays */
    n = (num_poles + 1);

    filter->x = calloc(n, sizeof(complex double));
    filter->y = calloc(n, sizeof(complex double));

    /* Check for allocation problems */
    if (!ta || !tb || !filter->a || !filter->b || !filter->x || !filter->y) {
        lrpt_dsp_filter_deinit(filter);

        free(ta);
        free(tb);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "DSP filter internal arrays allocation failed");

        return NULL;
    }

    /* Initialize specific coefficient arrays explicitly */
    filter->a[2] = 1.0;
    filter->b[2] = 1.0;

    /* S-domain to Z-domain conversion */
    const double t = (2.0 * tan(0.5));

    /* Cutoff frequency (as a fraction of sample rate) */
    const double w = (LRPT_M_2PI * (bandwidth / 2.0 / samplerate));

    /* Low Pass to Low Pass or Low Pass to High Pass transform */
    double k;

    if (type == LRPT_DSP_FILTER_TYPE_HIGHPASS)
        k = (-cos((w + 1.0) / 2.0) / cos((w - 1.0) / 2.0));
    else if (type == LRPT_DSP_FILTER_TYPE_LOWPASS)
        k = (sin((1.0 - w) / 2.0) / sin((1.0 + w) / 2.0));
    else
        k = 1.0;

    /* Find coefficients for 2-pole filter for each pole pair */
    for (uint8_t i = 1; i <= (num_poles / 2); i++) {
        /* Calculate the pole location on the unit circle */
        double tmp = (M_PI / (num_poles * 2.0) + M_PI * (i - 1) / num_poles);
        double rp = -cos(tmp);
        double ip = sin(tmp);

        /* Warp from a circle to an ellipse */
        if (ripple > 0.0) {
            tmp = (100.0 / (100.0 - ripple));

            const double es = sqrt(tmp * tmp - 1.0);

            tmp = (1.0 / num_poles);

            const double vx = (tmp * asinh(1.0 / es));
            double kx = (tmp * acosh(1.0 / es));

            kx = cosh(kx);
            rp *= (sinh(vx) / kx);
            ip *= (cosh(vx) / kx);
        }

        /* S-domain to Z-domain conversion */
        const double m = (rp * rp + ip * ip);
        double d = (4.0 - 4.0 * rp * t + m * t * t);
        const double xn0 = (t * t / d);
        const double xn1 = (2.0 * t * t / d);
        const double xn2 = (t * t / d);
        const double yn1 = ((8.0 - 2.0 * m * t * t) / d);
        const double yn2 = ((-4.0 - 4.0 * rp * t - m * t * t) / d);

        /* (Low Pass to Low Pass) or (Low Pass to High Pass) transform */
        d = (1.0 + yn1 * k - yn2 * k * k);

        const double a0 = ((xn0 - xn1 * k + xn2 * k * k) / d);
        double a1 = ((-2.0 * xn0 * k + xn1 + xn1 * k * k - 2.0 * xn2 * k) / d);
        const double a2 = ((xn0 * k * k - xn1 * k + xn2) / d);
        double b1 = ((2.0 * k + yn1 + yn1 * k * k - 2.0 * yn2 * k) / d);
        const double b2 = ((-k * k - yn1 * k + yn2) / d);

        if (type == LRPT_DSP_FILTER_TYPE_HIGHPASS) {
            a1 = -a1;
            b1 = -b1;
        }

        /* Add coefficients to the cascade */
        for (uint8_t j = 0; j < (num_poles + 3); j++) {
            ta[j] = filter->a[j];
            tb[j] = filter->b[j];
        }

        for (uint8_t j = 2; j < (num_poles + 3); j++) {
            filter->a[j] = (a0 * ta[j] + a1 * ta[j - 1] + a2 * ta[j - 2]);
            filter->b[j] = (tb[j] - b1 * tb[j - 1] - b2 * tb[j - 2]);
        }
    }

    /* Free temporary arrays */
    free(ta);
    free(tb);

    /* Finish combining coefficients */
    filter->b[2] = 0.0;

    for (uint8_t i = 0; i < (num_poles + 1); i++) {
        filter->a[i] = filter->a[i + 2];
        filter->b[i] = -filter->b[i + 2];
    }

    /* Normalize the gain */
    double sa = 0.0;
    double sb = 0.0;

    for (uint8_t i = 0; i < (num_poles + 1); i++) {
        if (type == LRPT_DSP_FILTER_TYPE_LOWPASS) {
            sa += filter->a[i];
            sb += filter->b[i];
        }
        else if (type == LRPT_DSP_FILTER_TYPE_HIGHPASS) {
            sa += (filter->a[i] * ((-1) ^ i));
            sb += (filter->b[i] * ((-1) ^ i));
        }
    }

    const double gain = (sa / (1.0 - sb));

    for (uint8_t i = 0; i < (num_poles + 1); i++)
        filter->a[i] /= gain;

    return filter;
}

/*************************************************************************************************/

/* lrpt_dsp_filter_deinit() */
void lrpt_dsp_filter_deinit(
        lrpt_dsp_filter_t *filter) {
    if (!filter)
        return;

    free(filter->a);
    free(filter->b);
    free(filter->x);
    free(filter->y);

    free(filter);
}

/*************************************************************************************************/

/* lrpt_dsp_filter_apply() */
bool lrpt_dsp_filter_apply(
        lrpt_dsp_filter_t *filter,
        lrpt_iq_data_t *data) {
    /* Return immediately if filter is empty */
    if (!filter)
        return false;

    /* For convenient access purposes */
    const uint8_t npp1 = (filter->npoles + 1);
    complex double * const samples = data->iq;

    /* Filter samples in the buffer */
    for (size_t i = 0; i < data->len; i++) {
        complex double *cur_s = (samples + i);

        /* Calculate and save filtered samples */
        complex double yn0 = (*cur_s * filter->a[0]);

        for (uint8_t j = 1; j < npp1; j++) {
            /* Summate contribution of past input samples */
            yn0 += (filter->x[filter->ri] * filter->a[j]);

            /* Summate contribution of past output samples */
            yn0 += (filter->y[filter->ri] * filter->b[j]);

            /* Advance ring buffers index */
            filter->ri++;

            if (filter->ri >= npp1)
                filter->ri = 0;
        }

        /* Save new yn0 output to y ring buffer */
        filter->y[filter->ri] = yn0;

        /* Save current input sample to x ring buffer */
        filter->x[filter->ri] = *cur_s;

        /* Return filtered samples */
        *cur_s = yn0;
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_dsp_dediffcoder_init() */
lrpt_dsp_dediffcoder_t *lrpt_dsp_dediffcoder_init(
        lrpt_error_t *err) {
    /* Allocate dediffcoder object */
    lrpt_dsp_dediffcoder_t *dediff = malloc(sizeof(lrpt_dsp_dediffcoder_t));

    if (!dediff) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Dediffcoder object allocation failed");

        return NULL;
    }

    /* Try to allocate lookup table */
    dediff->lut = calloc(16385, sizeof(uint8_t));

    if (!dediff->lut) {
        free(dediff);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Dediffcoder lookup table allocation failed");

        return NULL;
    }

    for (uint16_t i = 0; i < 16385; i++)
        dediff->lut[i] = sqrt(i);

    dediff->pr_I = 0;
    dediff->pr_Q = 0;

    return dediff;
}

/*************************************************************************************************/

/* lrpt_dsp_dediffcoder_deinit() */
void lrpt_dsp_dediffcoder_deinit(
        lrpt_dsp_dediffcoder_t *dediff) {
    if (!dediff)
        return;

    free(dediff->lut);
    free(dediff);
}

/*************************************************************************************************/

/* lrpt_dsp_dediffcoder_exec() */
bool lrpt_dsp_dediffcoder_exec(
        lrpt_dsp_dediffcoder_t *dediff,
        lrpt_qpsk_data_t *data,
        lrpt_error_t *err) {
    if (!data || data->len < 2 || (data->len % 2) != 0) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object is corrupted");

        return false;
    }

    int8_t t1 = data->qpsk[0];
    int8_t t2 = data->qpsk[1];

    data->qpsk[0] = lut_isqrt(dediff->lut, data->qpsk[0] * dediff->pr_I);
    data->qpsk[1] = lut_isqrt(dediff->lut, -(data->qpsk[1]) * dediff->pr_Q);

    for (size_t i = 2; i <= (data->len - 2); i += 2) {
        int8_t x = data->qpsk[i];
        int8_t y = data->qpsk[i + 1];

        data->qpsk[i] = lut_isqrt(dediff->lut, data->qpsk[i] * t1);
        data->qpsk[i + 1] = lut_isqrt(dediff->lut, -(data->qpsk[i + 1]) * t2);

        t1 = x;
        t2 = y;
    }

    dediff->pr_I = t1;
    dediff->pr_Q = t2;

    return true;
}

/*************************************************************************************************/

/* lrpt_dsp_deinterleaver_exec() */
bool lrpt_dsp_deinterleaver_exec(
        lrpt_qpsk_data_t *data,
        lrpt_error_t *err) {
    size_t old_size = data->len;
    int8_t *res_buf = NULL;

    /* Resynchronize raw data at the bottom of the raw buffer after the
     * INTLV_BRANCHES * INTLV_BASE_LEN and up to the end
     */
    if (!resync_stream(data, err))
        return false;

    /* Allocate resulting buffer */
    if ((data->len > 0) && (data->len < old_size)) {
        res_buf = calloc(data->len, sizeof(int8_t));

        if (!res_buf) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                        "Resulting buffer allocation failed");

            return false;
        }
    }
    else {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATACORR,
                    "Resynced data length is incorrect");

        return false;
    }

    /* Perform convolutional deinterleaving */
    /* https://en.wikipedia.org/wiki/Burst_error-correcting_code#Convolutional_interleaver */
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

    return true;
}

/*************************************************************************************************/

/** \endcond */
