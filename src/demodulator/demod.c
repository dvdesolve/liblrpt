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

/* lrpt_demodulator_demod_qpsk()
 *
 * Performs plain QPSK demodulation.
 */
static bool lrpt_demodulator_demod_qpsk(complex double fdata, int8_t *buffer) {
}

/* lrpt_demodulator_demod_doqpsk()
 *
 * Performs differential offset QPSK demodulation.
 */
static bool lrpt_demodulator_demod_doqpsk(complex double fdata, int8_t *buffer) {
}

/* lrpt_demodulator_demod_idoqpsk()
 *
 * Performs interleaved differential offset QPSK demodulation.
 */
static bool lrpt_demodulator_demod_idoqpsk(complex double fdata, int8_t *buffer) {
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
        const double rrc_alpha) {
    /* Library defaults */
    const double AGC_TARGET = 180.0;

    /* Allocate our working handle */
    lrpt_demodulator_t *handle =
        (lrpt_demodulator_t *)malloc(sizeof(lrpt_demodulator_t));

    if (!handle)
        return NULL;

    /* Initialize demodulator parameters */
    handle->sym_rate = symbol_rate;
    handle->sym_period = (double)interp_factor * demod_samplerate / (double)symbol_rate;
    handle->interp_factor = interp_factor;

    /* Initialize AGC */
    handle->agc = lrpt_demodulator_agc_init(AGC_TARGET);

    if (!handle->agc) {
        lrpt_demodulator_deinit(handle);

        return NULL;
    }

    /* Initialize Costas PLL */
    double pll_bw = LRPT_M_2PI * costas_bandwidth / (double)symbol_rate;
    handle->pll = lrpt_demodulator_pll_init(pll_bw, mode);

    if (!handle->pll) {
        lrpt_demodulator_deinit(handle);

        return NULL;
    }

    /* Initialize RRC filter */
    double osf = demod_samplerate / (double)symbol_rate;
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
    }

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
    /* Library defaults */
    const double DSP_FILTER_RIPPLE = 5.0;
    const uint8_t DSP_FILTER_NUMPOLES = 6;

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
            DSP_FILTER_RIPPLE,
            DSP_FILTER_NUMPOLES,
            LRPT_DSP_FILTER_TYPE_LOWPASS
            );

    if (!filter)
        return false;

    /* Try to apply our filter */
    if (!lrpt_dsp_filter_apply(filter))
        return false;

    /* Free it ASAP */
    lrpt_dsp_filter_deinit(filter);

    /* Now we're ready to process filtered I/Q data and get soft-symbols */
    for (size_t i = 0; i < input->len; i++) {
        /* Make complex variable from filtered sample */
        complex double cdata = input->iq[i].i + input->iq[i].q * (complex double)I;

        for (uint8_t j = 0; j < handle->interp_factor; j++) {
            /* Pass samples through interpolator RRC filter */
            complex double fdata = lrpt_demodulator_rrc_filter_apply(handle->rrc, cdata);
//
//            /* Demodulate using appropriate function.
//             * Try to decode one or more LRPT frames when PLL is locked.
//             */
//            if (handle->demod_func(fdata, out_buffer) && handle->pll->locked) {
//                /* Try to decode one or more LRPT frames */
//                Decode_Image( (uint8_t *)out_buffer, SOFT_FRAME_LEN );
//
//                /* The mtd_record.pos and mtd_record.prev_pos pointers must be
//                 * decrimented to point back to the same data in the soft buffer */
//                mtd_record.pos      -= SOFT_FRAME_LEN;
//                mtd_record.prev_pos -= SOFT_FRAME_LEN;
//            }
        }
    }

    return true;
}
