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

/* Support for C++ codes */
#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************************************/

/** \addtogroup common Common
 *
 * Common utility routines.
 *
 * Interfaces for basic utility routines such as data objects allocation, freeing, conversion
 * and manipulation.
 *
 * @{
 */

/** I/Q samples data storage type */
typedef struct lrpt_iq_data__ lrpt_iq_data_t;

/** I/Q samples ring buffer storage type */
typedef struct lrpt_iq_rb__ lrpt_iq_rb_t;

/** QPSK symbols data storage type */
typedef struct lrpt_qpsk_data__ lrpt_qpsk_data_t;

/** QPSK symbols ring buffer storage type */
typedef struct lrpt_qpsk_rb__ lrpt_qpsk_rb_t;

/** LRPT image storage type */
typedef struct lrpt_image__ lrpt_image_t;

/** Error levels */
typedef enum lrpt_error_level__ {
    LRPT_ERR_LVL_NONE = 0,
    LRPT_ERR_LVL_INFO,
    LRPT_ERR_LVL_WARN,
    LRPT_ERR_LVL_ERROR
} lrpt_error_level_t;

/** Error codes */
typedef enum lrpt_error_code__ {
    LRPT_ERR_CODE_NONE = 0,
    LRPT_ERR_CODE_ALLOC,
    LRPT_ERR_CODE_INVOBJ,
    LRPT_ERR_CODE_PARAM,
    LRPT_ERR_CODE_NODATA,
    LRPT_ERR_CODE_FOPEN,
    LRPT_ERR_CODE_FREAD,
    LRPT_ERR_CODE_FWRITE,
    LRPT_ERR_CODE_FSEEK,
    LRPT_ERR_CODE_EOF,
    LRPT_ERR_CODE_FILECORR,
    LRPT_ERR_CODE_UNSUPP,
    LRPT_ERR_CODE_DATAPROC
} lrpt_error_code_t;

/** Error object type */
typedef struct lrpt_error__ lrpt_error_t;

/** @} */

/** \addtogroup io I/O
 *
 * Input/output routines.
 *
 * Interfaces for basic utility routines for I/O operations with common data types.
 *
 * @{
 */

/** I/Q samples data file type */
typedef struct lrpt_iq_file__ lrpt_iq_file_t;

/** Supported I/Q samples data file format versions */
typedef enum lrpt_iq_file_version__ {
    LRPT_IQ_FILE_VER1 = 0x01 /**< Version 1 */
} lrpt_iq_file_version_t;

/** Supported flags for Version 1 I/Q file */
typedef enum lrpt_iq_file_flags_ver1__ {
    LRPT_IQ_FILE_FLAGS_VER1_OFFSET = 0x01 /**< Offset modulation */
} lrpt_iq_file_flags_ver1_t;

/** QPSK symbols data file type */
typedef struct lrpt_qpsk_file__ lrpt_qpsk_file_t;

/** Supported QPSK symbols data file format versions */
typedef enum lrpt_qpsk_file_version__ {
    LRPT_QPSK_FILE_VER1 = 0x01 /**< Version 1 */
} lrpt_qpsk_file_version_t;

/** Supported flags for Version 1 QPSK file */
typedef enum lrpt_qpsk_file_flags_ver1__ {
    LRPT_QPSK_FILE_FLAGS_VER1_DIFFCODED = 0x01, /**< Diffcoded QPSK data */
    LRPT_QPSK_FILE_FLAGS_VER1_INTERLEAVED = 0x02, /**< Interleaved QPSK data */
    LRPT_QPSK_FILE_FLAGS_VER1_HARDSYMBOLED = 0x04 /**< Hard-symboled file */
} lrpt_qpsk_file_flags_ver1_t;

/** @} */

/** \addtogroup dsp DSP
 *
 * Digital signal processing routines.
 *
 * Interfaces for DSP operations such as filtering, FFT, dediffcoding and deinterleaving.
 *
 * @{
 */

/** Chebyshev filter object type */
typedef struct lrpt_dsp_filter__ lrpt_dsp_filter_t;

/** Supported Chebyshev filter types */
typedef enum lrpt_dsp_filter_type__ {
    LRPT_DSP_FILTER_TYPE_LOWPASS,  /**< Lowpass filter */
    LRPT_DSP_FILTER_TYPE_HIGHPASS, /**< Highpass filter */
    LRPT_DSP_FILTER_TYPE_BANDPASS  /**< Bandpass filter */
} lrpt_dsp_filter_type_t;

/** Dediffcoder object type */
typedef struct lrpt_dsp_dediffcoder__ lrpt_dsp_dediffcoder_t;

/** Integer FFT object type */
typedef struct lrpt_dsp_ifft__ lrpt_dsp_ifft_t;

/** @} */

/** \addtogroup postproc Postprocessor
 *
 * Image postprocessing routines.
 *
 * Interfaces for image postprocessing operations such as flipping, histogram normalization,
 * rectification and color enhancement.
 *
 * @{
 */

/** @} */

/** \addtogroup demod Demodulator
 *
 * QPSK demodulation routines.
 *
 * Interfaces for performing QPSK demodulation.
 *
 * @{
 */

/** Demodulator object type */
typedef struct lrpt_demodulator__ lrpt_demodulator_t;

/** @} */

/** \addtogroup decoder Decoder
 *
 * LRPT data decoder routines.
 *
 * Interfaces for performing LRPT data decoding.
 *
 * @{
 */

/** Decoder object type */
typedef struct lrpt_decoder__ lrpt_decoder_t;

/** Supported spacecrafts */
typedef enum lrpt_decoder_spacecraft__ {
    LRPT_DECODER_SC_METEORM2,   /**< Meteor-M2 */
    LRPT_DECODER_SC_METEORM2_1, /**< Meteor-M2-1 */
    LRPT_DECODER_SC_METEORM2_2  /**< Meteor-M2-2 */
} lrpt_decoder_spacecraft_t;

/** @} */

/*************************************************************************************************/

/** \addtogroup common
 * @{
 */

/** Allocate I/Q data object.
 *
 * Tries to allocate I/Q data object of requested length \p len. If zero length is requested
 * empty object will be allocated but it will be possible to resize it later with
 * #lrpt_iq_data_resize(). User should free object with #lrpt_iq_data_free() after use.
 *
 * \param len Length of new I/Q data object in number of I/Q samples.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated I/Q data object or \c NULL in case of error.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_alloc(
        size_t len,
        lrpt_error_t *err);

/** Free I/Q data object.
 *
 * \param data Pointer to the I/Q data object.
 */
LRPT_API void lrpt_iq_data_free(
        lrpt_iq_data_t *data);

/** Length of I/Q data object.
 *
 * \param data Pointer to the I/Q data object.
 *
 * \return Number of I/Q samples currently stored in \p data. \c 0 will be returned for \c NULL
 * \p data.
 */
LRPT_API size_t lrpt_iq_data_length(
        const lrpt_iq_data_t *data);

/** Resize existing I/Q data object.
 *
 * \p data will be resized to fit requested number \p new_len of I/Q samples. If new I/Q samples
 * were allocated during resize they will be initialized to \c 0. If zero new length is requested
 * I/Q data object will be set to the empty object.
 *
 * \param data Pointer to the I/Q data object.
 * \param new_len Length \p data will be resized to.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resize or \c false in case of error.
 *
 * \note In case of error \p data object will not be modified.
 */
LRPT_API bool lrpt_iq_data_resize(
        lrpt_iq_data_t *data,
        size_t new_len,
        lrpt_error_t *err);

/** Append I/Q data to the existing I/Q data object.
 *
 * Adds \p n I/Q samples from \p data_src object starting with position \p offset to the end of
 * \p data_dest object. If \p n exceeds available number of I/Q samples in \p data_src
 * (considering offset) all I/Q samples starting from \p offset will be appended.
 *
 * \param[in,out] data_dest Pointer to the destination I/Q data object.
 * \param[in] data_src Pointer to the source I/Q data object.
 * \param offset How much I/Q samples should be skipped from the beginning of \p data_src.
 * \param n Number of I/Q samples to append.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull append or \c false in case of error.
 *
 * \warning \p data_dest and \p data_src can't be the same object!
 *
 * \note In case of error \p data_dest object will not be modified.
 */
