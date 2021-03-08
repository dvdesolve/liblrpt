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
#include "correlator.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* Library defaults */
static const uint8_t VITERBI_STATES_NUM = 128; /**< Number of states of Viterbi decoder */
static const uint8_t VITERBI_PAIR_OUTPUTS_NUM = 16; /**< 2 ^ (2 * rate), rate = 2 */
static const uint8_t VITERBI_PAIR_KEYS_NUM = 64; /**< 2 ^ (order - 1), order = 7 */
static const uint8_t VITERBI_TRACEBACK_MIN = 25; /**< Minimal traceback (5 * 7) */
static const uint8_t VITERBI_TRACEBACK_LENGTH = 105; /**< Length of traceback (15 * 7) */
static const uint8_t VITERBI_POLYA = 0x4F; /**< Viterbi polynomial A, 01001111 */
static const uint8_t VITERBI_POLYB = 0x6D; /**< Viterbi polynomial B, 01101101 */
static const uint16_t VITERBI_FRAME_BITS = 8192; /**< Number of bits in frame */
static const uint8_t VITERBI_HIGH_BIT = 64;
static const uint8_t VITERBI_NUM_ITER = (VITERBI_HIGH_BIT * 2);
static const uint8_t VITERBI_RENORM_INTERVAL = 128; /* 65536 / (2 * 256) */

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

/** Perform error buffers swapping.
 *
 * \param vit Pointer to the Viterbi decoder object.
 */
static inline void swap_error_buffers(
        lrpt_decoder_viterbi_t *vit);

/** Fill up pair lookup distances.
 *
 * \param vit Pointer to the Viterbi decoder object.
 */
static void fill_pair_lookup_dists(
        lrpt_decoder_viterbi_t *vit);

/** Find best path in history buffer with set search interval.
 *
 * \param vit Pointer to the Viterbi decoder object.
 * \param search_every Search interval.
 *
 * \return The best found path.
 */
static uint8_t history_buffer_search(
        lrpt_decoder_viterbi_t *vit,
        uint8_t search_every);

/** Perform history buffer renormalization.
 *
 * \param vit Pointer to the Viterbi decoder object.
 * \param bestpath Index of best path.
 */
static void history_buffer_renormalize(
        lrpt_decoder_viterbi_t *vit,
        uint8_t bestpath);

/* Perform history buffer traceback.
 *
 * \param vit Pointer to the Viterbi decoder object.
 * \param bestpath Best found path.
 * \param min_traceback_length Minimum set traceback length.
 */
static void history_buffer_traceback(
        lrpt_decoder_viterbi_t *vit,
        uint8_t bestpath,
        uint8_t min_traceback_length);

/** Process history buffer with set skip interval.
 *
 * \param vit Pointer to the Viterbi decoder object.
 * \param skip How many entries to skip.
 */
static void history_buffer_process_skip(
        lrpt_decoder_viterbi_t *vit,
        uint8_t skip);

/** Viterbi inner code.
 *
 * \param vit Pointer to the Viterbi decoder object.
 * \param input Encoded soft-symbols data.
 */
static void viterbi_inner(
        lrpt_decoder_viterbi_t *vit,
        const uint8_t *input);

/** Viterbi tail code.
 *
 * \param vit Pointer to the Viterbi decoder object.
 * \param input Encoded soft-symbols data.
 */
static void viterbi_tail(
        lrpt_decoder_viterbi_t *vit,
        const uint8_t *input);

/** Perform convolutional Viterbi decoding.
 *
 * \param vit Pointer to the Viterbi decoder.
 * \param input Encoded soft-symbols data.
 * \param output Decoded data destination.
 */
static void convolutional_decode(
        lrpt_decoder_viterbi_t *vit,
        const uint8_t *input,
        uint8_t *output);

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

/* swap_error_buffers() */
static inline void swap_error_buffers(
        lrpt_decoder_viterbi_t *vit) {
    vit->read_errors = vit->errors[vit->err_index];
    vit->err_index = (vit->err_index + 1) % 2;
    vit->write_errors = vit->errors[vit->err_index];
}

