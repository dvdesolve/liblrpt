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

/** \file
 *  \author Viktor Drobot
 *  \author Neoklis Kyriazis
 *
 * Public liblrpt API
 *
 * This is basic header to access public API which liblrpt provides.
 */

/*************************************************************************************************/

#ifndef LRPT_LRPT_H
#define LRPT_LRPT_H

/*************************************************************************************************/

/* Generic helper definitions for shared library support.
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

/* Now we use the generic helper definitions above to define LRPT_API and LRPT_LOCAL.
 * LRPT_API is used for the public API symbols. It either DLL imports or DLL exports
 * (or does nothing for static build) LRPT_LOCAL is used for non-api symbols.
 */
#define LRPT_DLL /* Always build DLL */

#ifdef LRPT_DLL /* Defined if LRPT is compiled as a DLL */
  #ifdef LRPT_DLL_EXPORTS /* Defined if we are building the LRPT DLL (instead of using it) */
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** Basic data type for storing I/Q samples */
typedef struct lrpt_iq_data__ lrpt_iq_data_t;

/** Basic data type for storing QPSK soft symbols */
typedef struct lrpt_qpsk_data__ lrpt_qpsk_data_t;

/** Available PSK demodulator modes */
typedef enum lrpt_demod_mode__ {
    LRPT_DEMOD_MODE_QPSK,   /**< Plain QPSK mode */
    LRPT_DEMOD_MODE_DOQPSK, /**< Differential offset QPSK mode */
    LRPT_DEMOD_MODE_IDOQPSK /**< Interleaved differential offset QPSK mode */
} lrpt_demod_mode_t;

/*************************************************************************************************/

/** Allocates raw I/Q data storage object.
 *
 * Tries to allocate storage for raw I/Q data of requested \p length.
 *
 * \param[in] length Desired length for new I/Q data storage. If zero length is requested,
 * empty storage will be allocated but user can resize it with #lrpt_iq_data_resize().
 *
 * \return Returns a pointer to newly allocated storage object and NULL if allocation has failed.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_alloc(const size_t length);

/** Frees previously allocated I/Q data storage
 *
 * \param[in,out] handle Pointer to the I/Q data storage object.
 */
LRPT_API void lrpt_iq_data_free(lrpt_iq_data_t *handle);

/** Resizes existing I/Q data storage.
 *
 * If valid \p handle is provided it will be resized to accomodate \p new_length I/Q pairs.
 *
 * \param[in,out] handle Pointer to the I/Q data storage object.
 * \param[in] new_length New length \p handle will be resized to.
 *
 * \return Returns true on successfull resize and false otherwise (original storage will not be
 * touched in that case).
 */
LRPT_API bool lrpt_iq_data_resize(lrpt_iq_data_t *handle, const size_t new_length);

/* TODO describe internal format */
/** Loads I/Q data from file.
 *
 * Reads raw I/Q data from file with name \p fname and saves it into I/Q storage referebced by
 * \p handle. Storage will be auto-resized to proper length.
 *
 * \param[in,out] handle Pointer to the I/Q data storage object.
 * \param[in] fname Name of file with raw I/Q data.
 *
 * \return Returns true on successfull reading and false otherwise.
 *
 * \note File with I/Q data is expected to be compatible with internal library format, e. g.
 * created with #lrpt_iq_data_save_to_file()!
 */
LRPT_API bool lrpt_iq_data_load_from_file(lrpt_iq_data_t *handle, const char *fname);

/** Saves I/Q data to file.
 *
 * Saves raw I/Q data pointed by \p handle to file with name \p fname. If file already exists it
 * will be overwritten.
 *
 * \param[in] handle Pointer to the I/Q data storage object.
 * \param[in] fname Name of file to save raw I/Q data to.
 *
 * \return Returns true on successfull reading and false otherwise.
 *
 * \note Resulting file maintains internal library format and can be read with
 * #lrpt_iq_data_load_from_file().
 */
LRPT_API bool lrpt_iq_data_save_to_file(lrpt_iq_data_t *handle, const char *fname);

/** Allocates QPSK soft symbol data storage object.
 *
 * Tries to allocate storage for QPSK soft symbol data of requested \p length.
 *
 * \param[in] length Desired length for new QPSK soft symbol data storage. If zero length
 * is requested, empty storage will be allocated but user can resize it with
 * #lrpt_qpsk_data_resize().
 *
 * \return Returns a pointer to newly allocated storage object and NULL if allocation has failed.
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(const size_t length);

/** Frees previously allocated QPSK soft symbol data storage.
 *
 * \param[in,out] handle Pointer to the QPSK soft symbol data storage object.
 */
LRPT_API void lrpt_qpsk_data_free(lrpt_qpsk_data_t *handle);

/** Resizes existing QPSK soft symbol data storage.
 *
 * If valid \p handle is provided it will be resized to accomodate \p new_length QPSK soft symbols.
 *
 * \param[in,out] handle Pointer to the QPSK soft symbol data storage object.
 * \param[in] new_length New length \p handle will be resized to.
 *
 * \return Returns true on successfull resize and false otherwise (original storage will not be
 * touched in that case).
 */
LRPT_API bool lrpt_qpsk_data_resize(lrpt_qpsk_data_t *handle, const size_t new_length);

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
