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

/*************************************************************************************************/

#include "demod.h"

#include "../../include/lrpt.h"
#include "../liblrpt/dsp.h"
#include "../liblrpt/lrpt.h"
#include "agc.h"
#include "pll.h"
#include "rrc.h"
#include "utils.h"

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/* Library defaults */
const double DEMOD_RESYNC_SCALE_QPSK = 2000000.0;

const double DEMOD_AGC_TARGET = 180.0;

const double DEMOD_DSP_FILTER_RIPPLE = 5.0;
const uint8_t DEMOD_DSP_FILTER_NUMPOLES = 6;

/*************************************************************************************************/

static inline int8_t lrpt_demodulator_clamp_int8(const double x);

/*************************************************************************************************/

/* lrpt_demodulator_clamp_int8()
 *
 * Clamps double value to the int8_t range.
 */
static inline int8_t lrpt_demodulator_clamp_int8(const double x) {
    if (x < -128.0)
        return -128;
    else if (x > 127.0)
        return 127;
    else if ((x > 0.0) && (x < 1.0))
        return 1;
    else if ((x > -1.0) && (x < 0.0))
        return -1;
    else
        return (int8_t)x;
}

/*************************************************************************************************/

/* lrpt_demodulator_demod_qpsk()
 *
 * Performs plain QPSK demodulation.
 */
static bool lrpt_demodulator_demod_qpsk(
        lrpt_demodulator_t *handle,
        const complex double fdata,
        int8_t *buffer) {
    /* Helper variables */
    const double sym_period = handle->sym_period;
    const double sp2 = sym_period / 2.0;
    const double sp2p1 = sp2 + 1.0;

    /* Symbol timing recovery (Gardner) */
    if ((handle->resync_offset >= sp2) && (handle->resync_offset < sp2p1))
        handle->middle = lrpt_demodulator_agc_apply(handle->agc, fdata);
    else if (handle->resync_offset >= sym_period) {
        handle->current = lrpt_demodulator_agc_apply(handle->agc, fdata);

        handle->resync_offset -= sym_period;

        const double resync_error =
            (cimag(handle->current) - cimag(handle->before)) * cimag(handle->middle);
        handle->resync_offset += (resync_error * sym_period / DEMOD_RESYNC_SCALE_QPSK);

        handle->before = handle->current;

        /* Costas' loop frequency/phase tuning */
        handle->current = lrpt_demodulator_pll_mix(handle->pll, handle->current);
        const double delta =
            lrpt_demodulator_pll_delta(handle->pll, handle->current, handle->current);
        lrpt_demodulator_pll_correct_phase(handle->pll, delta, handle->interp_factor);

        handle->resync_offset += 1.0;

        /* Save result in buffer */
        buffer[(handle->buf_idx)++] = lrpt_demodulator_clamp_int8(creal(handle->current) / 2.0);
        buffer[(handle->buf_idx)++] = lrpt_demodulator_clamp_int8(cimag(handle->current) / 2.0);

        /* If we've reached frame length reset buffer index and signal caller with true */
        if (handle->buf_idx >= LRPT_SOFT_FRAME_LEN) {
            handle->buf_idx = 0;

            return true;
        }
        else
            return false;
    }

    /* Initialize internal variables for routines */
    handle->resync_offset += 1.0;

    return false;
}

/* lrpt_demodulator_demod_doqpsk()
 *
 * Performs differential offset QPSK demodulation.
 */
static bool lrpt_demodulator_demod_doqpsk(
        lrpt_demodulator_t *handle,
        const complex double fdata,
        int8_t *buffer) {
    return false;
}

/* lrpt_demodulator_demod_idoqpsk()
 *
 * Performs interleaved differential offset QPSK demodulation.
 */
static bool lrpt_demodulator_demod_idoqpsk(
        lrpt_demodulator_t *handle,
        const complex double fdata,
        int8_t *buffer) {
    return false;
}

/*************************************************************************************************/

/* lrpt_demodulator_init()
 *
 * Allocates and initializes demodulator object. If allocation was unsuccessfull then <NULL>
 * will be returned.
 */
