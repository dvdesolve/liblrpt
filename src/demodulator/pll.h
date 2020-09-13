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

#ifndef LRPT_DEMODULATOR_PLL_H
#define LRPT_DEMODULATOR_PLL_H

/*************************************************************************************************/

#include "../../include/lrpt.h"

#include <stdbool.h>

/*************************************************************************************************/

/* PLL object */
typedef struct lrpt_demodulator_pll__ {
    double nco_phase, nco_freq;
    double alpha, beta;
    double damping, bw;
    double moving_average;
    double err_scale;
    bool locked;
    double lut_tanh[256];
} lrpt_demodulator_pll_t;

/*************************************************************************************************/

lrpt_demodulator_pll_t *lrpt_demodulator_pll_init(
        const double bandwidth,
        const lrpt_demodulator_mode_t mode);
void lrpt_demodulator_pll_deinit(lrpt_demodulator_pll_t *handle);

/*************************************************************************************************/

#endif
