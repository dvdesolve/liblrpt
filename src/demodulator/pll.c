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

#include "pll.h"

#include "../../include/lrpt.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/*************************************************************************************************/

static void lrpt_demodulator_pll_recompute_coeffs(
        lrpt_demodulator_pll_t *handle,
        const double damping,
        const double bandwidth);

/*************************************************************************************************/

/* lrpt_demodulator_pll_recompute_coeffs()
 *
 * (Re)computes the alpha and beta coefficients of the Costas' PLL from damping and bandwidth
 * parameters and updates/sets them in the PLL object.
 */
static void lrpt_demodulator_pll_recompute_coeffs(
        lrpt_demodulator_pll_t *handle,
        const double damping,
        const double bandwidth) {
    const double bw2 = bandwidth * bandwidth;
    const double denom = (1.0 + 2.0 * damping * bandwidth + bw2);

    handle->alpha = (4.0 * damping * bandwidth) / denom;
    handle->beta = (4.0 * bw2) / denom;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_init()
 *
 * Allocates and initializes PLL object with specified <bandwidth> and demodulation mode <mode>.
 */
lrpt_demodulator_pll_t *lrpt_demodulator_pll_init(
        const double bandwidth,
        const lrpt_demodulator_mode_t mode) {
    /* LIBRARY DEFAULTS */
    const double PLL_INIT_FREQ = 0.001; /* Initial Costas' PLL frequency */
    const double PLL_DAMPING = 1.0 / M_SQRT2; /* Damping factor */
    const double PLL_ERR_SCALE_QPSK = 43.0; /* Scaling factors to control error magnitude */
    const double PLL_ERR_SCALE_DOQPSK = 80.0;
    const double PLL_ERR_SCALE_IDOQPSK = 80.0;

    /* Try to allocate our handle */
    lrpt_demodulator_pll_t *handle =
        (lrpt_demodulator_pll_t *)malloc(sizeof(lrpt_demodulator_pll_t));

    if (!handle)
        return NULL;

    /* Populate lookup table for tanh() */
    for (size_t i = 0; i < 256; i++)
        handle->lut_tanh[i] = tanh((double)(i - 128));

    /* Set default parameters */
    handle->nco_freq = PLL_INIT_FREQ;
    handle->nco_phase = 0.0;

    lrpt_demodulator_pll_recompute_coeffs(handle, PLL_DAMPING, bandwidth);
    handle->damping = PLL_DAMPING;
    handle->bw = bandwidth;

    /* TODO previously it allowed to reset of avg_winsize in Costas_Resync()
     * if receiving is stopped and restarted while PLL is locked */
    /*handle->locked = false;*/

    /* Needed to cut off stray locks at startup */
    handle->moving_average = 1.0e6;

    /* Error scaling depends on modulation mode */
    switch (mode) {
        case LRPT_DEMODULATOR_MODE_QPSK:
            handle->err_scale = PLL_ERR_SCALE_QPSK;

            break;

        case LRPT_DEMODULATOR_MODE_DOQPSK:
            handle->err_scale = PLL_ERR_SCALE_DOQPSK;

            break;

        case LRPT_DEMODULATOR_MODE_IDOQPSK:
            handle->err_scale = PLL_ERR_SCALE_IDOQPSK;

            break;

        /* All other modes are unsupported */
        default:
            lrpt_demodulator_pll_deinit(handle);

            return NULL;

            break;
    }

    return handle;
}

/*************************************************************************************************/

/* lrpt_demodulator_pll_deinit()
 *
 * Frees previously allocated PLL object.
 */
void lrpt_demodulator_pll_deinit(lrpt_demodulator_pll_t *handle) {
    free(handle);
}
