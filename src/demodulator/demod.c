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
 * Demodulation routines.
 *
 * This source file contains routines for performing demodulation of QPSK signals
 * (and all derivatives).
 */

/*************************************************************************************************/

#include "demod.h"

#include "../../include/lrpt.h"
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
static const double DEMOD_RESYNC_SCALE_QPSK = 2000000.0;
static const double DEMOD_RESYNC_SCALE_OQPSK = 2000000.0;

static const double DEMOD_AGC_TARGET = 180.0;

/*************************************************************************************************/

/** Clamps double value to the int8_t range.
 *
 * \param x Input value.
 *
 * \return Value clamped to the int8_t range (typically [-128; 127]).
 */
static inline int8_t clamp_int8(
        double x);

/** Performs plain QPSK demodulation.
 *
 * \param demod Demodulator object.
 * \param fdata I/Q sample.
 * \param[out] buffer Output buffer.
 *
 * \return \c true when #LRPT_SOFT_FRAME_LEN QPSK symbols were demodulated and \c false otherwise.
 */
static bool demod_qpsk(
        lrpt_demodulator_t *demod,
        complex double fdata,
        int8_t *buffer);

/** Performs offset QPSK demodulation.
 *
 * \param demod Demodulator object.
 * \param fdata I/Q sample.
 * \param[out] buffer Output buffer.
 *
 * \return \c true when #LRPT_SOFT_FRAME_LEN QPSK symbols were demodulated and \c false otherwise.
 */
static bool demod_oqpsk(
        lrpt_demodulator_t *demod,
        complex double fdata,
        int8_t *buffer);

/*************************************************************************************************/

