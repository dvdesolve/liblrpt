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
 * Author: Davide Belloli
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
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

#include "demodulator.h"

#include "../../include/lrpt.h"
#include "../liblrpt/error.h"
#include "../liblrpt/lrpt.h"
#include "agc.h"
#include "pll.h"
#include "rrc.h"

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/** Convenient type for passing QPSK symbols */
typedef struct qpsk_sym__ {
    int8_t f, s;
} qpsk_sym_t;

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

/** Perform QPSK demodulation.
 *
 * \param demod Demodulator object.
 * \param fdata I/Q sample.
 * \param[out] sym Pointer to the output symbol.
 *
 * \return \c true on successfull demodulation and \c false otherwise.
 */
static bool demod_qpsk(
        lrpt_demodulator_t *demod,
        complex double fdata,
        qpsk_sym_t *sym);

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
        return x;
}

/*************************************************************************************************/

/* demod_qpsk() */
static bool demod_qpsk(
        lrpt_demodulator_t *demod,
        complex double fdata,
        qpsk_sym_t *sym) {
    /* Helper variables */
    const double sym_period = demod->sym_period;
    const double sp2 = sym_period / 2.0;
    const double sp2p1 = sp2 + 1.0;

    /* Symbol timing recovery (Gardner) */
    if ((demod->resync_offset >= sp2) && (demod->resync_offset < sp2p1)) {
        if (demod->offset) {
            const complex double agc = lrpt_demodulator_agc_apply(demod->agc, fdata);

            demod->inphase = lrpt_demodulator_pll_mix(demod->pll, agc);
            demod->middle = demod->prev_I + cimag(demod->inphase) * I;
            demod->prev_I = creal(demod->inphase);
        }
        else
            demod->middle = lrpt_demodulator_agc_apply(demod->agc, fdata);
    }
    else if (demod->resync_offset >= sym_period) {
        complex double current = 0, quadrature = 0; /* Needed to suppress dumb warning */

        if (demod->offset) {
            const complex double agc = lrpt_demodulator_agc_apply(demod->agc, fdata);

            /* Costas' loop frequency/phase tuning */
            quadrature = lrpt_demodulator_pll_mix(demod->pll, agc);

            current = demod->prev_I + cimag(quadrature) * I;
            demod->prev_I = creal(quadrature);
        }
        else
            current = lrpt_demodulator_agc_apply(demod->agc, fdata);

        demod->resync_offset -= sym_period;

        const double resync_error =
            (cimag((demod->offset) ? quadrature : current) -
             cimag(demod->before)) * cimag(demod->middle);

        demod->resync_offset += (resync_error * sym_period /
                ((demod->offset) ? DEMOD_RESYNC_SCALE_OQPSK : DEMOD_RESYNC_SCALE_QPSK));
        demod->before = current;

        if (!demod->offset) /* Costas' loop frequency/phase tuning */
            current = lrpt_demodulator_pll_mix(demod->pll, current);

        /* Carrier tracking */
        const double delta = lrpt_demodulator_pll_delta(demod->pll,
                (demod->offset) ? demod->inphase : current,
                (demod->offset) ? quadrature : current);

        lrpt_demodulator_pll_correct_phase(demod->pll, delta, demod->interp_factor);
        demod->resync_offset += 1.0;

        /* Save result */
        sym->f = clamp_int8(creal(current) / 2.0);
        sym->s = clamp_int8(cimag(current) / 2.0);

        return true;
    }

    demod->resync_offset += 1.0;

    return false;
}

/*************************************************************************************************/

