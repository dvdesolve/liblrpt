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
 * Correlator routines.
 *
 * This source file contains routines for performing decoding of LRPT signals.
 */

/*************************************************************************************************/

#include "correlator.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

static const size_t CORR_PATTERN_SIZE = 64; /**< Pattern size for correlator */
static const size_t CORR_PATTERN_COUNT = 8; /**< Number of patterns */
/** CCSDS synchronization word (0x1ACFFC1D) in Viterbi-encoded form */
static const uint64_t CORR_SYNC_WORD_ENC = 0xFCA2B63DB00D9794;
static const uint32_t CORR_LIMIT = 55; /**< Correlation limit */

/*************************************************************************************************/

/** Initialize correlator patterns.
 *
 * \param corr Pointer to the correlator object.
 * \param n Pattern number.
 * \param p Coded pattern phases.
 */
static void set_patterns(
        lrpt_decoder_correlator_t *corr,
        size_t n,
        uint64_t p);

/** Rotate symbol.
 *
 * \param corr Pointer to the correlator object.
 * \param data Symbol to rotate.
 * \param shift IQ plane quadrant.
 *
 * \return Rotated symbol.
 */
static uint8_t rotate_iq(
        const lrpt_decoder_correlator_t *corr,
        uint8_t data,
        size_t shift);

/** Rotate word.
 *
 * \param corr Pointer to the correlator object.
 * \param data Word to rotate.
 * \param shift IQ plane quadrant.
 *
 * \return Rotated word.
 */
static uint64_t rotate_iq_qw(
        const lrpt_decoder_correlator_t *corr,
        uint64_t data,
        size_t shift);

/** Invert word.
 *
 * \param corr Pointer to the correlator object.
 * \param data Word to invert.
 *
 * \return Inverted word.
 */
static uint64_t flip_iq_qw(
        const lrpt_decoder_correlator_t *corr,
        uint64_t data);

/*************************************************************************************************/

/* set_patterns() */
static void set_patterns(
        lrpt_decoder_correlator_t *corr,
        size_t n,
        uint64_t p) {
    for (size_t i = 0; i < CORR_PATTERN_SIZE; i++) {
        if ((p >> (CORR_PATTERN_SIZE - i - 1)) & 0x01)
            corr->patterns[i * CORR_PATTERN_SIZE + n] = 0xFF;
        else
            corr->patterns[i * CORR_PATTERN_SIZE + n] = 0;
    }
}

/*************************************************************************************************/

/* rotate_iq() */
static uint8_t rotate_iq(
        const lrpt_decoder_correlator_t *corr,
        uint8_t data,
        size_t shift) {
    if ((shift == 1) | (shift == 3))
        data = corr->rotate_iq_tab[data];

    if ((shift == 2) | (shift == 3))
        data = data ^ 0xFF;

    return data;
}

/*************************************************************************************************/

/* rotate_iq_qw() */
static uint64_t rotate_iq_qw(
        const lrpt_decoder_correlator_t *corr,
        uint64_t data,
        size_t shift) {
    uint64_t result = 0;

    for (size_t i = 0; i < CORR_PATTERN_COUNT; i++) {
        uint8_t bdata = (uint8_t)((data >> (56 - 8 * i)) & 0xFF);

        result <<= 8;
        result |= rotate_iq(corr, bdata, shift);
    }

    return result;
}

/*************************************************************************************************/

/* flip_iq_qw() */
static uint64_t flip_iq_qw(
        const lrpt_decoder_correlator_t *corr,
        uint64_t data) {
    uint64_t result = 0;

    for (size_t i = 0; i < CORR_PATTERN_COUNT; i++) {
        uint8_t bdata = (uint8_t)((data >> (56 - 8 * i)) & 0xFF);

        result <<= 8;
        result |= corr->invert_iq_tab[bdata];
    }

    return result;
}

/*************************************************************************************************/