/* clamp_int8() */
static inline int8_t clamp_int8(
        double x) {
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

/* demod_qpsk() */
static bool demod_qpsk(
        lrpt_demodulator_t *demod,
        complex double fdata,
        int8_t *buffer) {
    /** \todo check for qpsk mode explicitly */
    /** \todo deal with int8_t vs uint8_t data types */
    /* Helper variables */
    const double sym_period = demod->sym_period;
    const double sp2 = sym_period / 2.0;
    const double sp2p1 = sp2 + 1.0;

    /* Symbol timing recovery (Gardner) */
    if ((demod->resync_offset >= sp2) && (demod->resync_offset < sp2p1))
        demod->middle = lrpt_demodulator_agc_apply(demod->agc, fdata);
    else if (demod->resync_offset >= sym_period) {
        complex double current = lrpt_demodulator_agc_apply(demod->agc, fdata);

        demod->resync_offset -= sym_period;

        const double resync_error =
            (cimag(current) - cimag(demod->before)) * cimag(demod->middle);

        demod->resync_offset += (resync_error * sym_period / DEMOD_RESYNC_SCALE_QPSK);
        demod->before = current;

        /* Costas' loop frequency/phase tuning */
        current = lrpt_demodulator_pll_mix(demod->pll, current);

        const double delta = lrpt_demodulator_pll_delta(demod->pll, current, current);

        lrpt_demodulator_pll_correct_phase(demod->pll, delta, demod->interp_factor);
        demod->resync_offset += 1.0;

        /* Save result in buffer */
        /* TODO may be ring buffer will be more appropriate so there will be no need in SOFT_FRAME_LEN */
        /* TODO may be it's better to just return soft-symbols directly; wrapper should be responsible to store them where necessary */
        buffer[(demod->buf_idx)++] = clamp_int8(creal(current) / 2.0);
        buffer[(demod->buf_idx)++] = clamp_int8(cimag(current) / 2.0);

        /* If we've reached frame length reset buffer index and signal caller with true */
        if (demod->buf_idx >= LRPT_SOFT_FRAME_LEN) {
            demod->buf_idx = 0;

            return true;
        }
        else
            return false;
    }

    demod->resync_offset += 1.0;

    return false;
}

/*************************************************************************************************/

/* demod_oqpsk() */
static bool demod_oqpsk(
        lrpt_demodulator_t *demod,
        complex double fdata,
        int8_t *buffer) {
    /** \todo check for oqpsk mode explicitly */
    /** \todo deal with int8_t vs uint8_t data types */
    /* Helper variables */
    const double sym_period = demod->sym_period;
    const double sp2 = sym_period / 2.0;
    const double sp2p1 = sp2 + 1.0;

    /* Symbol timing recovery (Gardner) */
    if ((demod->resync_offset >= sp2) && (demod->resync_offset < sp2p1)) {
        complex double agc = lrpt_demodulator_agc_apply(demod->agc, fdata);

        demod->inphase = lrpt_demodulator_pll_mix(demod->pll, agc);
        demod->middle = demod->prev_I + cimag(demod->inphase) * I;
        demod->prev_I = creal(demod->inphase);
    }
    else if (demod->resync_offset >= sym_period) {
        /* Symbol timing recovery (Gardner) */
        complex double agc = lrpt_demodulator_agc_apply(demod->agc, fdata);
        complex double quadrature = lrpt_demodulator_pll_mix(demod->pll, agc);
        complex double current = demod->prev_I + cimag(quadrature) * I;

        demod->prev_I = creal(quadrature);
        demod->resync_offset -= sym_period;

        const double resync_error =
            (cimag(quadrature) - cimag(demod->before)) * cimag(demod->middle);

        demod->resync_offset += resync_error * sym_period / DEMOD_RESYNC_SCALE_OQPSK;
        demod->before = current;

        /* Carrier tracking */
        const double delta = lrpt_demodulator_pll_delta(demod->pll, demod->inphase, quadrature);

        lrpt_demodulator_pll_correct_phase(demod->pll, delta, demod->interp_factor);
        demod->resync_offset += 1.0;

        /* Save result in buffer */
        /* TODO may be ring buffer will be more appropriate so there will be no need in SOFT_FRAME_LEN */
        /* TODO may be it's better to just return soft-symbols directly; wrapper should be responsible to store them where necessary */
        buffer[(demod->buf_idx)++] = clamp_int8(creal(current) / 2.0);
        buffer[(demod->buf_idx)++] = clamp_int8(cimag(current) / 2.0);

        /* If we've reached frame length reset buffer index and signal caller with true */
        if (demod->buf_idx >= LRPT_SOFT_FRAME_LEN) {
            demod->buf_idx = 0;

            return true;
        }
        else
            return false;
    }

    demod->resync_offset += 1.0;

    return false;
}

/*************************************************************************************************/

/* lrpt_demodulator_init() */
lrpt_demodulator_t *lrpt_demodulator_init(
        lrpt_demodulator_mode_t mode,
        double costas_bandwidth,
        uint8_t interp_factor,
        double demod_samplerate,
        uint32_t symbol_rate,
        uint16_t rrc_order,
        double rrc_alpha,
        double pll_threshold) {
    /* Allocate our working demodulator */
    lrpt_demodulator_t *demod = malloc(sizeof(lrpt_demodulator_t));

    if (!demod)
        return NULL;

    /* NULL-init internal objects for safe deallocation */
    demod->agc = NULL;
    demod->pll = NULL;
    demod->rrc = NULL;
    demod->lut_isqrt = NULL;
    demod->out_buffer = NULL; /* TODO debug only */

    /* Initialize demodulator parameters */
    demod->sym_rate = symbol_rate;
    demod->sym_period = (double)interp_factor * demod_samplerate / (double)symbol_rate;
    demod->interp_factor = interp_factor;

    /* Initialize AGC object */
    demod->agc = lrpt_demodulator_agc_init(DEMOD_AGC_TARGET);

    if (!demod->agc) {
        lrpt_demodulator_deinit(demod);

        return NULL;
    }

    /* Initialize Costas' PLL object */
    const double pll_bw = LRPT_M_2PI * costas_bandwidth / (double)symbol_rate;
    demod->pll = lrpt_demodulator_pll_init(pll_bw, pll_threshold, mode);

    if (!demod->pll) {
        lrpt_demodulator_deinit(demod);

        return NULL;
    }

    /* Initialize RRC filter object */
    const double osf = demod_samplerate / (double)symbol_rate;
    demod->rrc = lrpt_demodulator_rrc_filter_init(rrc_order, interp_factor, osf, rrc_alpha);

    if (!demod->rrc) {
        lrpt_demodulator_deinit(demod);

        return NULL;
    }

    /* TODO debug only */
    demod->out_buffer = calloc(LRPT_SOFT_FRAME_LEN, sizeof(int8_t));
    if (!demod->out_buffer) {
        lrpt_demodulator_deinit(demod);

        return NULL;
    }

    /* Select demodulator function */
    switch (mode) {
        case LRPT_DEMODULATOR_MODE_QPSK:
            demod->lut_isqrt = NULL;
            demod->demod_func = demod_qpsk;

            break;

        case LRPT_DEMODULATOR_MODE_OQPSK:
            if (!lrpt_demodulator_lut_isqrt_init(demod->lut_isqrt)) {
                lrpt_demodulator_deinit(demod);

                return NULL;
            }

            demod->demod_func = demod_oqpsk;

            break;

        /* All other modes are unsupported */
        default:
            lrpt_demodulator_deinit(demod);

            return NULL;

            break;
    }

    /* Initialize internal working variables */
    demod->resync_offset = 0.0;
    demod->before = 0.0;
    demod->middle = 0.0;
    demod->inphase = 0.0;
    demod->prev_I = 0.0;
    demod->buf_idx = 0;
    demod->pr_I = 0;
    demod->pr_Q = 0;

    return demod;
}

/*************************************************************************************************/

/* lrpt_demodulator_deinit() */
void lrpt_demodulator_deinit(
        lrpt_demodulator_t *demod) {
    if (!demod)
        return;

    free(demod->out_buffer); /* TODO debug only */
    lrpt_demodulator_lut_isqrt_deinit(demod->lut_isqrt);
    lrpt_demodulator_rrc_filter_deinit(demod->rrc);
    lrpt_demodulator_pll_deinit(demod->pll);
    lrpt_demodulator_agc_deinit(demod->agc);
    free(demod);
}

/*************************************************************************************************/

/* lrpt_demodulator_gain() */
bool lrpt_demodulator_gain(
        const lrpt_demodulator_t *demod,
        double *gain) {
    if (!demod || !demod->agc)
        return false;

    *gain = demod->agc->gain;
    return true;
}

/*************************************************************************************************/

/* lrpt_demodulator_siglvl() */
bool lrpt_demodulator_siglvl(
        const lrpt_demodulator_t *demod,
        double *level) {
    if (!demod || !demod->agc)
        return false;

    *level = demod->agc->average;
    return true;
}

/*************************************************************************************************/

/* lrpt_demodulator_phaseerr() */
bool lrpt_demodulator_phaseerr(
        const lrpt_demodulator_t *demod,
        double *error) {
    if (!demod || !demod->pll)
        return false;

    *error = demod->pll->moving_average;
    return true;
}

/*************************************************************************************************/

/* lrpt_demodulator_exec() */
bool lrpt_demodulator_exec(
        lrpt_demodulator_t *demod,
        const lrpt_iq_data_t *input,
        lrpt_qpsk_data_t *output,
        FILE *fh /* TODO debug only */) {
    /* Return immediately if no valid input or output were given */
    /* TODO debug only */
    if (!input)
        return false;
//    if (!input || !output)
//        return false;

//    /* Resize output data structure if it is more than twice shorter than input I/Q data */
//    if (output->len < (2 * input->len))
//        if (!lrpt_qpsk_data_resize(output, 2 * input->len))
//            return false;

//    /* Allocate output buffer */
//    /* TODO may be move inside demodulator object */
//    int8_t *out_buffer = calloc(LRPT_SOFT_FRAME_LEN, sizeof(int8_t));
//
//    if (!out_buffer)
//        return false;

    /* Now we're ready to process filtered I/Q data and get soft-symbols */
    for (size_t i = 0; i < input->len; i++) {
        /* Make complex variable from filtered sample */
        complex double cdata = input->iq[i];

        for (uint8_t j = 0; j < demod->interp_factor; j++) {
            /* Pass samples through interpolator RRC filter */
            complex double fdata = lrpt_demodulator_rrc_filter_apply(demod->rrc, cdata);

            /* Demodulate using appropriate function */
            if (demod->demod_func(demod, fdata, demod->out_buffer)) {
                //lrpt_demodulator_dediffcode(demod, data);
                fwrite(demod->out_buffer, sizeof(int8_t), LRPT_SOFT_FRAME_LEN, fh); /* TODO debug only */
                /* TODO just store resulting symbol in output buffer */
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

/*************************************************************************************************/

/** \endcond */
