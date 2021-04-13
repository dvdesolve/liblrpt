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
 * Public internal API for packet manipulation routines.
 */

/*************************************************************************************************/

#ifndef LRPT_DECODER_PACKET_H
#define LRPT_DECODER_PACKET_H

/*************************************************************************************************/

#include "../../include/lrpt.h"

#include <stddef.h>

/*************************************************************************************************/

/** Parse one coded virtual channel data unit.
 *
 * \param decoder Pointer to the decoder object.
 */
void lrpt_decoder_packet_parse_cvcdu(
        lrpt_decoder_t *decoder);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
