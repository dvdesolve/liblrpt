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

#ifndef LRPT_DEMODULATOR_RRC_H
#define LRPT_DEMODULATOR_RRC_H

/*************************************************************************************************/

#include <complex.h>
#include <stdint.h>

/*************************************************************************************************/

/* RRC filter object */
typedef struct lrpt_demodulator_rrc_filter__ {
    complex double *memory; /* Filter memory */
    int16_t idm; /* Index for memory ring buffer */
    double *coeffs; /* Filter coefficients */
    uint16_t count; /* Number of filter coefficients */
} lrpt_demodulator_rrc_filter_t;

/*************************************************************************************************/

lrpt_demodulator_rrc_filter_t *lrpt_demodulator_rrc_filter_init(
        const uint16_t order,
        const uint8_t factor,
        const double osf,
        const double alpha);
void lrpt_demodulator_rrc_filter_deinit(lrpt_demodulator_rrc_filter_t *handle);
complex double lrpt_demodulator_rrc_filter_apply(
        lrpt_demodulator_rrc_filter_t *handle,
        const complex double value);

/*************************************************************************************************/

#endif