LRPT_API bool lrpt_iq_data_append(
        lrpt_iq_data_t *data_dest,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Copy a part of I/Q samples to another I/Q data object.
 *
 * Copies \p n I/Q samples from \p data_src object starting with position \p offset to the
 * \p data_dest object which will be auto-resized to fit requested number of I/Q samples.
 * If \p n exceeds available number of I/Q samples in \p data_src (considering offset) all
 * I/Q samples starting from \p offset will be copied.
 *
 * \param[out] data_dest Pointer to the destination I/Q data object.
 * \param[in] data_src Pointer to the source I/Q data object.
 * \param offset How much I/Q samples should be skipped from the beginning of \p data_src.
 * \param n Number of I/Q samples to copy.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull copying or \c false in case of error.
 *
 * \warning \p data_dest and \p data_src can't be the same object!
 *
 * \note In case of error \p data_dest object will not be modified.
 */
LRPT_API bool lrpt_iq_data_from_iq(
        lrpt_iq_data_t *data_dest,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Create I/Q data object from a part of another I/Q data object.
 *
 * This function behaves similarly to #lrpt_iq_data_from_iq(), however, it allocates I/Q data
 * object automatically. User should free object with #lrpt_iq_data_free() after use.
 *
 * \param[in] data_src Pointer to the source I/Q data object.
 * \param offset How much I/Q samples should be skipped from the beginning of \p data_src.
 * \param n Number of I/Q samples to copy.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated I/Q data object or \c NULL in case of error.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_iq(
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert array of complex I/Q samples to the I/Q data object.
 *
 * Converts \p n I/Q samples from array of complex I/Q samples \p samples starting with position
 * \p offset to the internal format of I/Q data storage and saves them to \p data_dest object
 * which will be auto-resized to fit requested number of I/Q samples.
 *
 * \param[out] data_dest Pointer to the destination I/Q data object.
 * \param[in] samples Pointer to the source array of complex I/Q samples.
 * \param offset How much I/Q samples should be skipped from the beginning of \p samples.
 * \param n Number of I/Q samples to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull converting or \c false in case of error.
 *
 * \warning User should be responsible that \p samples array contains at least \p n elements
 * starting from \p offset and up to the end!
 *
 * \note In case of error \p data_dest object will not be modified.
 */
LRPT_API bool lrpt_iq_data_from_complex(
        lrpt_iq_data_t *data_dest,
        const _Complex double *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Create I/Q data object from array of complex I/Q samples.
 *
 * This function behaves similarly to #lrpt_iq_data_from_complex(), however, it allocates I/Q
 * data object automatically. User should free object with #lrpt_iq_data_free() after use.
 *
 * \param[in] samples Pointer to the source array of complex I/Q samples.
 * \param offset How much I/Q samples should be skipped from the beginning of \p samples.
 * \param n Number of I/Q samples to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated I/Q data object or \c NULL in case of error.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_complex(
        const _Complex double *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert I/Q data to array of complex I/Q samples.
 *
 * Converts \p n I/Q samples from \p data_src object starting with position \p offset to the
 * \p samples array. If \p n exceeds available number of I/Q samples in \p data_src (considering
 * offset) all I/Q samples starting from \p offset will be converted.
 *
 * \param[out] samples Pointer to the destination array of complex I/Q samples.
 * \param[in] data_src Pointer to the source I/Q data object.
 * \param offset How much I/Q samples should be skipped from the beginning of \p data_src.
 * \param n Number of I/Q samples to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful conversion or \c false in case of error.
 *
 * \warning User should be responsible that \p samples array is large enough to keep at least
 * \p n elements!
 *
 * \note In case of error \p samples array will not be modified.
 */
LRPT_API bool lrpt_iq_data_to_complex(
        _Complex double *samples,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert array of adjacent double-valued I/Q samples to the I/Q data object.
 *
 * Converts \p n I/Q samples from array of adjacent double-valued I/Q samples \p samples
 * starting with position \p offset to the internal format of I/Q data storage and saves them to
 * \p data_dest object which will be auto-resized to fit requested number of I/Q samples.
 *
 * \param[out] data_dest Pointer to the destination I/Q data object.
 * \param[in] samples Pointer to the source array of adjacent double-valued I/Q samples.
 * \param offset How much I/Q samples of internal format should be skipped from the beginning
 * of \p samples.
 * \param n Number of I/Q samples of internal format to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull converting or \c false in case of error.
 *
 * \note 1 I/Q sample of internal format equals to 2 double-valued I/Q samples.
 *
 * \warning User should be responsible that \p samples array contains at least \c 2x \p n
 * elements starting from \c 2x \p offset and up to the end!
 *
 * \note In case of error \p data_dest object will not be modified.
 */
LRPT_API bool lrpt_iq_data_from_doubles(
        lrpt_iq_data_t *data_dest,
        const double *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Create I/Q data object from array of adjacent double-valued I/Q samples.
 *
 * This function behaves similarly to #lrpt_iq_data_from_doubles(), however, it allocates I/Q
 * data object automatically. User should free object with #lrpt_iq_data_free() after use.
 *
 * \param[in] samples Pointer to the source array of adjacent double-valued I/Q samples.
 * \param offset How much I/Q samples of internal format should be skipped from the beginning
 * of \p samples.
 * \param n Number of I/Q samples of internal format to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated I/Q data object or \c NULL in case of error.
 *
 * \note 1 I/Q sample of internal format equals to 2 double-valued I/Q samples.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_doubles(
        const double *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert I/Q data to the array of adjacent double-valued I/Q samples.
 *
 * Converts \p n I/Q samples from \p data_src object starting with position \p offset to the
 * \p samples array. If \p n exceeds available number of I/Q samples in \p data_src (considering
 * offset) all I/Q samples starting from \p offset will be converted.
 *
 * \param[out] samples Pointer to the destination array of adjacent double-valued I/Q samples.
 * \param[in] data_src Pointer to the source I/Q data object.
 * \param offset How much I/Q samples should be skipped from the beginning of \p data_src.
 * \param n Number of I/Q samples to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful conversion or \c false in case of error.
 *
 * \warning User should be responsible that \p samples array is large enough to keep at least
 * \c 2x \p n elements!
 *
 * \note In case of error \p samples array will not be modified.
 */
LRPT_API bool lrpt_iq_data_to_doubles(
        double *samples,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Allocate I/Q ring buffer object.
 *
 * Tries to allocate I/Q ring buffer object of requested length \p len. User should object with
 * #lrpt_iq_rb_free() after use.
 *
 * \param len Length of new I/Q ring buffer object in number of I/Q samples.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated I/Q ring buffer object or \c NULL in case of error.
 */
LRPT_API lrpt_iq_rb_t *lrpt_iq_rb_alloc(
        size_t len,
        lrpt_error_t *err);

/** Free I/Q ring buffer object.
 *
 * \param rb Pointer to the I/Q ring buffer object.
 */
LRPT_API void lrpt_iq_rb_free(
        lrpt_iq_rb_t *rb);

/** Length of I/Q ring buffer object.
 *
 * \param rb Pointer to the I/Q ring buffer object.
 *
 * \return Number of I/Q samples that can be stored in \p rb. \c 0 will be returned for \c NULL
 * \p rb.
 */
LRPT_API size_t lrpt_iq_rb_length(
        const lrpt_iq_rb_t *rb);

/** Number of used samples in I/Q ring buffer object.
 *
 * \param rb Pointer to the I/Q ring buffer object.
 *
 * \return Number of I/Q samples stored in \p rb. \c 0 will be returned for \c NULL \p rb.
 */
LRPT_API size_t lrpt_iq_rb_used(
        const lrpt_iq_rb_t *rb);

/** Number of available samples in I/Q ring buffer object.
 *
 * \param rb Pointer to the I/Q ring buffer object.
 *
 * \return Number of available I/Q samples for storage in \p rb. \c 0 will be returned for
 * \c NULL \p rb.
 */
LRPT_API size_t lrpt_iq_rb_avail(
        const lrpt_iq_rb_t *rb);

/** Check if I/Q ring buffer object is empty.
 *
 * \param rb Pointer to the I/Q ring buffer object.
 *
 * \return \c true if ring buffer is fully empty or \c false if at least one I/Q sample
 * is stored inside \p rb. \c false will be returned also for \c NULL \p rb.
 */
LRPT_API bool lrpt_iq_rb_is_empty(
        const lrpt_iq_rb_t *rb);

/** Check if I/Q ring buffer object is full.
 *
 * \param rb Pointer to the I/Q ring buffer object.
 *
 * \return \c true if ring buffer is totally full or \c false if at least one I/Q sample can
 * be placed inside \p rb. \c false will be returned also for \c NULL \p rb.
 */
LRPT_API bool lrpt_iq_rb_is_full(
        const lrpt_iq_rb_t *rb);

/** Get I/Q data from I/Q ring buffer object.
 *
 * Pops requested number of I/Q samples from \p rb object and frees occupied space for future
 * usage. Resulting data will be stored in \p data_dest object. If \p n is greater than remaining
 * number of I/Q samples in ring buffer all remaining samples will be popped.
 *
 * \param rb Pointer to the source I/Q ring buffer object.
 * \param[out] data_dest Pointer to the destination I/Q data object.
 * \param n Number of I/Q samples to pop.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful popping or \c false in case of error.
 *
 * \note In case of error \p data_dest and \p rb objects will not be modified.
 */
LRPT_API bool lrpt_iq_rb_pop(
        lrpt_iq_rb_t *rb,
        lrpt_iq_data_t *data_dest,
        size_t n,
        lrpt_error_t *err);

/** Put I/Q data to I/Q ring buffer object.
 *
 * Pushes requested number of I/Q samples to \p rb object. If \p n exceeds available number of
 * I/Q samples in \p data_src (considering offset) all I/Q samples starting from \p offset
 * will be pushed.
 *
 * \param rb Pointer to the I/Q ring buffer object.
 * \param[in] data_src Pointer to the source I/Q data object.
 * \param offset How much I/Q samples should be skipped from the beginning of \p data_src.
 * \param n Number of I/Q samples to push.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful pushing or \c false in case of error.
 *
 * \note In case of error \p rb object will not be modified.
 */
LRPT_API bool lrpt_iq_rb_push(
        lrpt_iq_rb_t *rb,
        const lrpt_iq_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Allocate QPSK data object.
 *
 * Tries to allocate QPSK data object of requested length \p len. If zero length is requested
 * empty object will be allocated but it will be possible to resize it later with
 * #lrpt_qpsk_data_resize(). User should free object with #lrpt_qpsk_data_free() after use.
 *
 * \param len Length of new QPSK data object in number of symbols.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK data object or \c NULL in case of error.
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(
        size_t len,
        lrpt_error_t *err);

/** Free QPSK data object.
 *
 * \param data Pointer to the QPSK data object.
 */
LRPT_API void lrpt_qpsk_data_free(
        lrpt_qpsk_data_t *data);

/** Length of QPSK data object.
 *
 * \param data Pointer to the QPSK data object.
 *
 * \return Number of QPSK symbols currently stored in \p data. \c 0 will be returned for \c NULL
 * \p data.
 */
LRPT_API size_t lrpt_qpsk_data_length(
        const lrpt_qpsk_data_t *data);

/** Resize existing QPSK data object.
 *
 * \p data will be resized to fit requested number \p new_len of QPSK symbols If new QPSK symbols
 * were allocated during resize they will be initialized to \c 0. If zero new length is requested
 * QPSK data object will be set to the empty object.
 *
 * \param data Pointer to the QPSK data object.
 * \param new_len Length \p data will be resized to.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resize or \c false in case of error.
 *
 * \note In case of error \p data object will not be modified.
 */
LRPT_API bool lrpt_qpsk_data_resize(
        lrpt_qpsk_data_t *data,
        size_t new_len,
        lrpt_error_t *err);

/** Append QPSK data to the existing QPSK data object.
 *
 * Adds \p n QPSK symbols from \p data_src object starting with position \p offset to the end of
 * \p data_dest object. If \p n exceeds available number of QPSK symbols in \p data_src
 * (considering offset) all QPSK symbols starting from \p offset will be appended.
 *
 * \param[in,out] data_dest Pointer to the destination QPSK data object.
 * \param[in] data_src Pointer to the source QPSK data object.
 * \param offset How much QPSK symbols should be skipped from the beginning of \p data_src.
 * \param n Number of QPSK symbols to append.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull append or \c false in case of error.
 *
 * \warning \p data_dest and \p data_src can't be the same object!
 *
 * \note In case of error \p data_dest object will not be modified.
 */
LRPT_API bool lrpt_qpsk_data_append(
        lrpt_qpsk_data_t *data_dest,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Copy a part of QPSK symbols to another QPSK data object.
 *
 * Copies \p n QPSK symbols from \p data_src object starting with position \p offset to the
 * \p data_dest object which will be auto-resized to fit requested number of QPSK symbols.
 * If \p n exceeds available number of QPSK symbols in \p data_src (considering offset) all
 * QPSK symbols starting from \p offset will be copied.
 *
 * \param[out] data_dest Pointer to the destination QPSK data object.
 * \param[in] data_src Pointer to the source QPSK data object.
 * \param offset How much QPSK symbols should be skipped from the beginning of \p data_src.
 * \param n Number of QPSK symbols to copy.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull copying or \c false in case of error.
 *
 * \warning \p data_dest and \p data_src can't be the same object!
 *
 * \note In case of error \p data_dest object will not be modified.
 */
LRPT_API bool lrpt_qpsk_data_from_qpsk(
        lrpt_qpsk_data_t *data_dest,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Create QPSK data object from a part of another QPSK data object.
 *
 * This function behaves similarly to #lrpt_qpsk_data_from_qpsk(), however, it allocates QPSK
 * data object automatically. User should free object with #lrpt_qpsk_data_free() after use.
 *
 * \param[in] data_src Pointer to the source QPSK data object.
 * \param offset How much QPSK symbols should be skipped from the beginning of \p data_src.
 * \param n Number of QPSK symbols to copy.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK data object or \c NULL in case of error.
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_qpsk(
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert array of soft QPSK symbols to the QPSK data object.
 *
 * Converts \p n QPSK symbols from array of soft QPSK symbols \p symbols starting with position
 * \p offset to the internal format of QPSK data storage and saves them to \p data_dest object
 * which will be auto-resized to fit requested number of QPSK symbols.
 *
 * \param[out] data_dest Pointer to the destination QPSK data object.
 * \param[in] symbols Pointer to the source array of soft QPSK symbols.
 * \param offset How much QPSK symbols of internal format should be skipped from the beginning
 * of \p symbols.
 * \param n Number of QPSK symbols of internal format to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull converting or \c false in case of error.
 *
 * \note 1 QPSK symbol of internal format equals to 2 soft QPSK symbols.
 *
 * \warning User should be responsible that \p symbols array contains at least \c 2x \p n elements
 * starting from \c 2x \p offset and up to the end!
 */
LRPT_API bool lrpt_qpsk_data_from_soft(
        lrpt_qpsk_data_t *data_dest,
        const int8_t *symbols,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Create QPSK data object from array of soft QPSK symbols.
 *
 * This function behaves similarly to #lrpt_qpsk_data_from_soft(), however, it allocates QPSK
 * data object automatically. User should free object with #lrpt_qpsk_data_free() after use.
 *
 * \param[in] symbols Pointer to the source array of soft QPSK symbols.
 * \param offset How much QPSK samples of internal format should be skipped from the beginning
 * of \p symbols.
 * \param n Number of QPSK symbols of internal format to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK data object or \c NULL in case of error.
 *
 * \note 1 QPSK symbol of internal format equals to 2 soft QPSK symbols.
  */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_soft(
        const int8_t *symbols,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert QPSK data to the array of soft QPSK symbols.
 *
 * Converts \p n QPSK symbols from \p data_src object starting with position \p offset to the
 * \p symbols array. If \p n exceeds available number of QPSK symbols in \p data_src (considering
 * offset) all QPSK symbols starting from \p offset will be converted.
 *
 * \param[out] symbols Pointer to the destination array of soft QPSK symbols.
 * \param[in] data_src Pointer to the source QPSK data object.
 * \param offset How much QPSK symbols should be skipped from the beginning of \p data_src.
 * \param n Number of QPSK symbols to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful conversion or \c false in case of error.
 *
 * \warning User should be responsible that \p symbols array is large enough to keep at least
 * \c 2x \p n elements!
 *
 * \note In case of error \p symbols array will not be modified.
 */
LRPT_API bool lrpt_qpsk_data_to_soft(
        int8_t *symbols,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert array of hard QPSK symbols to the QPSK data object.
 *
 * Converts \p n QPSK symbols from array of hard QPSK symbols \p symbols to the internal format
 * of QPSK data storage and saves them to \p data_dest object which will be auto-resized to fit
 * requested number of QPSK symbols.
 *
 * \param[out] data_dest Pointer to the destination QPSK data object.
 * \param[in] symbols Pointer to the source array of hard QPSK symbols.
 * \param n Number of QPSK symbols of internal format to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull converting or \c false in case of error.
 *
 * \note 4 QPSK symbols of internal format equal to 1 hard QPSK symbol.
 *
 * \warning User should be responsible that \p symbols array contains at least \c 1/4 of \p n
 * elements!
 *
 * \todo Implement offsetted access.
 */
LRPT_API bool lrpt_qpsk_data_from_hard(
        lrpt_qpsk_data_t *data_dest,
        const unsigned char *symbols,
        size_t n,
        lrpt_error_t *err);

/** Create QPSK data object from array of hard QPSK symbols.
 *
 * This function behaves similarly to #lrpt_qpsk_data_from_hard(), however, it allocates QPSK
 * data object automatically. User should free object with #lrpt_qpsk_data_free() after use.
 *
 * \param[in] symbols Pointer to the source array of hard QPSK symbols.
 * \param n Number of QPSK symbols of internal format to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK data object or \c NULL in case of error.
 *
 * \note 4 QPSK symbols of internal format equal to 1 hard QPSK symbol.
 *
 * \todo Implement offsetted access
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_hard(
        const unsigned char *symbols,
        size_t n,
        lrpt_error_t *err);

/** Convert QPSK data to the array of hard QPSK symbols.
 *
 * Converts \p n QPSK symbols from \p data_src object starting with position \p offset to the
 * \p symbols array. If \p n exceeds available number of QPSK symbols in \p data_src (considering
 * offset) all QPSK symbols starting from \p offset will be converted.
 *
 * \param[out] symbols Pointer to the destination array of hard QPSK symbols.
 * \param[in] data_src Pointer to the source QPSK data object.
 * \param offset How much QPSK symbols should be skipped from the beginning of \p data_src.
 * \param n Number of QPSK symbols to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful conversion or \c false in case of error.
 *
 * \warning User should be responsible that \p symbols array is large enough to keep at least
 * \c 1/4 \p n elements!
 *
 * \note In case of error \p symbols array will not be modified.
 */
LRPT_API bool lrpt_qpsk_data_to_hard(
        unsigned char *symbols,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Allocate QPSK ring buffer object.
 *
 * Tries to allocate QPSK ring buffer object of requested \p len. User should free object with
 * #lrpt_qpsk_rb_free() after use.
 *
 * \param len Length of new QPSK ring buffer object in number of QPSK symbols.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK ring buffer object or \c NULL in case of erro.
 */
LRPT_API lrpt_qpsk_rb_t *lrpt_qpsk_rb_alloc(
        size_t len,
        lrpt_error_t *err);

/** Free QPSK ring buffer object.
 *
 * \param rb Pointer to the QPSK ring buffer object.
 */
LRPT_API void lrpt_qpsk_rb_free(
        lrpt_qpsk_rb_t *rb);

/** Length of QPSK ring buffer.
 *
 * \param rb Pointer to the QPSK ring buffer object.
 *
 * \return Number of QPSK symbols that can be stored in \p rb. \c 0 will be returned for if \c NULL
 * \p rb.
 */
LRPT_API size_t lrpt_qpsk_rb_length(
        const lrpt_qpsk_rb_t *rb);

/** Number of used symbols in QPSK ring buffer object.
 *
 * \param rb Pointer to the QPSK ring buffer object.
 *
 * \return Number of QPSK symbols stored in \p rb. \c 0 will be returned for \c NULL \p rb.
 */
LRPT_API size_t lrpt_qpsk_rb_used(
        const lrpt_qpsk_rb_t *rb);

/** Number of available symbols in QPSK ring buffer object.
 *
 * \param rb Pointer to the QPSK ring buffer object.
 *
 * \return Number of available QPSK symbols for storage in \p rb. \c 0 will be returned for
 * \c NULL \p rb.
 */
LRPT_API size_t lrpt_qpsk_rb_avail(
        const lrpt_qpsk_rb_t *rb);

/** Check if QPSK ring buffer object is empty.
 *
 * \param rb Pointer to the QPSK ring buffer object.
 *
 * \return \c true if ring buffer is fully empty or \c false if at least one QPSK symbol
 * is stored inside \p rb. \c false will be returned also for \c NULL \p rb.
 */
LRPT_API bool lrpt_qpsk_rb_is_empty(
        const lrpt_qpsk_rb_t *rb);

/** Check if QPSK ring buffer object is full.
 *
 * \param rb Pointer to the QPSK ring buffer object.
 *
 * \return \c true if ring buffer is totally full or \c false if at least one QPSK symbol can
 * be placed inside \p rb. \c false will be returned also for \c NULL \p rb.
 */
LRPT_API bool lrpt_qpsk_rb_is_full(
        const lrpt_qpsk_rb_t *rb);

/** Get QPSK data from QPSK ring buffer object.
 *
 * Pops requested number of QPSK symbols from \p rb object and frees occupied space for future
 * usage. Resulting data will be stored in \p data_dest object. If \p n is greater than remaining
 * number of QPSK symbols in ring buffer all remaining symbols will be popped.
 *
 * \param rb Pointer to the source QPSK ring buffer object.
 * \param[out] data_dest Pointer to the destination QPSK data object.
 * \param n Number of QPSK symbols to pop.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful popping or \c false otherwise.
 *
 * \note In case of error \p data_dest and \p rb objects will not be modified.
 */
LRPT_API bool lrpt_qpsk_rb_pop(
        lrpt_qpsk_rb_t *rb,
        lrpt_qpsk_data_t *data_dest,
        size_t n,
        lrpt_error_t *err);

/** Put QPSK data to QPSK ring buffer object.
 *
 * Pushes requested number of QPSK symbols to \p rb object. If \p n exceeds available number of
 * QPSK samples in \p data_src (considering offset) all QPSK samples starting from \p offset
 * will be pushed.
 *
 * \param rb Pointer to the QPSK ring buffer object.
 * \param[in] data_src Pointer to the source QPSK data object.
 * \param offset How much QPSK samples should be skipped from the beginning of \p data_src.
 * \param n Number of QPSK symbols to push.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful pushing or \c false in case of error.
 *
 * \note In case of error \p rb object will not be modified.
 */
LRPT_API bool lrpt_qpsk_rb_push(
        lrpt_qpsk_rb_t *rb,
        const lrpt_qpsk_data_t *data_src,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Allocate LRPT image object.
 *
 * Tries to allocate LRPT image object of requested width \p width and height \p height. If either
 * width or height is zero empty image object will be allocated. User should free object with
 * #lrpt_image_free() after use.
 *
 * \param width Width of the image in number of pixels.
 * \param height Height of the image in number of pixels.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated LRPT image object or \c NULL in case of error.
 */
LRPT_API lrpt_image_t *lrpt_image_alloc(
        size_t width,
        size_t height,
        lrpt_error_t *err);

/** Free LRPT image object.
 *
 * \param image Pointer to the LRPT image object.
 */
LRPT_API void lrpt_image_free(
        lrpt_image_t *image);

/** Width of LRPT image object.
 *
 * \param image Pointer to the LRPT image object.
 *
 * \return Width of LRPT image object in pixels. \c 0 will be returned for \c NULL \p image.
 */
LRPT_API size_t lrpt_image_width(
        const lrpt_image_t *image);

/** Height of LRPT image object.
 *
 * \param image Pointer to the LRPT image object.
 *
 * \return Height of LRPT image object in pixels. \c 0 will be returned for \c NULL \p image.
 */
LRPT_API size_t lrpt_image_height(
        const lrpt_image_t *image);

/** Resize LRPT image object and set new width.
 *
 * \param image Pointer to the LRPT image object.
 * \param new_width New image width in number of pixels.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resize or \c false in case of error.
 *
 * \note In case of error \p image object will not be modified.
 */
LRPT_API bool lrpt_image_set_width(
        lrpt_image_t *image,
        size_t new_width,
        lrpt_error_t *err);

/** Resize LRPT image object and set new height.
 *
 * \param image Pointer to the LRPT image object.
 * \param new_height New image height in number of pixels.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resize or \c false in case of error.
 *
 * \note In case of error \p image object will not be modified.
 */
LRPT_API bool lrpt_image_set_height(
        lrpt_image_t *image,
        size_t new_height,
        lrpt_error_t *err);

/** Get pixel value.
 *
 * Gets pixel value for specified absolute position and selected APID. APIDs that are outside of
 * 64-69 range are not supported.
 *
 * \param image Pointer to the LRPT image object.
 * \param apid APID number (must be within 64-69 range).
 * \param pos Absolute position of pixel.
 *
 * \return Pixel value or \c 0 in case of error.
 */
LRPT_API uint8_t lrpt_image_get_px(
        const lrpt_image_t *image,
        uint8_t apid,
        size_t pos);

/** Set pixel value.
 *
 * Sets pixel value for specified absolute position and selected APID. APIDs that are outside of
 * 64-69 range are not supported.
 *
 * \param image Pointer to the LRPT image object.
 * \param apid APID number (must be within 64-69 range).
 * \param pos Absolute position of pixel.
 * \param val Value of pixel.
 */
LRPT_API void lrpt_image_set_px(
        lrpt_image_t *image,
        uint8_t apid,
        size_t pos,
        uint8_t val);

/** Initialize error object.
 *
 * Creates error object with default level and code. Internal string buffer for error message
 * is set to \c NULL.
 *
 * \return Pointer to the allocated error object or \c NULL in case of error.
 */
LRPT_API lrpt_error_t *lrpt_error_init(void);

/** Free error object.
 *
 * \param err Pointer to the error object.
 */
LRPT_API void lrpt_error_deinit(
        lrpt_error_t *err);

/** Reset resources used by error object.
 *
 * Resets error level and code and frees internal message buffer used by error object. Object will
 * not be freed at all.
 *
 * \param err Pointer to the error object.
 */
LRPT_API void lrpt_error_reset(
        lrpt_error_t *err);

/** Numerical error level.
 *
 * \param err Pointer to the error object.
 *
 * \return Numerical error level category (see #lrpt_error_level_t). \c 0 will be returned for
 * \c NULL \p err.
 */
LRPT_API lrpt_error_level_t lrpt_error_level(
        const lrpt_error_t *err);

/** Numerical error code.
 *
 * \param err Pointer to the error object.
 *
 * \return Numerical error code (see #lrpt_error_code_t). \c 0 will be returned for \c NULL \p err.
 */
LRPT_API lrpt_error_code_t lrpt_error_code(
        const lrpt_error_t *err);

/** Error message string.
 *
 * \param err Pointer to the error object.
 *
 * \return Pointer to the error message string or \c NULL for \c NULL \p err or if message is empty.
 *
 * \warning User should never free returned string pointer! Use #lrpt_error_deinit() or
 * #lrpt_error_reset() instead!
 */
LRPT_API const char * lrpt_error_message(
        const lrpt_error_t *err);

/** @} */

/** \addtogroup io
 * @{
 */

/** Open I/Q data file for reading.
 *
 * File format is described at \ref lrptiq section. User should close file properly with
 * #lrpt_iq_file_close() after use.
 *
 * \param fname Name of file to read I/Q data from.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the readable I/Q data file object or \c NULL in case of error.
 */
LRPT_API lrpt_iq_file_t *lrpt_iq_file_open_r(
        const char *fname,
        lrpt_error_t *err);

/** Open I/Q data file, Version 1 for writing.
 *
 * File format is described at \ref lrptiq section. User should close file properly with
 * #lrpt_iq_file_close() after use.
 *
 * \param fname Name of file to write I/Q data to.
 * \param offset Whether offset QPSK was used or not.
 * \param samplerate Sampling rate, samples per second.
 * \param bandwidth Bandwidth of the signal in Hertz.
 * \param device_name Device name string. If \p device_name is \c NULL no device name info will
 * be written to the file.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the writable I/Q file object or \c NULL in case of error.
 */
LRPT_API lrpt_iq_file_t *lrpt_iq_file_open_w_v1(
        const char *fname,
        bool offset,
        uint32_t samplerate,
        uint32_t bandwidth,
        const char *device_name,
        lrpt_error_t *err);

/** Close I/Q data file.
 *
 * \param file Pointer to the I/Q data file object.
 */
LRPT_API void lrpt_iq_file_close(
        lrpt_iq_file_t *file);

/** I/Q data file format version info.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return File version number info. \c 0 will be returned for \c NULL \p file.
 */
LRPT_API uint8_t lrpt_iq_file_version(
        const lrpt_iq_file_t *file);

/** Check if file has offset QPSK modulation.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return \c true if offset QPSK is used or \c false otherwise. \c false also will be returned
 * for \c NULL \p file.
 */
LRPT_API bool lrpt_iq_file_is_offsetted(
        const lrpt_iq_file_t *file);

/** I/Q data file sampling rate.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Sampling rate for I/Q data file. \c 0 will be returned for \c NULL \p file.
 */
LRPT_API uint32_t lrpt_iq_file_samplerate(
        const lrpt_iq_file_t *file);

/** I/Q data file bandwidth.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Signal bandwidth of I/Q data file. \c 0 will be returned for \c NULL \p file.
 */
LRPT_API uint32_t lrpt_iq_file_bandwidth(
        const lrpt_iq_file_t *file);

/** Device name which was used during file creation.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Pointer to the device name string. \c NULL will be returned if no device name was set
 * or if \p file is \c NULL.
 *
 * \warning User should never free returned string pointer! Use #lrpt_iq_file_close() instead!
 */
LRPT_API const char *lrpt_iq_file_devicename(
        const lrpt_iq_file_t *file);

/** Number of I/Q samples in file.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Number of I/Q samples stored in file. \c 0 will be returned for \c NULL \p file.
 */
LRPT_API uint64_t lrpt_iq_file_length(
        const lrpt_iq_file_t *file);

/** Set current position in I/Q data file stream.
 *
 * \param file Pointer to the I/Q data file object.
 * \param sample I/Q sample index to set file pointer to. Index enumeration starts with 0.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull positioning or \c false in case of error. \c false also will be
 * returned for \c NULL \p file.
 *
 * \warning Seeking is disabled for files opened with write mode (\c false will be returned).
 *
 * \note If positioning has failed internal I/Q sample pointer will not be changed.
 *
 * \note If seekable sample index exceeds file length I/Q sample pointer will be set to the last
 * I/Q sample.
 */
LRPT_API bool lrpt_iq_file_goto(
        lrpt_iq_file_t *file,
        uint64_t sample,
        lrpt_error_t *err);

/** Read I/Q data from file.
 *
 * Reads \p n consecutive I/Q samples from I/Q data file \p file to \p data_dest object.
 * If \p n exceeds available number of I/Q samples all I/Q samples up to the end of file will be
 * read.
 *
 * \param[out] data_dest Pointer to the destination I/Q data object.
 * \param[in] file Pointer to the source I/Q data file object.
 * \param n Number of I/Q samples to read.
 * \param rewind If set to \c true, sample position in file stream will be restored after reading.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull reading or \c false in case of error.
 */
LRPT_API bool lrpt_iq_data_read_from_file(
        lrpt_iq_data_t *data_dest,
        lrpt_iq_file_t *file,
        size_t n,
        bool rewind,
        lrpt_error_t *err);

/** Write I/Q data to file.
 *
 * Writes the whole contents of \p data_src object to the I/Q data file \p file.
 *
 * \param[in] data_src Pointer to the source I/Q data object.
 * \param[out] file Pointer to the destination I/Q data file object.
 * \param inplace Determines whether data length should be dumped as soon as possible (after every
 * chunk, slower, more robust) or at the end of writing (faster).
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull writing or \c false in case of error.
 *
 * \todo Implement custom length writing with custom starting offset.
 */
LRPT_API bool lrpt_iq_data_write_to_file(
        const lrpt_iq_data_t *data_src,
        lrpt_iq_file_t *file,
        bool inplace,
        lrpt_error_t *err);

/** Open QPSK data file for reading.
 *
 * File format is described at \ref lrptqpsk section. User should close file properly with
 * #lrpt_qpsk_file_close() after use.
 *
 * \param fname Name of file to read QPSK data from.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the readable QPSK data file object or \c NULL in case of error.
 */
LRPT_API lrpt_qpsk_file_t *lrpt_qpsk_file_open_r(
        const char *fname,
        lrpt_error_t *err);

/** Open QPSK data file, Version 1 for writing.
 *
 * File format is described at \ref lrptqpsk section. User should close file properly with
 * #lrpt_qpsk_file_close() after use.
 *
 * \param fname Name of file to write QPSK data to.
 * \param differential Whether differential coding was used or not.
 * \param interleaved Whether interleaving was used or not.
 * \param hard Whether data is in hard symbols format (if \p hard is \c true) or soft
 * (\p hard is \c false).
 * \param symrate Symbol rate, symbols per second.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the writable QPSK file object or \c NULL in case of error.
 */
LRPT_API lrpt_qpsk_file_t *lrpt_qpsk_file_open_w_v1(
        const char *fname,
        bool differential,
        bool interleaved,
        bool hard,
        uint32_t symrate,
        lrpt_error_t *err);

/** Close QPSK data file.
 *
 * \param file Pointer to the QPSK data file object.
 */
LRPT_API void lrpt_qpsk_file_close(
        lrpt_qpsk_file_t *file);

/** QPSK data file format version info.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return File version number info. \c 0 will be returned for \c NULL \p file.
 */
LRPT_API uint8_t lrpt_qpsk_file_version(
        const lrpt_qpsk_file_t *file);

/** Check if file has differential coding.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return \c true if differential coding is used or \c false otherwise. \c false also will be
 * returned for \c NULL \p file.
 */
LRPT_API bool lrpt_qpsk_file_is_diffcoded(
        const lrpt_qpsk_file_t *file);

/** Check if file has interleaving.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return \c true if interleaving is used or \c false otherwise. \c false also will be returned
 * for \c NULL \p file.
 */
LRPT_API bool lrpt_qpsk_file_is_interleaved(
        const lrpt_qpsk_file_t *file);

/** Check whether symbols are in hard format.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return \c true if symbols are in hard format or \c false otherwise. \c false will be returned
 * for \c NULL \p file.
 */
LRPT_API bool lrpt_qpsk_file_is_hardsymboled(
        const lrpt_qpsk_file_t *file);

/** QPSK data file symbol rate.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return Symbol rate for QPSK data file. \c 0 will be returned for \c NULL \p file.
 */
LRPT_API uint32_t lrpt_qpsk_file_symrate(
        const lrpt_qpsk_file_t *file);

/** Number of QPSK symbols in file.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return Number of QPSK symbols stored in file. \c 0 will be returned for \c NULL \p file.
 */
LRPT_API uint64_t lrpt_qpsk_file_length(
        const lrpt_qpsk_file_t *file);

/** Set current position in QPSK data file stream.
 *
 * \param file Pointer to the QPSK data file object.
 * \param symbol Symbol index to set file pointer to. Index enumeration starts with 0.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull positioning or \c false in case of error. \c false also will be
 * returned for \c NULL \p file.
 *
 * \warning Seeking is disabled for files opened with write mode (\c false will be returned).
 *
 * \note If positioning has failed internal QPSK symbol pointer will not be changed.
 *
 * \note If seekable symbol index exceeds file length QPSK symbol pointer will be set to the last
 * QPSK symbol.
 */
LRPT_API bool lrpt_qpsk_file_goto(
        lrpt_qpsk_file_t *file,
        uint64_t symbol,
        lrpt_error_t *err);

/** Read QPSK data from file.
 *
 * Reads \p n consecutive QPSK symbols from QPSK data file \p file to \p data_dest object.
 * If \p n exceeds available number of QPSK symbols all QPSK symbols up to the end of file will be
 * read.
 *
 * \param[out] data_dest Pointer to the destination QPSK data object.
 * \param[in] file Pointer to the source QPSK data file object.
 * \param n Number of QPSK symbols to read.
 * \param rewind If set to \c true, symbol position in file stream will be restored after reading.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull reading or \c false in case of error.
 */
LRPT_API bool lrpt_qpsk_data_read_from_file(
        lrpt_qpsk_data_t *data_dest,
        lrpt_qpsk_file_t *file,
        size_t n,
        bool rewind,
        lrpt_error_t *err);

/** Write QPSK data to file.
 *
 * Writes the whole contents of \p data_src object to the QPSK data file \p file.
 *
 * \param[in] data_src Pointer to the source QPSK data object.
 * \param[out] file Pointer to the destination QPSK file object.
 * \param inplace Determines whether data length should be dumped as soon as possible (after every
 * chunk, slower, more robust) or at the end of writing (faster).
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull writing or \c false in case of error.
 *
 * \todo Implement custom length writing with custom starting offset.
 */
LRPT_API bool lrpt_qpsk_data_write_to_file(
        const lrpt_qpsk_data_t *data_src,
        lrpt_qpsk_file_t *file,
        bool inplace,
        lrpt_error_t *err);

/** Dump single channel image as PNM file.
 *
 * Saves specified APID channel image to the PNM file format.
 *
 * \param[in] image Pointer to the LRPT image object.
 * \param fname Name of file to save PNM image to.
 * \param apid Number of APID channel to save image from.
 * \param corr Whether to perform BT.709 gamma correction. If \c false linear PNM file will
 * be saved.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull saving or \c false in case of error.
 */
LRPT_API bool lrpt_image_dump_channel_pnm(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid,
        bool corr,
        lrpt_error_t *err);

/** Dump red, green and blue channels as PNM file.
 *
 * Saves specified APID channel images to the PNM file format.
 *
 * \param[in] image Pointer to the LRPT image object.
 * \param fname Name of file to save PNM image to.
 * \param apid_red Number of APID channel to use as red channel.
 * \param apid_green Number of APID channel to use as green channel.
 * \param apid_blue Number of APID channel to use as blue channel.
 * \param corr Whether to perform BT.709 gamma correction. If \c false linear PNM file will
 * be saved.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull saving or \c false in case of error.
 */
LRPT_API bool lrpt_image_dump_combo_pnm(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid_red,
        uint8_t apid_green,
        uint8_t apid_blue,
        bool corr,
        lrpt_error_t *err);

/** Dump single channel as BMP file.
 *
 * Saves specified APID channel image to the BMP file format.
 *
 * \param[in] image Pointer to the LRPT image object.
 * \param fname Name of file to save PNM image to.
 * \param apid Number of APID channel to save image from.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull saving or \c false in case of error.
 */
LRPT_API bool lrpt_image_dump_channel_bmp(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid,
        lrpt_error_t *err);

/** Dump red, green and blue channels as BMP file.
 *
 * Saves specified APID channel images to the BMP file format.
 *
 * \param[in] image Pointer to the LRPT image object.
 * \param fname Name of file to save BMP image to.
 * \param apid_red Number of APID channel to use as red channel.
 * \param apid_green Number of APID channel to use as green channel.
 * \param apid_blue Number of APID channel to use as blue channel.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull saving or \c false in case of error.
 */
LRPT_API bool lrpt_image_dump_combo_bmp(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid_red,
        uint8_t apid_green,
        uint8_t apid_blue,
        lrpt_error_t *err);

/** @} */

/** \addtogroup dsp
 * @{
 */

/** Initialize recursive Chebyshev filter.
 *
 * \param bandwidth Bandwidth of the signal, Hz.
 * \param samplerate Signal sampling rate, samples/s.
 * \param ripple Ripple level, %.
 * \param num_poles Number of filter poles. Must be even, non-zero and not greater than 252!
 * \param type Filter type (see #lrpt_dsp_filter_type_t for supported filter types).
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the Chebyshev filter object or \c NULL in case of error.
 */
LRPT_API lrpt_dsp_filter_t *lrpt_dsp_filter_init(
        uint32_t bandwidth,
        uint32_t samplerate,
        double ripple,
        uint8_t num_poles,
        lrpt_dsp_filter_type_t type,
        lrpt_error_t *err);

/** Free initialized Chebyshev filter object.
 *
 * \param filter Pointer to the Chebyshev filter object.
 */
LRPT_API void lrpt_dsp_filter_deinit(
        lrpt_dsp_filter_t *filter);

/** Apply recursive Chebyshev filter to the I/Q data.
 *
 * \param filter Pointer to the Chebyshev filter object.
 * \param[in,out] data Pointer to the I/Q data object.
 *
 * \return \c false if \p filter and/or \p data are empty or \c true otherwise.
 */
LRPT_API bool lrpt_dsp_filter_apply(
        lrpt_dsp_filter_t *filter,
        lrpt_iq_data_t *data);

/** Allocate and initialize dediffcoder object.
 *
 * Allocates and initializes dediffcoder object for use with QPSK diffcoded data.
 *
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the dediffcoder object or \c NULL in case of error.
 */
LRPT_API lrpt_dsp_dediffcoder_t *lrpt_dsp_dediffcoder_init(
        lrpt_error_t *err);

/** Free previously allocated dediffcoder object.
 *
 * \param dediff Pointer to the dediffcoder object.
 */
LRPT_API void lrpt_dsp_dediffcoder_deinit(
        lrpt_dsp_dediffcoder_t *dediff);

/** Perform dediffcoding of QPSK data.
 *
 * \param dediff Pointer to the dediffcoder object.
 * \param[in,out] data Pointer to the QPSK data object.
 *
 * \return \c false if \p dediff and/or \p data are empty or \c true otherwise.
 */
LRPT_API bool lrpt_dsp_dediffcoder_exec(
        lrpt_dsp_dediffcoder_t *dediff,
        lrpt_qpsk_data_t *data);

/** Resynchronize and deinterleave a stream of QPSK symbols.
 *
 * Performs resynchronization and deinterleaving of QPSK symbols stream.
 *
 * \param[in,out] data Pointer to the QPSK data object.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull deinterleaving or \c false otherwise.
 */
LRPT_API bool lrpt_dsp_deinterleaver_exec(
        lrpt_qpsk_data_t *data,
        lrpt_error_t *err);

/** Initialize Integer FFT object.
 *
 * \param width Width of the FFT.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the Integer FFT object or \c NULL in case of error.
 */
LRPT_API lrpt_dsp_ifft_t *lrpt_dsp_ifft_init(
        uint16_t width,
        lrpt_error_t *err);

/** Free previously allocated Integer FFT object.
 *
 * \param ifft Pointer to the Integer FFT object.
 */
LRPT_API void lrpt_dsp_ifft_deinit(
        lrpt_dsp_ifft_t *ifft);

/** Compute complex->complex radix-2 forward FFT in 16-bit integer arithmetics.
 *
 * \p data length should be twice the Integer FFT object's width.
 *
 * \param ifft Pointer to the Integer FFT object.
 * \param data Pointer to the resulting FFT coefficients storage.
 */
LRPT_API void lrpt_dsp_ifft_exec(
        const lrpt_dsp_ifft_t *ifft,
        int16_t *data);

/** @} */

/** \addtogroup postproc
 * @{
 */

/** Flip image upside-down (rotate by 180 degrees).
 *
 * \param image Pointer to the LRPT image object.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful flipping or \c false otherwise.
 */
LRPT_API bool lrpt_postproc_image_flip(
        lrpt_image_t *image,
        lrpt_error_t *err);

/** Perform image rectification.
 *
 * Compensates tangential scale distortion and distortion caused by the curvature of the Earth.
 * If \p interpolate is \c true intermediate pixel values will be interpolated rather than
 * filled with the same value.
 *
 * \param image Pointer to the LRPT image object.
 * \param altitude Spacecraft altitude, in km.
 * \param interpolate Whether to perform pixel interpolation.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the rectified LRPT image object or \c NULL in case of error.
 *
 * \warning If rectification was successfull original image will be internally deallocated!
 */
LRPT_API lrpt_image_t *lrpt_postproc_image_rectify(
        lrpt_image_t *image,
        double altitude,
        bool interpolate,
        lrpt_error_t *err);

/** Perform histogram equalization on image.
 *
 * \param image Pointer to the LRPT image object.
 * \param clahe Whether to perform contrast limited adaptive histogram equalization.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful equalization or \c false otherwise.
 */
LRPT_API bool lrpt_postproc_image_normalize(
        lrpt_image_t *image,
        bool clahe,
        lrpt_error_t *err);

/** Rescale color range for specified APID.
 *
 * Performs rescaling of the original color range for specified \p apid. Resulting range will be
 * between \p pxval_min and \p pxval_max.
 *
 * \param image Pointer to the LRPT image object.
 * \param apid APID number for which rescaling will be applied.
 * \param pxval_min Lower edge of desired color range.
 * \param pxval_max Upper edge of desired color range.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful rescaling or \c false otherwise.
 */
LRPT_API bool lrpt_postproc_image_rescale_range(
        lrpt_image_t *image,
        uint8_t apid,
        uint8_t pxval_min,
        uint8_t pxval_max,
        lrpt_error_t *err);

/** Fix watery areas coloring.
 *
 * Histogram equalization tends to darken watery areas. Use this function to counteract this effect.
 * All blue pixels which value are less than \p pxval_min will be shifted and rescaled to fit into
 * specified range between \p pxval_min and \p pxval_max.
 *
 * \param image Pointer to the LRPT image object.
 * \param apid_blue APID number for blue channel.
 * \param pxval_min Lower edge of desired color range. Good starting value is 60.
 * \param pxval_max Upper edge of desired color range. Good starting value is 80.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful rescaling or \c false otherwise.
 */
LRPT_API bool lrpt_postproc_image_fix_water(
        lrpt_image_t *image,
        uint8_t apid_blue,
        uint8_t pxval_min,
        uint8_t pxval_max,
        lrpt_error_t *err);

/** Fix cloudy areas coloring.
 *
 * Red/infrared channel of AVHRR instrument may produce wrong (too much) luminance so coloring of
 * cloudy areas will be screwed. Use this function to counteract this effect. All pixel values of
 * blue channel which are greater than \p threshold will be considered as cloudy areas and all RGB
 * components will be equalized. Otherwise, red channel will be rescaled to fit into
 * specified range between \p red_min and \p red_max.
 *
 * \param image Pointer to the LRPT image object.
 * \param apid_red APID number for red channel.
 * \param apid_green APID number for green channel.
 * \param apid_blue APID number for blue channel.
 * \param red_min Lower edge of desired color range. Good starting value is 0.
 * \param red_max Upper edge of desired color range. Good starting value is 240.
 * \param threshold Threshold to determine where cloudy areas are. Good starting value is 210.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful rescaling or \c false otherwise.
 */
LRPT_API bool lrpt_postproc_image_fix_clouds(
        lrpt_image_t *image,
        uint8_t apid_red,
        uint8_t apid_green,
        uint8_t apid_blue,
        uint8_t red_min,
        uint8_t red_max,
        uint8_t threshold,
        lrpt_error_t *err);

/** Invert color palette for specified APID.
 *
 * When AVHRR operates in infrared mode usage of this function can enhance final image. It's advised
 * to invert all IR channels at once.
 *
 * \param image Pointer to the LRPT image object.
 * \param apid APID number which will be inverted.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful rescaling or \c false otherwise.
 */
LRPT_API bool lrpt_postproc_image_invert_channel(
        lrpt_image_t *image,
        uint8_t apid,
        lrpt_error_t *err);

/** @} */

/** \addtogroup demod
 * @{
 */

/** Allocate and initialize demodulator object.
 *
 * Creates demodulator object with specified parameters. User should properly free the object with
 * #lrpt_demodulator_deinit() after use.
 *
 * \param offset Whether offsetted version of QPSK modulation is used.
 * \param costas_bandwidth Initial Costas' PLL bandwidth in Hz.
 * \param interp_factor Interpolation factor. Should be greater than 0! Common value is 4.
 * \param demod_samplerate Demodulation sampling rate in samples/s.
 * \param symbol_rate PSK symbol rate in Sym/s.
 * \param rrc_order Costas' PLL root raised cosine filter order. Common value is 32.
 * \param rrc_alpha Costas' PLL root raised cosine filter alpha factor. Common value is 0.6.
 * \param pll_locked_threshold Costas' PLL locked threshold. Can't be zero.
 * \param pll_unlocked_threshold Costas' PLL unlocked threshold. Should be strictly greater than
 * \p pll_locked_threshold! Can't be zero.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the demodulator object or \c NULL in case of error.
 */
LRPT_API lrpt_demodulator_t *lrpt_demodulator_init(
        bool offset,
        double costas_bandwidth,
        uint8_t interp_factor,
        uint32_t demod_samplerate,
        uint32_t symbol_rate,
        uint16_t rrc_order,
        double rrc_alpha,
        double pll_locked_threshold,
        double pll_unlocked_threshold,
        lrpt_error_t *err);

/** Free previously allocated demodulator object.
 *
 * \param demod Pointer to the demodulator object.
 */
LRPT_API void lrpt_demodulator_deinit(
        lrpt_demodulator_t *demod);

/** Gain applied by demodulator (in dB).
 *
 * \param demod Pointer to the demodulator object.
 *
 * \return Current gain value (in dB) or \c 0 in case of \c NULL \p demod parameter.
 */
LRPT_API double lrpt_demodulator_gain(
        const lrpt_demodulator_t *demod);

/** Maximum possible AGC gain (in dB).
 *
 * \param demod Pointer to the demodulator object.
 *
 * \return Maximum possible AGC gain value (in dB) or \c 0 in case of \c NULL \p demod parameter.
 */
LRPT_API double lrpt_demodulator_maxgain(
        const lrpt_demodulator_t *demod);

/** Signal level.
 *
 * \param demod Pointer to the demodulator object.
 *
 * \return Current signal level value or \c 0 in case of \c NULL \p demod parameter.
 */
LRPT_API double lrpt_demodulator_siglvl(
        const lrpt_demodulator_t *demod);

/** PLL status.
 *
 * \param demod Pointer to the demodulator object.
 *
 * \return \c true if Costas loop is locked or \c false otherwise or in case of
 * \c NULL \p demod parameter.
 */
LRPT_API bool lrpt_demodulator_pllstate(
        const lrpt_demodulator_t *demod);

/** PLL frequency.
 *
 * \param demod Pointer to the demodulator object.
 *
 * \return Current Costas loop NCO frequency or \c 0 in case of \c NULL \p demod parameter.
 */
LRPT_API double lrpt_demodulator_pllfreq(
        const lrpt_demodulator_t *demod);

/** Costas PLL average phase error.
 *
 * \param demod Pointer to the demodulator object.
 *
 * \return Current PLL average phase error value or \c 0 in case of \c NULL \p demod parameter.
 */
LRPT_API double lrpt_demodulator_pllphaseerr(
        const lrpt_demodulator_t *demod);

/** Perform QPSK demodulation.
 *
 * Runs demodulation on given \p input I/Q samples. Input samples are filtered with Chebyshev
 * recursive filter and demodulated with \p demod demodulator object. Resulting QPSK
 * symbols will be stored in \p output QPSK data object.
 *
 * \param demod Pointer to the demodulator object.
 * \param input Pointer to the I/Q samples array to demodulate.
 * \param[out] output Demodulated QPSK symbols.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull demodulation or \c false in case of error.
 */
LRPT_API bool lrpt_demodulator_exec(
        lrpt_demodulator_t *demod,
        const lrpt_iq_data_t *input,
        lrpt_qpsk_data_t *output,
        lrpt_error_t *err);

/** @} */

/** \addtogroup decoder
 * @{
 */

/** Allocate and initialize decoder object.
 *
 * Creates decoder object. User should properly free the object with #lrpt_decoder_deinit()
 * after use.
 *
 * \param sc Spacecraft identifier.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the decoder object or \c NULL in case of error.
 */
LRPT_API lrpt_decoder_t *lrpt_decoder_init(
        lrpt_decoder_spacecraft_t sc,
        lrpt_error_t *err);

/** Free previously allocated decoder object.
 *
 * \param decoder Pointer to the decoder object.
 */
LRPT_API void lrpt_decoder_deinit(
        lrpt_decoder_t *decoder);

/** Perform LRPT decoding for given QPSK data.
 *
 * \param decoder Pointer to the decoder object.
 * \param data QPSK symbols to decode.
 * \param syms_proc Pointer to the number of processed QPSK symbols. If \c NULL, no info about
 * the number of processed symbols will be stored.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull decoding or \c false otherwise.
 */
LRPT_API bool lrpt_decoder_exec(
        lrpt_decoder_t *decoder,
        const lrpt_qpsk_data_t *data,
        size_t *syms_proc,
        lrpt_error_t *err);

/** Dump current image stored in decoder.
 *
 * Creates new LRPT image object which reflects current state of image stored in decoder object.
 *
 * \param decoder Pointer to the decoder object.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the LRPT image object or \c NULL in case of error.
 *
 * \warning User should manually free resulting object with #lrpt_image_free()!
 */
LRPT_API lrpt_image_t *lrpt_decoder_dump_image(
        lrpt_decoder_t *decoder,
        lrpt_error_t *err);

/** Framing status.
 *
 * \param decoder Pointer to the decoder object.
 *
 * \return \c true if framing is good or \c false otherwise or in case of
 * \c NULL \p decoder parameter.
 */
LRPT_API bool lrpt_decoder_framingstate(
        const lrpt_decoder_t *decoder);

/** Total number of frames processed.
 *
 * \param decoder Pointer to the decoder object.
 *
 * \return Number of frames have been processed or \c 0 in case of \c NULL \p decoder parameter.
 */
LRPT_API size_t lrpt_decoder_framestot_cnt(
        const lrpt_decoder_t *decoder);

/** Number of good frames processed.
 *
 * \param decoder Pointer to the decoder object.
 *
 * \return Number of successfully processed frames or \c 0 in case of \c NULL \p decoder parameter.
 */
LRPT_API size_t lrpt_decoder_framesok_cnt(
        const lrpt_decoder_t *decoder);

/** Number of CVCDUs processed.
 *
 * \param decoder Pointer to the decoder object.
 *
 * \return Number of CVCDUs have been processed or \c 0 in case of \c NULL \p decoder parameter.
 */
LRPT_API size_t lrpt_decoder_cvcdu_cnt(
        const lrpt_decoder_t *decoder);

/** Number of partial packets processed.
 *
 * \param decoder Pointer to the decoder object.
 *
 * \return Number of partial packets have been processed or \c 0 in case of
 * \c NULL \p decoder parameter.
 */
LRPT_API size_t lrpt_decoder_packets_cnt(
        const lrpt_decoder_t *decoder);

/** Signal quality.
 *
 * \param decoder Pointer to the decoder object.
 *
 * \return Signal quality in percents or \c 0 in case of \c NULL \p decoder parameter.
 */
LRPT_API uint8_t lrpt_decoder_sigqual(
        const lrpt_decoder_t *decoder);

/** Current number of pixels available.
 *
 * \param decoder Pointer to the decoder object.
 * \param count Array of length 6 to store numbers to.
 */
LRPT_API void lrpt_decoder_pxls_avail(
        const lrpt_decoder_t *decoder,
        size_t count[6]);

/** Get a slice of pixels.
 *
 * Reads requested number of pixels \p n starting with \p offset for specified \p apid and stores
 * them into provided array \p pxls which should be large enough to hold at least \p n
 * pixels. If requested number of pixels plus offset exceeds image size all remaining pixels
 * will be returned.
 *
 * \param decoder Pointer to the decoder object.
 * \param pxls Resulting storage for pixels data.
 * \param apid APID number.
 * \param offset Offset to read pixels from.
 * \param n Number of pixels to get.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful reading or \c false otherwise.
 */
LRPT_API bool lrpt_decoder_pxls_get(
        const lrpt_decoder_t *decoder,
        uint8_t *pxls,
        uint8_t apid,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Current decoder channel image width.
 *
 * \param decoder Pointer to the decoder object.
 *
 * \return Current channel image width or \c 0 if \c NULL \p decoder parameter.
 */
LRPT_API size_t lrpt_decoder_imgwidth(
        const lrpt_decoder_t *decoder);

/** Standard image width for spacecraft.
 *
 * \param sc Spacecraft identifier.
 *
 * \return Standard image width (in px) for specified spacecraft or \c 0 if spacecraft is not
 * supported.
 */
LRPT_API size_t lrpt_decoder_spacecraft_imgwidth(
        lrpt_decoder_spacecraft_t sc);

/** LRPT decoder soft frame length.
 *
 * \return Length of decoder's soft frame (in bits).
 */
LRPT_API size_t lrpt_decoder_sfl(void);

/** LRPT decoder hard frame length.
 *
 * \return Length of decoder's hard frame (in bytes).
 */
LRPT_API size_t lrpt_decoder_hfl(void);

/** @} */

/*************************************************************************************************/

/* Support for C++ codes */
#ifdef __cplusplus
}
#endif

/*************************************************************************************************/

#endif
