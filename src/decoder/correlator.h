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
 * Public internal API for correlator routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_CORRELATOR_H
#define LRPT_DECODER_CORRELATOR_H

/*************************************************************************************************/

#include <stdint.h>

/*************************************************************************************************/

/** Correlator object */
typedef struct lrpt_decoder_correlator__ {
    /** @{ */
    /** Correlator internal state arrays */
    uint8_t *correlation;
    uint8_t *tmp_correlation;
    uint8_t *position;
    /** @} */

    /** Correlator patterns */
    uint8_t *patterns;

    /** @{ */
    /** Correlator tables */
    uint8_t *rotate_iq_tab;
    uint8_t *invert_iq_tab;
    uint8_t *corr_tab;
    /** @} */
} lrpt_decoder_correlator_t;

/*************************************************************************************************/

/** Allocate and initialize correlator object.
 *
 * \return Correlator object or \c NULL in case of error.
 */
lrpt_decoder_correlator_t *lrpt_decoder_correlator_init(void);

/** Free correlator object.
 *
 * \param corr Pointer to the correlator object.
 */
void lrpt_decoder_correlator_deinit(
        lrpt_decoder_correlator_t *corr);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