/*************************************************************************************************/

/* fill_pair_lookup_dists() */
static void fill_pair_lookup_dists(
        lrpt_decoder_viterbi_t *vit) {
    for (size_t i = 1; i < vit->pair_outputs_len; i++) {
        const uint16_t c = vit->pair_outputs[i];
        const uint16_t i0 = (c & 0x03);
        const uint16_t i1 = (c >> 2);

        /* TODO review casts */
        vit->pair_distances[i] = (uint32_t)((vit->distances[i1] << 16) | vit->distances[i0]);
    }
}

/*************************************************************************************************/

/* history_buffer_search() */
static uint8_t history_buffer_search(
        lrpt_decoder_viterbi_t *vit,
        uint8_t search_every) {
    uint16_t least = 0xFFFF;
    uint8_t bestpath = 0;

    for (uint8_t i = 0; i < (VITERBI_STATES_NUM / 2); i += search_every)
        if (vit->write_errors[i] < least) {
            least = vit->write_errors[i];
            bestpath = i;
        }

    return bestpath;
}

/*************************************************************************************************/

/* history_buffer_renormalize() */
static void history_buffer_renormalize(
        lrpt_decoder_viterbi_t *vit,
        uint8_t bestpath) {
    const uint16_t min_distance = vit->write_errors[bestpath];

    for (size_t i = 0; i < (VITERBI_STATES_NUM / 2); i++)
        vit->write_errors[i] -= min_distance;
}

/*************************************************************************************************/

/* history_buffer_traceback() */
static void history_buffer_traceback(
        lrpt_decoder_viterbi_t *vit,
        uint8_t bestpath,
        uint8_t min_traceback_length) {
    uint8_t index = vit->hist_index;

    for (uint8_t i = 0; i < min_traceback_length; i++) {
        if (index == 0)
            index = VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH - 1;
        else
            index--;

        const uint8_t history =
            vit->history[index * (VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH) + bestpath];

        uint8_t pathbit;

        if (history != 0)
            pathbit = VITERBI_HIGH_BIT;
        else
            pathbit = 0;

        bestpath = (bestpath | pathbit) >> 1;
    }

    uint8_t prefetch_index = index;

    if (prefetch_index == 0)
        prefetch_index = VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH - 1;
    else
        prefetch_index--;

    uint8_t fetched_index = 0;

    for (size_t i = min_traceback_length; i < vit->len; i++) {
        index = prefetch_index;

        if (prefetch_index == 0)
            prefetch_index = VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH - 1;
        else
            prefetch_index--;

        const uint8_t history =
            vit->history[index * (VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH) + bestpath];

        uint8_t pathbit;

        if (history != 0)
            pathbit = VITERBI_HIGH_BIT;
        else
            pathbit = 0;

        bestpath = (bestpath | pathbit) >> 1;

        if (pathbit != 0)
            vit->fetched[fetched_index] = 1;
        else
            vit->fetched[fetched_index] = 0;

        fetched_index++;
    }

    lrpt_decoder_bitop_writer_reverse(vit->bit_writer, vit->fetched, fetched_index);
    vit->len -= fetched_index;
}

/*************************************************************************************************/

/* history_buffer_process_skip() */
static void history_buffer_process_skip(
        lrpt_decoder_viterbi_t *vit,
        uint8_t skip) {
    vit->hist_index++;

    if (vit->hist_index == (VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH))
        vit->hist_index = 0;

    vit->renormalize_counter++;
    vit->len++;

    uint8_t bestpath;

    if (vit->renormalize_counter == VITERBI_RENORM_INTERVAL) {
        vit->renormalize_counter = 0;
        bestpath = history_buffer_search(vit, skip);
        history_buffer_renormalize(vit, bestpath);

        if (vit->len == (VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH))
            history_buffer_traceback(vit, bestpath, VITERBI_TRACEBACK_MIN);
    }
    else if (vit->len == (VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH)) {
        bestpath = history_buffer_search(vit, skip);
        history_buffer_traceback(vit, bestpath, VITERBI_TRACEBACK_MIN);
    }
}

