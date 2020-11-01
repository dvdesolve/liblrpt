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
 * Public internal API for DSP routines.
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_DSP_H
#define LRPT_LIBLRPT_DSP_H

/*************************************************************************************************/

#include "../../include/lrpt.h"

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

/** Available DSP filter types */
typedef enum lrpt_dsp_filter_type__ {
    LRPT_DSP_FILTER_TYPE_LOWPASS,   /**< Lowpass filter */
    LRPT_DSP_FILTER_TYPE_HIGHPASS,  /**< Highpass filter */
    LRPT_DSP_FILTER_TYPE_BANDPASS   /**< Bandpass filter */
} lrpt_dsp_filter_type_t;

/** DSP filter object */
typedef struct lrpt_dsp_filter__ {
    /** Cutoff frequency as a fraction of sample rate */
    double cutoff;

    /** Passband ripple as a percentage */
    double ripple;

    /** Number of poles, must be even. Max value is limited to the 252. */
    uint8_t npoles;

    /** Filter type */
    lrpt_dsp_filter_type_t type;

    /** a and b coefficients of the filter */
    double *a, *b;

    /** Saved input and output values */
    double *x, *y;

    /** Ring buffer index */
    uint8_t ring_idx;

    /** I/Q samples data */
    lrpt_iq_data_t *iq_data;
} lrpt_dsp_filter_t;

/*************************************************************************************************/

/** Initializes recursive Chebyshev filter.
 *
 * \param iq_data I/Q data to be filtered.
 * \param bandwidth Bandwidth of the signal, Hz.
 * \param sample_rate Signal's sampling rate.
 * \param ripple Ripple level, %.
 * \param num_poles Number of filter poles.
 * \param type Filter type.
 *
 * \return Chebyshev filter object or NULL in case of error.
 */
lrpt_dsp_filter_t *lrpt_dsp_filter_init(
        lrpt_iq_data_t *iq_data,
        uint32_t bandwidth,
        double sample_rate,
        double ripple,
        uint8_t num_poles,
        lrpt_dsp_filter_type_t type);

/** Frees allocated Chebyshev filter.
 *
 * \param handle Filter object.
 */
void lrpt_dsp_filter_deinit(lrpt_dsp_filter_t *handle);

/** Applies recursive Chebyshev filter to the raw I/Q data.
 *
 * \param handle Filter object.
 *
 * \return false if \p handle is empty and true otherwise.
 */
bool lrpt_dsp_filter_apply(lrpt_dsp_filter_t *handle);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
