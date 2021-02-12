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
 * Public internal API for Viterbi decoder routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_VITERBI_H
#define LRPT_DECODER_VITERBI_H

/*************************************************************************************************/

#include <stdint.h>

/*************************************************************************************************/

/** Viterbi decoder object */
typedef struct lrpt_decoder_viterbi__ {
    uint16_t ber; /**< Bit error rate */

    /** @{ */
    /** Distances stuff */
    uint16_t *dist_table;
    uint8_t *table;
    /** @} */

    /** @{ */
    /** Needed for pairs lookup */
    uint32_t *pair_outputs; /* 2 ^ (2 * rate), rate = 2 */
    uint32_t *pair_keys; /* 2 ^ (order - 1), order = 7 */
    uint32_t *pair_distances;
    uint32_t pair_outputs_len;
    /** @} */
} lrpt_decoder_viterbi_t;

/*************************************************************************************************/

/** Allocate and initialize Viterbi decoder object.
 *
 * \return Pointer to the allocated Viterbi decoder object or \c NULL in case of error.
 */
lrpt_decoder_viterbi_t *lrpt_decoder_viterbi_init(void);

/** Free previously allocated Viterbi decoder.
 *
 * \param vit Pointer to the Viterbi decoder object.
 */
void lrpt_decoder_viterbi_deinit(lrpt_decoder_viterbi_t *vit);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
