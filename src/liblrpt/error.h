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
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Public internal API for error reporting and handling routines.
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_ERROR_H
#define LRPT_LIBLRPT_ERROR_H

/*************************************************************************************************/

#include "../../include/lrpt.h"

/*************************************************************************************************/

/** Error object */
struct lrpt_error__ {
    lrpt_error_level_t level; /**< Error level */
    lrpt_error_code_t code; /**< Numeric error code */
    char *msg; /**< Message */
};

/*************************************************************************************************/

/** Fill error object with data.
 *
 * \param err Pointer to the error object.
 * \param level Error level (see #lrpt_error_level_t for full list of levels).
 * \param code Numeric error code (see #lrpt_error_code_t for full list of codes).
 * \param msg Pointer to the character string (null-terminated) containing optional message.
 *
 * \warning If non-empty \c msg was passed user must free claimed resources later with
 * #lrpt_error_cleanup() explicitly!
 */
void lrpt_error_set(
        lrpt_error_t *err,
        lrpt_error_level_t level,
        lrpt_error_code_t code,
        const char *msg);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
