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
 * Author: Artem Litvinovich
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
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

#include "bitop.h"
#include "correlator.h"

#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Viterbi decoder object */
typedef struct lrpt_decoder_viterbi__ {
    lrpt_decoder_bitop_t bit_writer; /**< Bit writer object */

    /** @{ */
    /** Used by history buffer */
    uint8_t *history;
    uint8_t *fetched;

    uint16_t len;
    uint8_t hist_index;
    uint8_t renormalize_counter;
    /** @} */

    /** @{ */
    /** Used by error buffer */
    uint16_t *errors[2];

    uint16_t *read_errors;
    uint16_t *write_errors;
    uint8_t err_index;
    /** @} */

    uint8_t *encoded; /**< Needed for BER estimation */
    uint16_t ber; /**< Bit error rate */

    /** @{ */
    /** Distances stuff */
    uint16_t distances[4];
    uint16_t *dist_table;
    uint8_t *table;
    /** @} */

    /** @{ */
    /** Needed for pairs lookup */
    uint16_t *pair_outputs;
    uint8_t *pair_keys;
    uint32_t *pair_distances;
    uint8_t pair_outputs_len;
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

/** Perform Viterbi decoding.
 *
 * \param vit Pointer to the Viterbi decoder object.
 * \param corr Pointer to the correlator object.
 * \param input Input data array.
 * \param output Output data array.
 */
void lrpt_decoder_viterbi_decode(
        lrpt_decoder_viterbi_t *vit,
        const lrpt_decoder_correlator_t *corr,
        const int8_t *input,
        uint8_t *output);

/** Return BER as a percentage.
 *
 * \param vit Pointer to the Viterbi object.
 *
 * \return Bit error rate expressed as a percentage.
 */
uint8_t lrpt_decoder_viterbi_ber_percent(
        const lrpt_decoder_viterbi_t *vit);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
