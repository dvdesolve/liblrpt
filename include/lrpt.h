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

/** \addtogroup liblrpt liblrpt
 *
 * Public liblrpt API.
 * This API provides an interface for demodulating, decoding and post-processing LRPT signals.
 *
 * \author Viktor Drobot
 * \author Neoklis Kyriazis
 *
 * @{
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

/** Basic data type for single I/Q sample */
typedef struct lrpt_iq_raw__ lrpt_iq_raw_t;

/** Basic data type for storing I/Q samples */
typedef struct lrpt_iq_data__ lrpt_iq_data_t;

/** Basic data type for storing QPSK soft symbols */
typedef struct lrpt_qpsk_data__ lrpt_qpsk_data_t;

/** Demodulator object type */
typedef struct lrpt_demodulator__ lrpt_demodulator_t;

/** Available PSK demodulator modes */
typedef enum lrpt_demodulator_mode__ {
    LRPT_DEMODULATOR_MODE_QPSK,   /**< Plain QPSK */
    LRPT_DEMODULATOR_MODE_DOQPSK, /**< Differential offset QPSK */
    LRPT_DEMODULATOR_MODE_IDOQPSK /**< Interleaved differential offset QPSK */
} lrpt_demodulator_mode_t;

/*************************************************************************************************/

/** Allocates raw I/Q data storage object.
 *
 * Tries to allocate storage for raw I/Q data of requested \p length. User should properly free
 * obtained object with #lrpt_iq_data_free() after use.
 *
 * \param[in] length Desired length for new I/Q data storage. If zero length is requested,
 * empty storage will be allocated but user can resize it later with #lrpt_iq_data_resize().
 *
 * \return Pointer to allocated storage object or NULL if allocation has failed.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_alloc(const size_t length);

/** Frees previously allocated I/Q data storage.
 *
 * \param[in] handle Pointer to the I/Q data storage object.
 */
LRPT_API void lrpt_iq_data_free(lrpt_iq_data_t *handle);

/** Resizes existing I/Q data storage.
 *
 * If valid \p handle is provided it will be resized to accomodate \p new_length I/Q pairs.
 *
 * \param[in] handle Pointer to the I/Q data storage object.
 * \param[in] new_length New length \p handle will be resized to.
 *
 * \return true on successfull resize and false otherwise (original storage will not be
 * touched in that case).
 */
LRPT_API bool lrpt_iq_data_resize(lrpt_iq_data_t *handle, const size_t new_length);

/* TODO describe internal format */
/** Loads I/Q data from file.
 *
 * Reads raw I/Q data from file with name \p fname and saves it into I/Q storage referenced by
 * \p handle. Storage will be auto-resized to proper length.
 *
 * \param[in] handle Pointer to the I/Q data storage object.
 * \param[in] fname Name of file with raw I/Q data.
 *
 * \return true on successfull reading and false otherwise.
 *
 * \note File with I/Q data is expected to be compatible with internal library format, e. g.
 * created with #lrpt_iq_data_save_to_file().
 *
 * \warning Current implementation reads the whole file into memory at once. This can lead to
 * potential problems with large files. Please consider this function as convenient routine for
 * testing purposes with reasonably small files!
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
 * \return true on successfull reading and false otherwise.
 *
 * \note Resulting file maintains internal library format and can be read back again with
 * #lrpt_iq_data_load_from_file().
 */
LRPT_API bool lrpt_iq_data_save_to_file(lrpt_iq_data_t *handle, const char *fname);

/** Transforms separate arrays of double-typed I/Q samples into library format.
 *
 * Repacks arrays of I and Q samples (\p i and \p q) of size \p length into library
 * format of I/Q data storage. Storage given with \p handle will be auto-resized to fit
 * requested data length.
 *
 * \param[in] handle Pointer to the I/Q data storage object.
 * \param[in] i Pointer to the array of I samples.
 * \param[in] q Pointer to the array of Q samples.
 * \param[in] length Number of samples in I and Q arrays to repack into I/Q data.
 *
 * \return true on successfull repacking and false otherwise.
 *
 * \warning It's the user's responsibility to be sure that \p i and \p q arrays are
 * big enough and contain at least \p length samples! If user has provided \p length bigger
 * than original samples storage behavior is unpredictable!
 */
LRPT_API bool lrpt_iq_data_load_from_doubles(
        lrpt_iq_data_t *handle,
        const double *i,
        const double *q,
        const size_t length);

/** Creates I/Q storage object from double-typed I/Q samples.
 *
 * This function behaves much like #lrpt_iq_data_load_from_doubles(), however, it allocates
 * I/Q storage automatically.
 *
 * \param[in] i Pointer to the array of I samples.
 * \param[in] q Pointer to the array of Q samples.
 * \param[in] length Number of samples in I and Q arrays to repack into I/Q data.
 *
 * \return Pointer to the newly allocated I/Q data storage object or NULL if allocation was
 * unsuccessful.
 *
 * \warning Because code logic is the same as with #lrpt_iq_data_load_from_doubles(), the same
 * caution about \p i and \p q array lengths is actual.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_doubles(
        const double *i,
        const double *q,
        const size_t length);

/** Transforms array of raw I/Q samples into library format.
 *
 * Merges array of raw I/Q samples \p iq of size \p length into library format of I/Q data storage.
 * Storage given with \p handle will be auto-resized to fit requested data length.
 *
 * \param[in] handle Pointer to the I/Q data storage object.
 * \param[in] iq Pointer to the array of I/Q samples.
 * \param[in] length Number of samples to merge into I/Q data.
 *
 * \return true on successfull merging and false otherwise.
 *
 * \warning It's the user's responsibility to be sure that \p iq array is big enough and contain
 * at least \p length samples! If user has provided \p length bigger than original samples storage
 * behavior is unpredictable!
 */
