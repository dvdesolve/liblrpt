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

#include "utils.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

bool lrpt_demodulator_lut_isqrt_init(uint8_t *lut) {
    lut = (uint8_t *)calloc(16385, sizeof(uint8_t));

    if (!lut)
        return false;

    for (uint16_t i = 0; i < 16385; i++)
        lut[i] = (uint8_t)sqrt((double)i);

    return true;
}

/*************************************************************************************************/

void lrpt_demodulator_lut_isqrt_deinit(uint8_t *lut) {
    free(lut);
}
