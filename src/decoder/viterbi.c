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
 * Viterbi decoder routines.
 *
 * This source file contains routines for performing decoding with Viterbi algorithm.
 */

/*************************************************************************************************/

#include "viterbi.h"

#include "bitop.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

static const size_t VITERBI_STATES_NUM = 128; /**< Number of states of Viterbi decoder */
static const uint8_t VITERBI_POLYA = 79; /**< Viterbi's polynomial A, 01001111 */
static const uint8_t VITERBI_POLYB = 109; /**< Viterbi's polynomial B, 01101101 */

/*************************************************************************************************/

/** Find distance between hard and soft symbols.
 *
 * \param hard Hard symbol.
 * \param soft_y0 LSB of soft symbol.
 * \param soft_y1 MSB of soft symbol.
 *
 * \return Metric distance between hard and soft symbols.
 */
static uint16_t metric_soft_distance(
        uint8_t hard,
        uint8_t soft_y0,
        uint8_t soft_y1);

/*************************************************************************************************/

/* metric_soft_distance() */
static uint16_t metric_soft_distance(
        uint8_t hard,
        uint8_t soft_y0,
        uint8_t soft_y1) {
    const int16_t mag = 255;
    int16_t soft_x0, soft_x1;

    switch (hard & 0x03) {
        case 0:
            soft_x0 = mag;
            soft_x1 = mag;

            break;

        case 1:
            soft_x0 = -mag;
            soft_x1 =  mag;

            break;

        case 2:
            soft_x0 =  mag;
            soft_x1 = -mag;

            break;

        case 3:
            soft_x0 = -mag;
            soft_x1 = -mag;

            break;

        default:
            soft_x0 = 0;
            soft_x1 = 0;

            break;
    }

    /* Linear distance */
    return (uint16_t)(abs((int8_t)soft_y0 - soft_x0) + abs((int8_t)soft_y1 - soft_x1));

    /* Quadratic distance, results are not much better or worser. Needs math.h for sqrt and pow. */
    /* return (uint16_t)sqrt(pow((int8_t)soft_y0 - soft_x0, 2.0) + pow((int8_t)soft_y1 - soft_x1, 2.0)); */
}

/*************************************************************************************************/

/* lrpt_decoder_viterbi_init() */
lrpt_decoder_viterbi_t *lrpt_decoder_viterbi_init(void) {
    /* Allocate Viterbi decoder object */
    lrpt_decoder_viterbi_t *vit = malloc(sizeof(lrpt_decoder_viterbi_t));

    if (!vit)
        return NULL;

    vit->ber = 0;

    /* NULL-init internals for safe deallocation */
    vit->dist_table = NULL;
    vit->table = NULL;
    vit->pair_outputs = NULL;
    vit->pair_keys = NULL;

    /* Allocate internals */
    vit->dist_table = calloc(4 * 65536, sizeof(uint16_t));
    vit->table = calloc(VITERBI_STATES_NUM, sizeof(uint8_t));
    vit->pair_outputs = calloc(16, sizeof(uint32_t));
    vit->pair_keys = calloc(64, sizeof(uint32_t));

    /* Check for allocation problems */
    if (!vit->dist_table || !vit->table || !vit->pair_outputs || !vit->pair_keys) {
        lrpt_decoder_viterbi_deinit(vit);

        return NULL;
    }

    /* Init metric lookup table */
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 65536; j++)
            vit->dist_table[i * 4 + j] =
                metric_soft_distance((uint8_t)i, (uint8_t)(j & 0xFF), (uint8_t)(j >> 8));

    /* Polynomial table */
    for (size_t i = 0; i < 128; i++) {
        vit->table[i] = 0;

        if (lrpt_decoder_bitop_count(i & VITERBI_POLYA) % 2)
            vit->table[i] = vit->table[i] | 0x01;
        if (lrpt_decoder_bitop_count(i & VITERBI_POLYB) % 2)
            vit->table[i] = vit->table[i] | 0x02;
    }

    /* Initialize inverted outputs */
    uint32_t inv_outputs[16];

    for (size_t i = 0; i < 16; i++)
        inv_outputs[i] = 0;

    uint32_t oc = 1;

    for (size_t i = 0; i < 64; i++) {
        uint32_t o = (uint32_t)((vit->table[i * 2 + 1] << 2) | vit->table[i * 2]);

        if (inv_outputs[o] == 0) {
            inv_outputs[o] = oc;
            vit->pair_outputs[oc] = (uint32_t)o;
            oc++;
        }

        vit->pair_keys[i] = (uint32_t)(inv_outputs[o]);
    }

    vit->pair_outputs_len = oc;
    vit->pair_distances = calloc(oc, sizeof(uint32_t));

    if (!vit->pair_distances) {
        lrpt_decoder_viterbi_deinit(vit);

        return NULL;
    }

    return vit;
}

/*************************************************************************************************/

/* lrpt_decoder_viterbi_deinit() */
void lrpt_decoder_viterbi_deinit(lrpt_decoder_viterbi_t *vit) {
    if (!vit)
        return;

    free(vit->pair_distances);
    free(vit->pair_keys);
    free(vit->pair_outputs);
    free(vit->table);
    free(vit->dist_table);
    free(vit);
}

/*************************************************************************************************/

/** \endcond */
