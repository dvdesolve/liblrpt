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
 * Error reporting and handling routines.
 */

/*************************************************************************************************/

#include "error.h"

#include "../../include/lrpt.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* lrpt_error_init() */
inline lrpt_error_t *lrpt_error_init(void) {
    lrpt_error_t *err = malloc(sizeof(lrpt_error_t));

    if (!err)
        return NULL;

    err->level = LRPT_ERR_LVL_NONE;
    err->code = LRPT_ERR_CODE_NONE;
    err->msg = NULL;

    return err;
}

/*************************************************************************************************/

/* lrpt_error_deinit() */
inline void lrpt_error_deinit(
        lrpt_error_t *err) {
    if (!err)
        return;

    free(err->msg);
    free(err);
}

/*************************************************************************************************/

/* lrpt_error_set() */
void lrpt_error_set(
        lrpt_error_t *err,
        lrpt_error_level_t level,
        lrpt_error_code_t code,
        const char *msg) {
    if (msg) {
        const size_t len = strlen(msg);

        err->msg = calloc(len + 1, sizeof(char));
        strcpy(err->msg, msg);
    }
    else
        err->msg = NULL;

    err->level = level;
    err->code = code;
}

/*************************************************************************************************/

/* lrpt_error_reset() */
inline void lrpt_error_reset(
        lrpt_error_t *err) {
    if (!err)
        return;

    err->level = LRPT_ERR_LVL_NONE;
    err->code = LRPT_ERR_CODE_NONE;

    free(err->msg);
    err->msg = NULL;
}

/*************************************************************************************************/

/* lrpt_error_level() */
inline lrpt_error_level_t lrpt_error_level(
        const lrpt_error_t *err) {
    if (!err)
        return 0;

    return err->level;
}

/*************************************************************************************************/

/* lrpt_error_code() */
inline lrpt_error_code_t lrpt_error_code(
        const lrpt_error_t *err) {
    if (!err)
        return 0;

    return err->code;
}

/*************************************************************************************************/

/* lrpt_error_message() */
inline const char * lrpt_error_message(
        const lrpt_error_t *err) {
    if (!err || !err->msg)
        return NULL;

    return err->msg;
}

/*************************************************************************************************/

/** \endcond */