/*************************************************************************************************/

/* viterbi_inner() */
static void viterbi_inner(
        lrpt_decoder_viterbi_t *vit,
        const uint8_t *input) {
    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 0; j < (1 << (i + 1)); j++) {
            size_t idx = (input[i * 2 + 1] << 8) + input[i * 2]; /* TODO may be we can do explicit cast to uint8_t here and pass int8_t in all previous places */

            vit->write_errors[j] =
                vit->dist_table[vit->table[j] * 65536 + idx] + vit->read_errors[j >> 1];
        }

        swap_error_buffers(vit);
    }

    for (uint16_t i = 6; i < (VITERBI_FRAME_BITS - 6); i++) {
        for (uint8_t j = 0; j < 4; j++) {
            size_t idx = (input[i * 2 + 1] << 8) + input[i * 2]; /* TODO may be we can do explicit cast to uint8_t here and pass int8_t in all previous places */

            vit->distances[j] = vit->dist_table[j * 65536 + idx];
        }

        uint8_t *history =
            vit->history + vit->hist_index * (VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH);

        fill_pair_lookup_dists(vit);

        const uint8_t highbase = (VITERBI_HIGH_BIT >> 1);
        uint8_t low = 0;
        uint8_t high = VITERBI_HIGH_BIT;
        uint8_t base = 0;

        while (high < VITERBI_NUM_ITER) {
            uint8_t offset = 0;
            uint8_t base_offset = 0;

            while (base_offset < 4) {
                const uint8_t low_key = vit->pair_keys[base + base_offset];
                const uint8_t high_key = vit->pair_keys[highbase + base + base_offset];

                const uint32_t low_concat_dist = vit->pair_distances[low_key];
                const uint32_t high_concat_dist = vit->pair_distances[high_key];

                const uint16_t low_past_error = vit->read_errors[base + base_offset];
                const uint16_t high_past_error = vit->read_errors[highbase + base + base_offset];

                const uint16_t low_error = (low_concat_dist & 0xFFFF) + low_past_error;
                const uint16_t high_error = (high_concat_dist & 0xFFFF) + high_past_error;

                const uint8_t successor = low + offset;
                uint16_t error;
                uint8_t history_mask;

                if (low_error <= high_error) {
                    error = low_error;
                    history_mask = 0;
                }
                else {
                    error = high_error;
                    history_mask = 1;
                }

                vit->write_errors[successor] = error;
                history[successor] = history_mask;

                const uint8_t low_plus_one = low + offset + 1;

                const uint16_t low_plus_one_error = (low_concat_dist >> 16) + low_past_error;
                const uint16_t high_plus_one_error = (high_concat_dist >> 16) + high_past_error;

                const uint8_t plus_one_successor = low_plus_one;
                uint16_t plus_one_error;
                uint8_t plus_one_history_mask;

                if (low_plus_one_error <= high_plus_one_error) {
                    plus_one_error = low_plus_one_error;
                    plus_one_history_mask = 0;
                }
                else {
                    plus_one_error = high_plus_one_error;
                    plus_one_history_mask = 1;
                }

                vit->write_errors[plus_one_successor] = plus_one_error;
                history[plus_one_successor] = plus_one_history_mask;

                offset += 2;
                base_offset++;
            }

            low += 8;
            high += 8;
            base += 4;
        }

        history_buffer_process_skip(vit, 1);
        swap_error_buffers(vit);
    }
}

/*************************************************************************************************/

