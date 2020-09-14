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

/*************************************************************************************************/

#include "agc.h"

#include <stddef.h>
#include <stdlib.h>

/*************************************************************************************************/

/* lrpt_demodulator_agc_init()
 *
 * Allocates and initializes AGC object with target gain value of <target>.
 */
lrpt_demodulator_agc_t *lrpt_demodulator_agc_init(const double target) {
    /* Try to allocate our handle */
    lrpt_demodulator_agc_t *handle =
        (lrpt_demodulator_agc_t *)malloc(sizeof(lrpt_demodulator_agc_t));

    if (!handle)
        return NULL;

    /* Set default parameters */
    handle->target = target;
    handle->average = target;
    handle->gain = 1.0;
    handle->bias = 0.0;

    return handle;
}

/*************************************************************************************************/

/* lrpt_demodulator_agc_deinit()
 *
 * Frees previously allocated AGC object.
 */
void lrpt_demodulator_agc_deinit(lrpt_demodulator_agc_t *handle) {
    free(handle);
}
