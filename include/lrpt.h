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

#ifndef LRPT_LRPT_H
#define LRPT_LRPT_H

/*************************************************************************************************/

/*
 * Generic helper definitions for shared library support.
 * Taken from https://gcc.gnu.org/wiki/Visibility
 */
#if defined _WIN32 || defined __CYGWIN__
  #define LRPT_HELPER_DLL_IMPORT __declspec(dllimport)
  #define LRPT_HELPER_DLL_EXPORT __declspec(dllexport)
  #define LRPT_HELPER_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define LRPT_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
    #define LRPT_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define LRPT_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define LRPT_HELPER_DLL_IMPORT
    #define LRPT_HELPER_DLL_EXPORT
    #define LRPT_HELPER_DLL_LOCAL
  #endif
#endif

/*
 * Now we use the generic helper definitions above to define LRPT_API and LRPT_LOCAL.
 * LRPT_API is used for the public API symbols. It either DLL imports or DLL exports
 * (or does nothing for static build) LRPT_LOCAL is used for non-api symbols.
 */
#define LRPT_DLL /* always build DLL */

#ifdef LRPT_DLL /* defined if LRPT is compiled as a DLL */
  #ifdef LRPT_DLL_EXPORTS /* defined if we are building the LRPT DLL (instead of using it) */
    #define LRPT_API LRPT_HELPER_DLL_EXPORT
  #else
    #define LRPT_API LRPT_HELPER_DLL_IMPORT
  #endif
  #define LRPT_LOCAL LRPT_HELPER_DLL_LOCAL
#else /* LRPT_DLL is not defined: this means LRPT is a static lib. */
  #define LRPT_API
  #define LRPT_LOCAL
#endif

/*************************************************************************************************/

#define LRPT_M_2PI 6.28318530717958647692

/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

typedef struct lrpt_iq_data__ lrpt_iq_data_t;
typedef struct lrpt_qpsk_data__ lrpt_qpsk_data_t;
typedef enum lrpt_demod_mode__ {
    LRPT_DEMOD_MODE_QPSK,
    LRPT_DEMOD_MODE_DOQPSK,
    LRPT_DEMOD_MODE_IDOQPSK
} lrpt_demod_mode_t;

/*************************************************************************************************/

LRPT_API lrpt_iq_data_t *lrpt_iq_data_alloc(const size_t length);
LRPT_API void lrpt_iq_data_free(lrpt_iq_data_t *handle);
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(const size_t length);
LRPT_API bool lrpt_qpsk_data_resize(lrpt_qpsk_data_t *handle, const size_t new_length);
LRPT_API void lrpt_qpsk_data_free(lrpt_qpsk_data_t *handle);

/* TODO check integer types */
LRPT_API bool lrpt_demodulator_exec(
        lrpt_iq_data_t *input,
        lrpt_qpsk_data_t *output,
        const uint32_t rrc_order,
        const double rrc_alpha,
        const uint32_t interp_factor,
        const double pll_bw,
        const double pll_thresh,
        const lrpt_demod_mode_t mode,
        const uint32_t symbol_rate);

/*************************************************************************************************/

#endif