lrpt_demodulator_t *lrpt_demodulator_init(
        const lrpt_demodulator_mode_t mode,
        const double costas_bandwidth,
        const uint8_t interp_factor,
        const double demod_samplerate,
        const uint32_t symbol_rate,
        const uint16_t rrc_order,
        const double rrc_alpha,
        const double pll_threshold) {
    /* Allocate our working handle */
    lrpt_demodulator_t *handle =
        (lrpt_demodulator_t *)malloc(sizeof(lrpt_demodulator_t));

    if (!handle)
        return NULL;

    /* NULL-init internal objects for safe deallocation */
    handle->agc = NULL;
    handle->pll = NULL;
    handle->rrc = NULL;
    handle->lut_isqrt = NULL;

    /* Initialize demodulator parameters */
    handle->sym_rate = symbol_rate;
    handle->sym_period = (double)interp_factor * demod_samplerate / (double)symbol_rate;
    handle->interp_factor = interp_factor;

    /* Initialize AGC object */
    handle->agc = lrpt_demodulator_agc_init(DEMOD_AGC_TARGET);

    if (!handle->agc) {
        lrpt_demodulator_deinit(handle);

        return NULL;
    }

    /* Initialize Costas' PLL object */
    const double pll_bw = LRPT_M_2PI * costas_bandwidth / (double)symbol_rate;
    handle->pll = lrpt_demodulator_pll_init(pll_bw, pll_threshold, mode);

    if (!handle->pll) {
        lrpt_demodulator_deinit(handle);

        return NULL;
    }

    /* Initialize RRC filter object */
    const double osf = demod_samplerate / (double)symbol_rate;
    handle->rrc = lrpt_demodulator_rrc_filter_init(rrc_order, interp_factor, osf, rrc_alpha);

    if (!handle->rrc) {
        lrpt_demodulator_deinit(handle);

        return NULL;
    }

    /* Select demodulator function */
    switch (mode) {
        case LRPT_DEMODULATOR_MODE_QPSK:
            handle->lut_isqrt = NULL;
            handle->demod_func = lrpt_demodulator_demod_qpsk;

            break;

        case LRPT_DEMODULATOR_MODE_DOQPSK:
            if (!lrpt_demodulator_lut_isqrt_init(handle->lut_isqrt)) {
                lrpt_demodulator_deinit(handle);

                return NULL;
            }

            handle->demod_func = lrpt_demodulator_demod_doqpsk;

            break;

        case LRPT_DEMODULATOR_MODE_IDOQPSK:
            if (!lrpt_demodulator_lut_isqrt_init(handle->lut_isqrt)) {
                lrpt_demodulator_deinit(handle);

                return NULL;
            }

            handle->demod_func = lrpt_demodulator_demod_idoqpsk;

            break;

        /* All other modes are unsupported */
        default:
            lrpt_demodulator_deinit(handle);

            return NULL;

            break;
    }

    /* Initialize internal working variables */
    handle->resync_offset = 0.0;
    handle->before = 0.0;
    handle->middle = 0.0;
    handle->current = 0.0;
    handle->buf_idx = 0;

    return handle;
}

/*************************************************************************************************/

/* lrpt_demodulator_deinit()
 *
 * Frees allocated demodulator object.
 */
void lrpt_demodulator_deinit(lrpt_demodulator_t *handle) {
    if (!handle)
        return;

    lrpt_demodulator_lut_isqrt_deinit(handle->lut_isqrt);
    lrpt_demodulator_rrc_filter_deinit(handle->rrc);
    lrpt_demodulator_pll_deinit(handle->pll);
    lrpt_demodulator_agc_deinit(handle->agc);
    free(handle);
}

/*************************************************************************************************/

/* lrpt_demodulator_exec()
 *
 * Demodulates given <input> I/Q samples and stores QPSK soft-symbol samples in <output>.
 * Uses demodulator object provided with <handle>.
 * <output> will be resized automatically to fit necessary chunk of QPSK soft symbols.
 */
bool lrpt_demodulator_exec(
        lrpt_demodulator_t *handle,
        lrpt_iq_data_t *input,
        lrpt_qpsk_data_t *output) {
    /* Return immediately if no valid input or output were given */
    if (!input || !output)
        return false;

    /* Resize output data structure if it is more than twice shorter than input I/Q data */
    if (output->len < (2 * input->len))
        if (!lrpt_qpsk_data_resize(output, 2 * input->len))
            return false;

    /* Prepare DSP filter */
    lrpt_dsp_filter_t *filter = lrpt_dsp_filter_init(
            input, /* I/Q storage */
            115000, /* Bandwidth of LRPT signal according to the config file */
            ((double)1024000 / (double)2), /* Sample rate (RTLSDR uses 1024000 samples/sec and
                                              decimation of 2
                                            */
            DEMOD_DSP_FILTER_RIPPLE,
            DEMOD_DSP_FILTER_NUMPOLES,
            LRPT_DSP_FILTER_TYPE_LOWPASS
            );

    if (!filter)
        return false;

    /* Try to apply our filter */
    if (!lrpt_dsp_filter_apply(filter))
        return false;

    /* Free it ASAP */
    lrpt_dsp_filter_deinit(filter);

    /* Allocate output buffer */
    /* TODO may be move inside demodulator object */
    int8_t *out_buffer = (int8_t *)calloc(LRPT_SOFT_FRAME_LEN, sizeof(int8_t));

    if (!out_buffer)
        return false;

    /* Now we're ready to process filtered I/Q data and get soft-symbols */
    for (size_t i = 0; i < input->len; i++) {
        /* Make complex variable from filtered sample */
        complex double cdata = input->iq[i].i + input->iq[i].q * (complex double)I;

        for (uint8_t j = 0; j < handle->interp_factor; j++) {
            /* Pass samples through interpolator RRC filter */
            complex double fdata = lrpt_demodulator_rrc_filter_apply(handle->rrc, cdata);

            /* Demodulate using appropriate function */
            if (handle->demod_func(handle, fdata, out_buffer) && handle->pll->locked) {
//                /* Try to decode one or more LRPT frames */
//                Decode_Image( (uint8_t *)out_buffer, SOFT_FRAME_LEN );
//
//                /* The mtd_record.pos and mtd_record.prev_pos pointers must be
//                 * decrimented to point back to the same data in the soft buffer */
//                mtd_record.pos      -= SOFT_FRAME_LEN;
//                mtd_record.prev_pos -= SOFT_FRAME_LEN;
            }
        }
    }

    /* Free output buffer */

    return true;
}
