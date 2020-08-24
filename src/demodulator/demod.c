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

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

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
    return true;
}