/* lrpt_decoder_correlator_init() */
lrpt_decoder_correlator_t *lrpt_decoder_correlator_init(void) {
    /* Allocate correlator object */
    lrpt_decoder_correlator_t *corr = malloc(sizeof(lrpt_decoder_correlator_t));

    if (!corr)
        return NULL;

    /* NULL-init internals for safe deallocation */
    corr->correlation = NULL;
    corr->tmp_correlation = NULL;
    corr->position = NULL;
    corr->patterns = NULL;
    corr->rotate_iq_tab = NULL;
    corr->invert_iq_tab = NULL;
    corr->corr_tab = NULL;

    /* Allocate internals */
    corr->correlation = calloc(CORR_PATTERN_COUNT, sizeof(uint8_t));
    corr->tmp_correlation = calloc(CORR_PATTERN_COUNT, sizeof(uint8_t));
    corr->position = calloc(CORR_PATTERN_COUNT, sizeof(uint8_t));
    corr->patterns = calloc(CORR_PATTERN_SIZE * CORR_PATTERN_SIZE, sizeof(uint8_t));
    corr->rotate_iq_tab = calloc(256, sizeof(uint8_t));
    corr->invert_iq_tab = calloc(256, sizeof(uint8_t));
    corr->corr_tab = calloc(256 * 256, sizeof(uint8_t));

    /* Check for allocation problems */
    if (!corr->correlation || !corr->tmp_correlation || !corr->position || !corr->patterns ||
            !corr->rotate_iq_tab || !corr->invert_iq_tab || !corr->corr_tab) {
        lrpt_decoder_correlator_deinit(corr);

        return NULL;
    }

    /* Initialize correlator tables */
    for (size_t i = 0; i < 256; i++) {
        corr->rotate_iq_tab[i] = (uint8_t)((((i & 0x55) ^ 0x55) << 1) | ((i & 0xAA) >> 1));
        corr->invert_iq_tab[i] = (uint8_t)(( (i & 0x55)         << 1) | ((i & 0xAA) >> 1));

        for (size_t j = 0; j < 256; j++)
            corr->corr_tab[i * 256 + j] =
                (uint8_t)(((i > 127) && (j == 0)) || ((i <= 127) && (j == 255)));
    }

    for (size_t i = 0; i < 4; i++)
        set_patterns(corr, i, rotate_iq_qw(corr, CORR_SYNC_WORD_ENC, i));

    for (size_t i = 0; i < 4; i++)
        set_patterns(corr, i + 4, rotate_iq_qw(corr, flip_iq_qw(corr, CORR_SYNC_WORD_ENC), i));

    return corr;
}

/*************************************************************************************************/

/* lrpt_decoder_correlator_deinit() */
void lrpt_decoder_correlator_deinit(
        lrpt_decoder_correlator_t *corr) {
    if (!corr)
        return;

    free(corr->correlation);
    free(corr->tmp_correlation);
    free(corr->position);
    free(corr->patterns);
    free(corr->corr_tab);
    free(corr->invert_iq_tab);
    free(corr->rotate_iq_tab);
    free(corr->patterns);
    free(corr);
}

/*************************************************************************************************/

/* TODO recheck, especially for -1 result. May be use boolean flag */
/* lrpt_decoder_correlator_correlate() */
int lrpt_decoder_correlator_correlate(
        lrpt_decoder_correlator_t *corr,
        uint8_t *data,
        size_t len) {
    /* Reset correlator arrays */
    memset(corr->correlation, 0, CORR_PATTERN_COUNT);
    memset(corr->position, 0, CORR_PATTERN_COUNT);
    memset(corr->tmp_correlation, 0, CORR_PATTERN_COUNT);

    for (size_t i = 0; i < (len - CORR_PATTERN_SIZE); i++) {
        for (size_t j = 0; j < CORR_PATTERN_COUNT; j++)
            corr->tmp_correlation[j] = 0;

        for (size_t j = 0; j < CORR_PATTERN_SIZE; j++) {
            uint8_t *d = (corr->corr_tab + data[i + j] * 256);
            uint8_t *p = (corr->patterns + j * CORR_PATTERN_SIZE);

            /* TODO review */
            /* For some reason it's unrolled to CORR_PATTERN_COUNT times */
            corr->tmp_correlation[0] += d[p[0]];
            corr->tmp_correlation[1] += d[p[1]];
            corr->tmp_correlation[2] += d[p[2]];
            corr->tmp_correlation[3] += d[p[3]];
            corr->tmp_correlation[4] += d[p[4]];
            corr->tmp_correlation[5] += d[p[5]];
            corr->tmp_correlation[6] += d[p[6]];
            corr->tmp_correlation[7] += d[p[7]];
        }

        for (size_t j = 0; j < CORR_PATTERN_COUNT; j++)
            if (corr->tmp_correlation[j] > corr->correlation[j]) {
                corr->correlation[j] = corr->tmp_correlation[j];
                corr->position[j] = i;
                corr->tmp_correlation[j] = 0;

                if (corr->correlation[j] > CORR_LIMIT)
                    return j;
            }
    }

    uint8_t k = 0;
    int result = -1; /* TODO recheck, see above */

    for (size_t i = 0; i < CORR_PATTERN_COUNT; i++)
        if (corr->correlation[i] > k) {
            result = i;
            k = corr->correlation[i];
        }

    return result;
}

/*************************************************************************************************/

/** \endcond */
