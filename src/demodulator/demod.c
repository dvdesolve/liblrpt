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

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

/* lrpt_demodulator_exec()
 *
 * Demodulates given <input> I/Q samples and stores QPSK soft-symbol samples in <output>.
 * Requires parameters for RRC filter (order and alpha factor), PLL (bandwidth and threshold),
 * interpolation factor, symbol rate and QPSK mode.
 * <output> will be resized automatically to fit necessary chunk of QPSK soft symbols.
 */
bool lrpt_demodulator_exec(
        lrpt_iq_data_t *input,
        lrpt_qpsk_data_t *output,
        const uint32_t rrc_order,
        const double rrc_alpha,
        const uint32_t interp_factor,
        const double pll_bw,
        const double pll_thresh,
        const lrpt_demod_mode_t mode,
        const uint32_t symbol_rate) {
    /* Library defaults */
    static const double DSP_FILTER_RIPPLE = 5.0;
    static const uint8_t DSP_FILTER_NUMPOLES = 6;

    /* Return immediately if no valid input or output were given */
    if (!input || !output)
        return false;

    /* Resize output data structure if it is more than twice shorter than input I/Q data */
    if (output->len < (2 * input->len))
        if (!lrpt_qpsk_data_resize(output, 2 * input->len))
            return false;

    /* Prepare DSP filter */
    lrpt_dsp_filter_data_t *filter = lrpt_dsp_filter_init(
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

    /* Cleanup */
    lrpt_dsp_filter_deinit(filter);

    return true;
}