/* viterbi_tail() */
static void viterbi_tail(
        lrpt_decoder_viterbi_t *vit,
        const uint8_t *input) {
    for (uint16_t i = (VITERBI_FRAME_BITS - 6); i < VITERBI_FRAME_BITS; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            size_t idx = (input[i * 2 + 1] << 8) + input[i * 2]; /* TODO may be we can do explicit cast to uint8_t here and pass int8_t in all previous places */

            vit->distances[j] = vit->dist_table[j * 65536 + idx];
        }

        uint8_t *history =
            vit->history + vit->hist_index * (VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH);

        const uint8_t skip = 1 << (7 - (VITERBI_FRAME_BITS - i));
        const uint8_t base_skip = skip >> 1;

        const uint8_t highbase = (VITERBI_HIGH_BIT >> 1);
        uint8_t low = 0;
        uint8_t high = VITERBI_HIGH_BIT;
        uint8_t base = 0;

        while (high < VITERBI_NUM_ITER) {
            const uint8_t low_output = vit->table[low];
            const uint8_t high_output = vit->table[high];

            const uint16_t low_dist = vit->distances[low_output];
            const uint16_t high_dist = vit->distances[high_output];

            const uint16_t low_past_error = vit->read_errors[base];
            const uint16_t high_past_error = vit->read_errors[highbase + base];

            const uint16_t low_error = low_dist + low_past_error;
            const uint16_t high_error = high_dist + high_past_error;

            const uint8_t successor = low;
            uint16_t error;
            uint8_t history_mask;

            if (low_error < high_error) {
                error = low_error;
                history_mask = 0;
            }
            else {
                error = high_error;
                history_mask = 1;
            }

            vit->write_errors[successor] = error;
            history[successor] = history_mask;

            low += skip;
            high += skip;
            base += base_skip;
        }

        history_buffer_process_skip(vit, skip);
        swap_error_buffers(vit);
    }
}

/*************************************************************************************************/

/* convolutional_decode() */
static void convolutional_decode(
        lrpt_decoder_viterbi_t *vit,
        const uint8_t *input,
        uint8_t *output) {
    /* (Re)init bit writer */
    lrpt_decoder_bitop_writer_set(vit->bit_writer, output);

    /* (Re)set history buffer */
    vit->len = 0;
    vit->hist_index = 0;
    vit->renormalize_counter = 0;

    /* (Re)set error buffer */
    memset(vit->errors[0], 0, VITERBI_STATES_NUM);
    memset(vit->errors[1], 0, VITERBI_STATES_NUM);

    vit->read_errors = vit->errors[0];
    vit->write_errors = vit->errors[1];
    vit->err_index = 0;

    /* Do Viterbi decoding */
    viterbi_inner(vit, input);
    viterbi_tail(vit, input);
    history_buffer_traceback(vit, 0, 0);
}

/*************************************************************************************************/

