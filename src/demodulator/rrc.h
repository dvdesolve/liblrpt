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
 * Public internal API for root raised cosine filtering routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DEMODULATOR_RRC_H
#define LRPT_DEMODULATOR_RRC_H

/*************************************************************************************************/

#include "../liblrpt/error.h"

#include <complex.h>
#include <stdint.h>

/*************************************************************************************************/

/** RRC filter object */
typedef struct lrpt_demodulator_rrc_filter__ {
    complex double *memory; /**< Filter memory */
    int16_t idm; /**< Index for memory ring buffer */

    double *coeffs; /**< Filter coefficients */
    uint16_t count; /**< Number of filter coefficients */
} lrpt_demodulator_rrc_filter_t;

/*************************************************************************************************/

/** Allocates and initializes RRC filter.
 *
 * \param order Filter order.
 * \param factor Interpolation factor.
 * \param osf Ratio of sampling rate and symbol rate.
 * \param alpha Filter alpha factor.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return RRC filter object.
 */
lrpt_demodulator_rrc_filter_t *lrpt_demodulator_rrc_filter_init(
        uint16_t order,
        uint8_t factor,
        double osf,
        double alpha,
        lrpt_error_t *err);

/** Frees previously allocated RRC filter object.
 *
 * \param rrc RRC filter object.
 */
void lrpt_demodulator_rrc_filter_deinit(
        lrpt_demodulator_rrc_filter_t *rrc);

/** Applies RRC filter to the I/Q sample.
 *
 * \param rrc RRC filter object.
 * \param value Input I/Q sample.
 *
 * \return Filtered I/Q sample.
 */
complex double lrpt_demodulator_rrc_filter_apply(
        lrpt_demodulator_rrc_filter_t *rrc,
        const complex double value);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