/* lrpt_demodulator_init() */
lrpt_demodulator_t *lrpt_demodulator_init(
        bool offset,
        double costas_bandwidth,
        uint8_t interp_factor,
        uint32_t demod_samplerate,
        uint32_t symbol_rate,
        uint16_t rrc_order,
        double rrc_alpha,
        double pll_locked_threshold,
        double pll_unlocked_threshold,
        lrpt_error_t *err) {
    /* Allocate our working demodulator */
    lrpt_demodulator_t *demod = malloc(sizeof(lrpt_demodulator_t));

    if (!demod) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Demodulator object allocation failed");

        return NULL;
    }

    /* NULL-init internal objects for safe deallocation */
    demod->agc = NULL;
    demod->pll = NULL;
    demod->rrc = NULL;

    /* Set correct demodulation mode */
    demod->offset = offset;

    /* Initialize demodulator parameters */
    demod->sym_rate = symbol_rate;
    demod->sym_period = (double)demod_samplerate * interp_factor / symbol_rate;
    demod->interp_factor = interp_factor;

    /* Some initial values for internal objects */
    const double pll_bw = LRPT_M_2PI * costas_bandwidth / symbol_rate;
    const double osf = (double)demod_samplerate / symbol_rate;

    /* Initialize internal objects */
    demod->agc = lrpt_demodulator_agc_init(DEMOD_AGC_TARGET);
    demod->pll =
        lrpt_demodulator_pll_init(pll_bw, pll_locked_threshold, pll_unlocked_threshold, offset);
    demod->rrc = lrpt_demodulator_rrc_filter_init(rrc_order, interp_factor, osf, rrc_alpha);

    /* Check for allocation problems */
    if (!demod->agc || !demod->pll || !demod->rrc) {
        lrpt_demodulator_deinit(demod);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Internal objects allocation for demodulator object failed");

        return NULL;
    }

    /* Initialize internal working variables */
    demod->resync_offset = 0.0;
    demod->before = 0.0;
    demod->middle = 0.0;
    demod->inphase = 0.0;
    demod->prev_I = 0.0;

    return demod;
}

/*************************************************************************************************/

/* lrpt_demodulator_deinit() */
void lrpt_demodulator_deinit(
        lrpt_demodulator_t *demod) {
    if (!demod)
        return;

    lrpt_demodulator_rrc_filter_deinit(demod->rrc);
    lrpt_demodulator_pll_deinit(demod->pll);
    lrpt_demodulator_agc_deinit(demod->agc);
    free(demod);
}

/*************************************************************************************************/

/* TODO may be return decibels, percents or similar */
/* lrpt_demodulator_gain() */
double lrpt_demodulator_gain(
        const lrpt_demodulator_t *demod) {
    if (!demod || !demod->agc)
        return 0;

    return demod->agc->gain;
}

/*************************************************************************************************/

/* TODO may be return percentage as gain * level / target */
/* lrpt_demodulator_siglvl() */
double lrpt_demodulator_siglvl(
        const lrpt_demodulator_t *demod) {
    if (!demod || !demod->agc)
        return 0;

    return demod->agc->average;
}

/*************************************************************************************************/

/* lrpt_demodulator_phaseerr() */
double lrpt_demodulator_phaseerr(
        const lrpt_demodulator_t *demod) {
    if (!demod || !demod->pll)
        return 0;

    return demod->pll->moving_average;
}

/*************************************************************************************************/

/* lrpt_demodulator_exec() */
bool lrpt_demodulator_exec(
        lrpt_demodulator_t *demod,
        const lrpt_iq_data_t *input,
        lrpt_qpsk_data_t *output,
        lrpt_error_t *err) {
    /* Return immediately if no valid demodulator, input or output were given */
    if (!demod || !input || !output) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "Demodulator object is NULL or input and/or output data objects are NULL");

        return false;
    }

    /* Resize output data structure */
    if (output->len < (input->len * demod->interp_factor))
        if (!lrpt_qpsk_data_resize(output, input->len * demod->interp_factor, err))
            return false;

    /* Intermediate result storage */
    qpsk_sym_t sym;
    size_t out_len = 0;

    /* Now we're ready to process filtered I/Q data and get symbols */
    for (size_t i = 0; i < input->len; i++) {
        /* Make complex variable from filtered sample */
        complex double cdata = input->iq[i];

        for (uint8_t j = 0; j < demod->interp_factor; j++) {
            /* Pass samples through interpolator RRC filter */
            complex double fdata = lrpt_demodulator_rrc_filter_apply(demod->rrc, cdata);

            /* Demodulate using appropriate function */
            if (demod_qpsk(demod, fdata, &sym)) {
                output->qpsk[out_len] = sym.f;
                output->qpsk[out_len + 1] = sym.s;

                out_len += 2;
            }
        }
    }

    if (!lrpt_qpsk_data_resize(output, out_len / 2, err))
        return false;

    return true;
}

/*************************************************************************************************/

/** \endcond */
