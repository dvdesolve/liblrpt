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
 * Public internal API for data manipulation routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_DATA_H
#define LRPT_DECODER_DATA_H

/*************************************************************************************************/

#include "../../include/lrpt.h"

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

/** Process LRPT frame data.
 *
 * \param decoder Pointer to the decoder object.
 * \param data Pointer to the raw data.
 *
 * \return \c true on successful frame processing and \c false otherwise.
 */
bool lrpt_decoder_data_process_frame(
        lrpt_decoder_t *decoder,
        uint8_t *data);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
