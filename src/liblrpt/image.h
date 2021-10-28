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
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Public internal API for basic image manipulation.
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_IMAGE_H
#define LRPT_LIBLRPT_IMAGE_H

/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Storage type for LRPT images */
struct lrpt_image__ {
    size_t width; /**< Width of the image (in px) */
    size_t height; /**< Height of the image (in px) */

    uint8_t *channels[6]; /**< Per-channel image storage */
};

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