LRPT_API bool lrpt_iq_data_load_from_samples(
        lrpt_iq_data_t *handle,
        const lrpt_iq_raw_t *iq,
        const size_t length);

/** Creates I/Q storage object from raw I/Q samples.
 *
 * This function behaves much like #lrpt_iq_data_load_from_samples(), however, it allocates
 * I/Q storage automatically.
 *
 * \param[in] iq Pointer to the array of raw I/Q samples.
 * \param[in] length Number of samples in I and Q arrays to repack into I/Q data.
 *
 * \return Pointer to the newly allocated I/Q data storage object or NULL if allocation was
 * unsuccessful.
 *
 * \warning Because code logic is the same as with #lrpt_iq_data_load_from_samples(), the same
 * caution about \p iq array length is actual.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_samples(
        const lrpt_iq_raw_t *iq,
        const size_t length);

/** Allocates QPSK soft symbol data storage object.
 *
 * Tries to allocate storage for QPSK soft symbol data of requested \p length. User should properly
 * free obtained object with #lrpt_qpsk_data_free() after use.
 *
 * \param[in] length Desired length for new QPSK soft symbol data storage. If zero length
 * is requested, empty storage will be allocated but user can resize it manually later with
 * #lrpt_qpsk_data_resize().
 *
 * \return Pointer to newly allocated storage object or NULL if allocation has failed.
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(const size_t length);

/** Frees previously allocated QPSK soft symbol data storage.
 *
 * \param[in] handle Pointer to the QPSK soft symbol data storage object.
 */
LRPT_API void lrpt_qpsk_data_free(lrpt_qpsk_data_t *handle);

/** Resizes existing QPSK soft symbol data storage.
 *
 * If valid \p handle is provided it will be resized to accomodate \p new_length QPSK soft symbols.
 *
 * \param[in] handle Pointer to the QPSK soft symbol data storage object.
 * \param[in] new_length New length \p handle will be resized to.
 *
 * \return true on successfull resize and false otherwise (original storage will not be
 * touched in that case).
 */
LRPT_API bool lrpt_qpsk_data_resize(lrpt_qpsk_data_t *handle, const size_t new_length);

/** Allocates and initializes demodulator object.
 *
 * Creates demodulator object with specified parameters. User should properly free obtained object
 * with #lrpt_demodulator_deinit() after use.
 *
 * \param[in] mode PSK modulation mode.
 * \param[in] costas_bandwidth Initial Costas' PLL bandwidth in Hz.
 * \param[in] interp_factor Interpolation factor. Usual value is 4.
 * \param[in] demod_samplerate Demodulation sampling rate in samples/s.
 * \param[in] symbol_rate PSK symbol rate in Sym/s.
 * \param[in] rrc_order Costas' PLL root raised cosine filter order.
 * \param[in] rrc_alpha Costas' PLL root raised cosine filter alpha factor.
 * \param[in] pll_threshold Costas' PLL locked threshold. Unlocked threshold will be set 3% above
 * it.
 *
 * \return Demodulator object or NULL in case of error.
 */
LRPT_API lrpt_demodulator_t *lrpt_demodulator_init(
        const lrpt_demodulator_mode_t mode,
        const double costas_bandwidth,
        const uint8_t interp_factor,
        const double demod_samplerate,
        const uint32_t symbol_rate,
        const uint16_t rrc_order,
        const double rrc_alpha,
        const double pll_threshold);

/** Frees previously allocated demodulator object.
 *
 * \param[in] handle Pointer to the demodulator object.
 */
LRPT_API void lrpt_demodulator_deinit(lrpt_demodulator_t *handle);

/** Performs QPSK demodulation.
 *
 * Runs demodulation on given \p input I/Q samples. Input samples are filtered with Chebyshev
 * recursive filter and then demodulated with \p handle demodulator object. Resulting QPSK soft
 * symbols will be stored in \p output QPSK data storage.
 *
 * \param[in] handle Demodulator object.
 * \param[in,out] input Raw I/Q samples to demodulate.
 * \param[out] output Demodulated QPSK soft symbols.
 *
 * \return true on successfull demodulation and false in case of error.
 *
 * \warning Original I/Q samples given with \p input will be modified (filtered with Chebyshev
 * recursive filter)!
 */
LRPT_API bool lrpt_demodulator_exec(
        lrpt_demodulator_t *handle,
        lrpt_iq_data_t *input,
        lrpt_qpsk_data_t *output);

/*************************************************************************************************/

/**
 * @}
 */

#endif
