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
 * Public internal API for decoder routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_DECODER_H
#define LRPT_DECODER_DECODER_H

/*************************************************************************************************/

#include "correlator.h"
#include "viterbi.h"

#include <stddef.h>

/*************************************************************************************************/

/** Decoder object */
struct lrpt_decoder__ {
    lrpt_decoder_correlator_t *corr; /**< Correlator */
    lrpt_decoder_viterbi_t *vit; /**< Viterbi decoder */
//
//    /** @{ */
//    /** Position information */
//    size_t pos, prev_pos;
//    /** @} */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