/* convolutional_encode() */
static void convolutional_encode(
        lrpt_decoder_viterbi_t *vit,
        uint8_t *input,
        uint8_t *output) {
    lrpt_decoder_bitop_t b;

    /* TODO why not to use lrpt_decoder_bitop_writer_set? */
    b.p = input;
    b.pos = 0;

    /* TODO review type and casts */
    uint32_t sh = 0;

    for (uint16_t i = 0; i < VITERBI_FRAME_BITS; i++) {
        sh = ((sh << 1) | lrpt_decoder_bitop_fetch_n_bits(&b, 1)) & 0x7F;

        if ((vit->table[sh] & 0x01) != 0)
            output[i * 2 + 0] = 0;
        else
            output[i * 2 + 0] = 255;

        if ((vit->table[sh] & 0x02) != 0)
            output[i * 2 + 1] = 0;
        else
            output[i * 2 + 1] = 255;
    }
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
    vit->bit_writer = NULL;

    vit->dist_table = NULL;
    vit->table = NULL;

    vit->pair_outputs = NULL;
    vit->pair_keys = NULL;
    vit->pair_distances = NULL;

    vit->history = NULL;
    vit->fetched = NULL;

    vit->errors[0] = NULL;
    vit->errors[1] = NULL;

    vit->encoded = NULL;

    /* Allocate internals */
    vit->bit_writer = malloc(sizeof(lrpt_decoder_bitop_t));

    vit->dist_table = calloc(4 * 65536, sizeof(uint16_t)); /* TODO use static consts instead of 4 and 65536 */
    vit->table = calloc(VITERBI_STATES_NUM, sizeof(uint8_t));

    vit->pair_outputs = calloc(VITERBI_PAIR_OUTPUTS_NUM, sizeof(uint16_t));
    vit->pair_keys = calloc(VITERBI_PAIR_KEYS_NUM, sizeof(uint8_t));

    vit->history = calloc((VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH) * VITERBI_STATES_NUM,
            sizeof(uint8_t));
    vit->fetched = calloc(VITERBI_TRACEBACK_MIN + VITERBI_TRACEBACK_LENGTH, sizeof(uint8_t));

    vit->errors[0] = calloc(VITERBI_STATES_NUM, sizeof(uint16_t));
    vit->errors[1] = calloc(VITERBI_STATES_NUM, sizeof(uint16_t));

    vit->encoded = calloc(VITERBI_FRAME_BITS * 2, sizeof(uint8_t));

    /* Check for allocation problems */
    if (!vit->bit_writer || !vit->dist_table || !vit->table || !vit->pair_outputs ||
            !vit->history || !vit->fetched || !vit->pair_keys ||
            !vit->errors[0] || !vit->errors[1] || !vit->encoded) {
        lrpt_decoder_viterbi_deinit(vit);

        return NULL;
    }

    /* Init metric lookup table */
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 65536; j++)
            vit->dist_table[i * 4 + j] =
                metric_soft_distance((uint8_t)i, (uint8_t)(j & 0xFF), (uint8_t)(j >> 8));

    /* Polynomial table */
    for (size_t i = 0; i < VITERBI_STATES_NUM; i++) {
        vit->table[i] = 0;

        if (lrpt_decoder_bitop_count(i & VITERBI_POLYA) % 2)
            vit->table[i] = vit->table[i] | 0x01;
        if (lrpt_decoder_bitop_count(i & VITERBI_POLYB) % 2)
            vit->table[i] = vit->table[i] | 0x02;
    }

    /* Initialize inverted outputs */
    uint8_t inv_outputs[VITERBI_PAIR_OUTPUTS_NUM];
    memset(inv_outputs, 0, VITERBI_PAIR_OUTPUTS_NUM);

    uint8_t oc = 1;

    for (uint8_t i = 0; i < VITERBI_PAIR_KEYS_NUM; i++) { /* TODO review casts */
        uint16_t o = (uint16_t)((vit->table[i * 2 + 1] << 2) | vit->table[i * 2]);

        if (inv_outputs[o] == 0) {
            inv_outputs[o] = oc;
            vit->pair_outputs[oc] = o;
            oc++;
        }

        vit->pair_keys[i] = inv_outputs[o];
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
void lrpt_decoder_viterbi_deinit(
        lrpt_decoder_viterbi_t *vit) {
    if (!vit)
        return;

    free(vit->encoded);

    free(vit->errors[0]);
    free(vit->errors[1]);

    free(vit->fetched);
    free(vit->history);

    free(vit->pair_distances);
    free(vit->pair_keys);
    free(vit->pair_outputs);

    free(vit->table);
    free(vit->dist_table);

    free(vit->bit_writer);

    free(vit);
}

/*************************************************************************************************/

/* lrpt_decoder_viterbi_decode() */
void lrpt_decoder_viterbi_decode(
        lrpt_decoder_viterbi_t *vit,
        const lrpt_decoder_correlator_t *corr,
        const uint8_t *input,
        uint8_t *output) {
    /* Perform convolutional decoding */
    convolutional_decode(vit, input, output);

    /* Estimate signal quality. For that we should do convolutional encoding and calculate BER
     * by means of cross-correlation
     */
    convolutional_encode(vit, output, vit->encoded);
    vit->ber = 0;

    for (uint16_t i = 0; i < (VITERBI_FRAME_BITS * 2); i++)
        vit->ber += corr->corr_tab[input[i] * 256 + (vit->encoded[i] ^ 0xFF)];
}

/*************************************************************************************************/

/* lrpt_decoder_viterbi_ber_percent() */
uint8_t lrpt_decoder_viterbi_ber_percent(
        const lrpt_decoder_viterbi_t *vit) {
    return (100 * vit->ber / VITERBI_FRAME_BITS);
}

/*************************************************************************************************/

/** \endcond */
