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

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************************************/

/** \addtogroup common Common
 *
 * Common utility routines.
 *
 * This API provides an interface for basic utility routines such as data allocation, freeing,
 * conversion and manipulation.
 *
 * @{
 */

/** Type for I/Q samples storage */
typedef struct lrpt_iq_data__ lrpt_iq_data_t;

/** Type for QPSK symbols storage */
typedef struct lrpt_qpsk_data__ lrpt_qpsk_data_t;

/** Type for LRPT image storage */
typedef struct lrpt_image__ lrpt_image_t;

/** Error levels */
typedef enum lrpt_error_level__ {
    LRPT_ERR_LVL_NONE = 0,
    LRPT_ERR_LVL_INFO,
    LRPT_ERR_LVL_WARN,
    LRPT_ERR_LVL_ERROR
} lrpt_error_level_t;

/** Supported error codes */
typedef enum lrpt_error_code__ {
    LRPT_ERR_CODE_NONE = 0,
    LRPT_ERR_CODE_ALLOC,
    LRPT_ERR_CODE_PARAM,
    LRPT_ERR_CODE_FOPEN,
    LRPT_ERR_CODE_FREAD,
    LRPT_ERR_CODE_FWRITE,
    LRPT_ERR_CODE_FSEEK,
    LRPT_ERR_CODE_EOF,
    LRPT_ERR_CODE_FILECORR,
    LRPT_ERR_CODE_UNSUPP,
    LRPT_ERR_CODE_DATAPROC
} lrpt_error_code_t;

/** Type for error reporting */
typedef struct lrpt_error__ lrpt_error_t;

/** @} */

/** \addtogroup io I/O
 *
 * Input/output routines.
 *
 * This API provides an interface for basic utility routines for I/O operations.
 *
 * @{
 */

/** Type for I/Q samples file */
typedef struct lrpt_iq_file__ lrpt_iq_file_t;

/** Supported I/Q samples file format versions */
typedef enum lrpt_iq_file_version__ {
    LRPT_IQ_FILE_VER1 = 0x01 /**< Version 1 */
} lrpt_iq_file_version_t;

/** Supported flags for Version 1 I/Q file */
typedef enum lrpt_iq_file_flags_ver1__ {
    LRPT_IQ_FILE_FLAGS_VER1_OFFSET = 0x01 /**< Offset modulation */
} lrpt_iq_file_flags_ver1_t;

/** Type for QPSK data file */
typedef struct lrpt_qpsk_file__ lrpt_qpsk_file_t;

/** Supported QPSK data file format versions */
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
 * This API provides an interface for DSP operations such as filtering, FFT, dediffcoding and
 * deinterleaving.
 *
 * @{
 */

/** DSP filter object type */
typedef struct lrpt_dsp_filter__ lrpt_dsp_filter_t;

/** Available DSP filter types */
typedef enum lrpt_dsp_filter_type__ {
    LRPT_DSP_FILTER_TYPE_LOWPASS,  /**< Lowpass filter */
    LRPT_DSP_FILTER_TYPE_HIGHPASS, /**< Highpass filter */
    LRPT_DSP_FILTER_TYPE_BANDPASS  /**< Bandpass filter */
} lrpt_dsp_filter_type_t;

/** Dediffcoder object type */
typedef struct lrpt_dsp_dediffcoder__ lrpt_dsp_dediffcoder_t;

/** @} */

/** \addtogroup demod Demodulator
 *
 * QPSK demodulation routines.
 *
 * This API provides an interface for performing QPSK demodulation.
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
 * This API provides an interface for performing LRPT data decoding.
 *
 * @{
 */

/** Decoder object type */
typedef struct lrpt_decoder__ lrpt_decoder_t;

/** Supported spacecrafts */
typedef enum lrpt_decoder_spacecraft__ {
    LRPT_DECODER_SC_METEORM2  /**< Meteor-M2 */
} lrpt_decoder_spacecraft_t;

/** @} */

/*************************************************************************************************/

/** \addtogroup common
 * @{
 */

/** Allocate I/Q data object.
 *
 * Tries to allocate storage for I/Q data of requested \p len. User should free the object with
 * #lrpt_iq_data_free() after use.
 *
 * \param len Length of new I/Q data object in number of I/Q samples. If zero length is
 * requested, empty object will be allocated but it will be possible to resize it later
 * with #lrpt_iq_data_resize().
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated I/Q data object or \c NULL if allocation has failed.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_alloc(
        size_t len,
        lrpt_error_t *err);

/** Free previously allocated I/Q data object.
 *
 * \param data Pointer to the I/Q data object.
 */
LRPT_API void lrpt_iq_data_free(
        lrpt_iq_data_t *data);

/** Length of I/Q data.
 *
 * \param data Pointer to the I/Q data object.
 *
 * \return Number of I/Q samples currently stored in \p data or \c 0 if \c NULL \p data was passed.
 */
LRPT_API size_t lrpt_iq_data_length(
        const lrpt_iq_data_t *data);

/** Resize existing I/Q data.
 *
 * If valid \p data is provided it will be resized to accomodate \p new_len I/Q pairs. If new
 * object was allocated during resize it will be initialized to 0.
 *
 * \param data Pointer to the I/Q data object.
 * \param new_len Length \p data will be resized to.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resize and \c false otherwise (original object will not be
 * modified in that case).
 */
LRPT_API bool lrpt_iq_data_resize(
        lrpt_iq_data_t *data,
        size_t new_len,
        lrpt_error_t *err);

/** Append I/Q data to existing data object.
 *
 * Adds \p n I/Q samples from I/Q data object \p add starting with position \p offset to the end
 * of \p data object. If \p n exceeds available number of samples in \p add (accounting for offset)
 * then all samples will be copied.
 *
 * \param data Pointer to the I/Q data object which will be enlarged.
 * \param add Pointer to the I/Q data object which contents will be added to the \p data.
 * \param offset How much samples should be skipped from the beginning of \p add.
 * \param n Number of samples to append.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull append and \c false otherwise (original object will not be
 * modified in that case).
 */
LRPT_API bool lrpt_iq_data_append(
        lrpt_iq_data_t *data,
        const lrpt_iq_data_t *add,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Copy part of I/Q samples to another I/Q data object.
 *
 * Copies \p n samples from I/Q data object \p samples starting with position \p offset to the
 * I/Q data object \p data which will be auto-resized to fit requested number of samples.
 * If \p n exceed available number of samples in \p samples (accounting for offset) then all
 * samples will be copied.
 *
 * \param data Pointer to the I/Q data object.
 * \param samples Pointer to the source I/Q data object.
 * \param offset How much samples should be skipped from the beginning of \p samples.
 * \param n Number of samples to copy.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull copying and \c false otherwise (original object will not be
 * modified in that case).
 *
 * \warning \p data and \p samples can't be the same object!
 */
LRPT_API bool lrpt_iq_data_from_iq(
        lrpt_iq_data_t *data,
        const lrpt_iq_data_t *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Create I/Q data object from a part of another I/Q data object.
 *
 * This function behaves much like #lrpt_iq_data_from_iq(), however, it allocates I/Q
 * data object automatically.
 *
 * \param samples Pointer to the source I/Q data object.
 * \param offset How much sampels should be skipped from the beginning of \p samples.
 * \param n Number of samples to copy.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated I/Q data object or \c NULL if allocation was unsuccessful or
 * \c NULL \p samples source I/Q data object was passed.
 *
 * \warning \p data and \p samples can't be the same object!
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_iq(
        const lrpt_iq_data_t *samples,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert array of complex I/Q samples to the library format.
 *
 * Converts \p n samples from array of complex I/Q samples \p samples to the library format of
 * I/Q data storage. Object given with \p data will be auto-resized to fit requested number of
 * samples.
 *
 * \param data Pointer to the I/Q data object.
 * \param samples Pointer to the array of complex I/Q samples.
 * \param n Number of samples to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull converting and \c false otherwise.
 *
 * \warning It's the user's responsibility to be sure that \p samples array contains at least
 * \p n elements!
 */
LRPT_API bool lrpt_iq_data_from_complex(
        lrpt_iq_data_t *data,
        const complex double *samples,
        size_t n,
        lrpt_error_t *err);

/** Create I/Q data object from complex I/Q samples.
 *
 * This function behaves much like #lrpt_iq_data_from_complex(), however, it allocates I/Q
 * data object automatically.
 *
 * \param samples Pointer to the array of complex I/Q samples.
 * \param n Number of samples to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated I/Q data object or \c NULL if allocation was unsuccessful or
 * \c NULL \p samples data array was passed.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_complex(
        const complex double *samples,
        size_t n,
        lrpt_error_t *err);

/** Convert I/Q data to complex samples.
 *
 * Saves \p n I/Q samples to the resulting \p samples array. Caller should be responsible that
 * \p samples is large enough to hold at least \p n elements!
 *
 * \param data Pointer to the I/Q data object.
 * \param samples Pointer to the resulting storage.
 * \param n Number of I/Q samples to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful conversion and \c false otherwise.
 *
 * \note If more samples than I/Q data object contains was requested then all samples will be
 * converted.
 */
LRPT_API bool lrpt_iq_data_to_complex(
        const lrpt_iq_data_t *data,
        complex double *samples,
        size_t n,
        lrpt_error_t *err);

/** Allocate QPSK data object.
 *
 * Tries to allocate storage for QPSK data of requested \p len symbols. User should free
 * the object with #lrpt_qpsk_data_free() after use.
 *
 * \param len Length of new QPSK data object in symbols. If zero length is requested, empty object
 * will be allocated but it will be possible to resize it later with #lrpt_qpsk_data_resize().
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK data object or \c NULL if allocation has failed.
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(
        size_t len,
        lrpt_error_t *err);

/** Free previously allocated QPSK data object.
 *
 * \param data Pointer to the QPSK data object.
 */
LRPT_API void lrpt_qpsk_data_free(
        lrpt_qpsk_data_t *data);

/** Length of QPSK data.
 *
 * \param data Pointer to the QPSK data object.
 *
 * \return Number of QPSK symbols currently stored in \p data or \c 0 if \c NULL \p data
 * was passed.
 */
LRPT_API size_t lrpt_qpsk_data_length(
        const lrpt_qpsk_data_t *data);

/** Resize existing QPSK data.
 *
 * If valid \p data is provided it will be resized to accomodate \p new_len QPSK symbols.
 *
 * \param data Pointer to the QPSK data object.
 * \param new_len New length \p data will be resized to.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resize and \c false otherwise (original object will not be
 * modified in that case).
 */
LRPT_API bool lrpt_qpsk_data_resize(
        lrpt_qpsk_data_t *data,
        size_t new_len,
        lrpt_error_t *err);

/** Append QPSK data to existing data object.
 *
 * Adds \p n QPSK symbols from QPSK data object \p add starting with position \p offset to the end
 * of \p data object. If \p n exceeds available number of symbols in \p add (accounting for offset)
 * then all symbols will be copied.
 *
 * \param data Pointer to the QPSK data object which will be enlarged.
 * \param add Pointer to the QPSK data object which contents will be added to the \p data.
 * \param offset How much symbols should be skipped from the beginning of \p add.
 * \param n Number of symbols to append.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull append and \c false otherwise (original object will not be
 * modified in that case).
 */
LRPT_API bool lrpt_qpsk_data_append(
        lrpt_qpsk_data_t *data,
        const lrpt_qpsk_data_t *add,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Copy part of QPSK symbols to another QPSK data object.
 *
 * Copies \p n symbols from QPSK data object \p symbols starting with position \p offset to the
 * QPSK data object \p data which will be auto-resized to fit requested number of symbols.
 * If \p n exceed available number of symbols in \p symbols (accounting for offset) then all
 * symbols will be copied.
 *
 * \param data Pointer to the QPSK data object.
 * \param symbols Pointer to the source QPSK data object.
 * \param offset How much symbols should be skipped from the beginning of \p symbols.
 * \param n Number of symbols to copy.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull copying and \c false otherwise (original object will not be
 * modified in that case).
 *
 * \warning \p data and \p symbols can't be the same object!
 */
LRPT_API bool lrpt_qpsk_data_from_qpsk(
        lrpt_qpsk_data_t *data,
        const lrpt_qpsk_data_t *symbols,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Create QPSK data object from a part of another QPSK data object.
 *
 * This function behaves much like #lrpt_qpsk_data_from_qpsk(), however, it allocates QPSK
 * data object automatically.
 *
 * \param samples Pointer to the source QPSK data object.
 * \param offset How much symbols should be skipped from the beginning of \p symbols.
 * \param n Number of symbols to copy.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK data object or \c NULL if allocation was unsuccessful or
 * \c NULL \p symbols source QPSK data object was passed.
 *
 * \warning \p data and \p symbols can't be the same object!
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_qpsk(
        const lrpt_qpsk_data_t *symbols,
        size_t offset,
        size_t n,
        lrpt_error_t *err);

/** Convert array of QPSK soft symbols to the library format.
 *
 * Converts \p n symbols from array of QPSK soft symbols \p symbols to the library format of
 * QPSK data storage. Object given with \p data will be auto-resized to fit requested number of
 * symbols.
 *
 * \param data Pointer to the QPSK data object.
 * \param symbols Pointer to the array of QPSK soft symbols.
 * \param n Number of symbols to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull converting and \c false otherwise.
 *
 * \warning It's the user's responsibility to be sure that \p symbols array contains at least
 * twice of \p n elements (1 QPSK symbol equals to 2 \c int8_t bytes)!
 */
LRPT_API bool lrpt_qpsk_data_from_soft(
        lrpt_qpsk_data_t *data,
        const int8_t *symbols,
        size_t n,
        lrpt_error_t *err);

/** Create QPSK data object from QPSK soft symbols.
 *
 * This function behaves much like #lrpt_qpsk_data_from_soft(), however, it allocates QPSK
 * data object automatically.
 *
 * \param symbols Pointer to the array of QPSK soft symbols.
 * \param n Number of symbols to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK data object or \c NULL if allocation was
 * unsuccessful or \c NULL \p symbols data array was passed.
  */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_soft(
        const int8_t *symbols,
        size_t n,
        lrpt_error_t *err);

/** Convert QPSK data to soft symbols.
 *
 * Saves \p n QPSK symbols to the resulting \p symbols array. Caller should be responsible that
 * \p symbols is large enough to hold at least twice of \p n elements (1 QPSK symbol equals to
 * 2 \c int8_t bytes)!
 *
 * \param data Pointer to the QPSK data object.
 * \param symbols Pointer to the resulting storage.
 * \param n Number of QPSK symbols to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful conversion and \c false otherwise.
 *
 * \note If more symbols than QPSK data object contains was requested then all symbols will be
 * converted.
 */
LRPT_API bool lrpt_qpsk_data_to_soft(
        const lrpt_qpsk_data_t *data,
        int8_t *symbols,
        size_t n,
        lrpt_error_t *err);

/** Convert array of QPSK hard symbols to the library format.
 *
 * Converts \p n symbols from array of QPSK hard symbols \p symbols to the library format of
 * QPSK data storage. Object given with \p data will be auto-resized to fit requested number of
 * symbols.
 *
 * \param data Pointer to the QPSK data object.
 * \param symbols Pointer to the array of QPSK hard symbols.
 * \param n Number of symbols to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull converting and \c false otherwise.
 *
 * \warning It's the user's responsibility to be sure that \p symbols array contain at least
 * one fourth of \p n elements (4 QPSK symbols equal to 1 \c unsigned \c char byte)!
 */
LRPT_API bool lrpt_qpsk_data_from_hard(
        lrpt_qpsk_data_t *data,
        const unsigned char *symbols,
        size_t n,
        lrpt_error_t *err);

/** Create QPSK data object from QPSK hard symbols.
 *
 * This function behaves much like #lrpt_qpsk_data_from_hard(), however, it allocates QPSK
 * data object automatically.
 *
 * \param symbols Pointer to the array of QPSK hard symbols.
 * \param n Number of symbols to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated QPSK data object or \c NULL if allocation was
 * unsuccessful or \c NULL \p symbols data array was passed.
  */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_hard(
        const unsigned char *symbols,
        size_t n,
        lrpt_error_t *err);

/** Convert QPSK data to hard symbols.
 *
 * Saves \p n QPSK symbols to the resulting \p symbols array. Caller should be responsible that
 * \p symbols is large enough to hold at least one fourth of \p n elements (4 QPSK symbols equal to
 * 1 \c unsigned \c char byte)!
 *
 * \param data Pointer to the QPSK data object.
 * \param symbols Pointer to the resulting storage.
 * \param n Number of QPSK symbols to convert.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successful conversion and \c false otherwise.
 *
 * \note If more symbols than QPSK data object contains was requested then all symbols will be
 * converted.
 */
LRPT_API bool lrpt_qpsk_data_to_hard(
        const lrpt_qpsk_data_t *data,
        unsigned char *symbols,
        size_t n,
        lrpt_error_t *err);

/** Allocate LRPT image object.
 *
 * If \p width or \p height are \c 0 then empty image will be allocated.
 *
 * \param width Width of the image (in px).
 * \param height Height of the image (in px).
 * \err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the allocated LRPT image object or \c NULL in case of error.
 */
LRPT_API lrpt_image_t *lrpt_image_alloc(
        size_t width,
        size_t height,
        lrpt_error_t *err);

/** Free allocated LRPT image object.
 *
 * \param image Pointer to the LRPT image object.
 */
LRPT_API void lrpt_image_free(
        lrpt_image_t *image);

/** Width of LRPT image (in px).
 *
 * \param image Pointer to the LRPT image object.
 *
 * \return Width of LRPT image (in px) or \c 0 if \c NULL \p image was passed.
 */
LRPT_API size_t lrpt_image_width(
        const lrpt_image_t *image);

/** Height of LRPT image (in px).
 *
 * \param image Pointer to the LRPT image object.
 *
 * \return Height of LRPT image (in px) or \c 0 if \c NULL \p image was passed.
 */
LRPT_API size_t lrpt_image_height(
        const lrpt_image_t *image);

/** Resize LRPT image object and set new width (in px).
 *
 * \param image Pointer to the LRPT image object.
 * \param new_width New image width (in px).
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resize and \c false otherwise (original object will not be
 * modified in that case).
 */
LRPT_API bool lrpt_image_set_width(
        lrpt_image_t *image,
        size_t new_width,
        lrpt_error_t *err);

/** Resize LRPT image object and set new height (in px).
 *
 * \param image Pointer to the LRPT image object.
 * \param new_height New image height (in px).
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull resize and \c false otherwise (original object will not be
 * modified in that case).
 */
LRPT_API bool lrpt_image_set_height(
        lrpt_image_t *image,
        size_t new_height,
        lrpt_error_t *err);

/** Get pixel value for specified position and APID channel.
 *
 * Gets pixel value for specified position and selected APID channel. APIDs that are outside of
 * 64-69 range are not supported.
 *
 * \param image Pointer to the LRPT image object.
 * \param apid APID number (must be within 64-69 range).
 * \param pos Position of pixel to get value of.
 *
 * \return Pixel value or \c 0 in case of error.
 */
LRPT_API uint8_t lrpt_image_get_px(
        const lrpt_image_t *image,
        uint8_t apid,
        size_t pos);

/** Set pixel value for specified position and APID channel.
 *
 * Sets pixel value for specified position and selected APID channel. APIDs that are outside of
 * 64-69 range are not supported.
 *
 * \param image Pointer to the LRPT image object.
 * \param apid APID number (must be within 64-69 range).
 * \param pos Position of pixel to set value of.
 * \param val Value of pixel.
 */
LRPT_API void lrpt_image_set_px(
        lrpt_image_t *image,
        uint8_t apid,
        size_t pos,
        uint8_t val);

/** Dump single channel as PGM file.
 *
 * Saves specified APID channel image to the PGM file format.
 *
 * \param image Pointer to the LRPT image object.
 * \param fname Name of file to save PGM image to.
 * \param apid Number of APID channel to save image to.
 * \param corr Whether to perform BT.709 gamma correction. If \c false then linear PGM file will
 * be saved.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull saving and \c false otherwise.
 */
LRPT_API bool lrpt_image_dump_pgm(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid,
        bool corr,
        lrpt_error_t *err);

/** Dump red, green and blue channels as PPM file.
 *
 * Saves specified APID channel images to the PPM file format.
 *
 * \param image Pointer to the LRPT image object.
 * \param fname Name of file to save PPM image to.
 * \param apid_red Number of APID channel to use as red channel.
 * \param apid_green Number of APID channel to use as green channel.
 * \param apid_blue Number of APID channel to use as blue channel.
 * \param corr Whether to perform BT.709 gamma correction. If \c false then linear PPM file will
 * be saved.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull saving and \c false otherwise.
 */
LRPT_API bool lrpt_image_dump_ppm(
        const lrpt_image_t *image,
        const char *fname,
        uint8_t apid_red,
        uint8_t apid_green,
        uint8_t apid_blue,
        bool corr,
        lrpt_error_t *err);

/** Initialize error object.
 *
 * Creates error object with default level and code. Internal string buffer for error message
 * is set to \c NULL.
 *
 * \return Pointer to the allocated error object or \c NULL in case of error.
 */
LRPT_API lrpt_error_t *lrpt_error_init(void);

/** Free initialized error object.
 *
 * \param err Pointer to the error object.
 */
LRPT_API void lrpt_error_deinit(
        lrpt_error_t *err);

/** Reset resources claimed for error object.
 *
 * Resets error level and code and frees internal string buffer (it will be set to \c NULL).
 *
 * \param err Pointer to the error object.
 */
LRPT_API void lrpt_error_cleanup(
        lrpt_error_t *err);

/** Error level.
 *
 * \param err Pointer to the error object.
 *
 * \return Error category (see #lrpt_error_level_t) or \c 0 if \c NULL \p err was passed.
 */
LRPT_API lrpt_error_level_t lrpt_error_level(
        const lrpt_error_t *err);

/** Error code.
 *
 * \param err Pointer to the error object.
 *
 * \return Numerical error code (see #lrpt_error_code_t) or \c 0 if \c NULL \p err was passed.
 */
LRPT_API lrpt_error_code_t lrpt_error_code(
        const lrpt_error_t *err);

/** Error message string.
 *
 * \param err Pointer to the error object.
 *
 * \return Character error message string or \c NULL if \c NULL \p err was passed or message is
 * empty.
 *
 * \warning User should never free returned string!
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

/** Open I/Q data file of Version 1 for writing.
 *
 * File format is described at \ref lrptiq section. User should close file properly with
 * #lrpt_iq_file_close() after use.
 *
 * \param fname Name of file to write I/Q data to.
 * \param offset Whether offset QPSK was used or not.
 * \param samplerate Sampling rate.
 * \param device_name Device name string. If \p device_name is \c NULL no device name info will
 * be written.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the writable I/Q file object or \c NULL in case of error.
 */
LRPT_API lrpt_iq_file_t *lrpt_iq_file_open_w_v1(
        const char *fname,
        bool offset,
        uint32_t samplerate,
        const char *device_name,
        lrpt_error_t *err);

/** Close previously opened file with I/Q data.
 *
 * \param file Pointer to the I/Q data file object.
 */
LRPT_API void lrpt_iq_file_close(
        lrpt_iq_file_t *file);

/** I/Q data file format version info.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return File version number info or \c 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint8_t lrpt_iq_file_version(
        const lrpt_iq_file_t *file);

/** Offset QPSK modulation presence.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return \c true if offset QPSK was used and \c false otherwise. \c false will be returned if
 * \c NULL \p file was passed.
 */
LRPT_API bool lrpt_iq_file_is_offsetted(
        const lrpt_iq_file_t *file);

/** I/Q data file sampling rate.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Sampling rate at which file was created or \c 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint32_t lrpt_iq_file_samplerate(
        const lrpt_iq_file_t *file);

/** Device name used to write file.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Pointer to the device name string or \c NULL in case of \c NULL \p file parameter.
 *
 * \warning User should never free returned string!
 */
LRPT_API const char *lrpt_iq_file_devicename(
        const lrpt_iq_file_t *file);

/** Number of I/Q samples in file.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Number of I/Q samples stored in file or \c 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint64_t lrpt_iq_file_length(
        const lrpt_iq_file_t *file);

/** Set current position in I/Q data file stream.
 *
 * \param file Pointer to the I/Q data file object.
 * \param sample Sample index to set file pointer to. Index enumeration starts with 0.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull positioning and \c false otherwise (if \p file is \c NULL,
 * position setting was unsuccessful).
 *
 * \note If repositioning failed internal I/Q sample pointer will not be changed.
 *
 * \note If seekable sample index exceeds file length file pointer will be set to the end.
 *
 * \note Seeking is prohibited for files opened with write mode.
 */
LRPT_API bool lrpt_iq_file_goto(
        lrpt_iq_file_t *file,
        uint64_t sample,
        lrpt_error_t *err);

/** Read I/Q data from file.
 *
 * Reads \p len consecutive I/Q samples into I/Q data object \p data from file \p file.
 * Object will be auto-resized to proper length. If requested \p len exceeds number of samples
 * remaining all samples up to the end of file will be read.
 *
 * \param[out] data Pointer to the I/Q data object.
 * \param file Pointer to the I/Q data file object.
 * \param len Number of I/Q samples to read.
 * \param rewind If true, sample position in file stream will be preserved after reading.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull reading and \c false otherwise.
 *
 * \note File with I/Q data is expected to be compatible with internal library format, e. g.
 * written with #lrpt_iq_data_write_to_file(). For more details see \ref lrptiq section.
 */
LRPT_API bool lrpt_iq_data_read_from_file(
        lrpt_iq_data_t *data,
        lrpt_iq_file_t *file,
        size_t len,
        bool rewind,
        lrpt_error_t *err);

/** Write I/Q data to file.
 *
 * \param data Pointer to the I/Q data object.
 * \param file Pointer to the I/Q file object to write I/Q data to.
 * \param inplace Determines whether data length should be dumped as soon as possible (after every
 * chunk, slower, more robust) or at the end of writing (faster).
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull writing and \c false otherwise.
 *
 * \note Resulting file maintains internal library format. For more details see
 * \ref lrptiq section.
 */
LRPT_API bool lrpt_iq_data_write_to_file(
        const lrpt_iq_data_t *data,
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

/** Open QPSK data file of Version 1 for writing.
 *
 * File format is described at \ref lrptqpsk section. User should close file properly with
 * #lrpt_qpsk_file_close() after use.
 *
 * \param fname Name of file to write QPSK data to.
 * \param differential Whether differential coding was used or not.
 * \param interleaved Whether interleaving was used or not.
 * \param hard Determines whether data is in hard symbols format (if \p hard is \c true) or soft
 * (\p hard is \c false).
 * \param symrate Symbol rate.
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

/** Close previously opened file with QPSK data.
 *
 * \param file Pointer to the QPSK data file object.
 */
LRPT_API void lrpt_qpsk_file_close(
        lrpt_qpsk_file_t *file);

/** QPSK data file format version info.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return File version number info or \c 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint8_t lrpt_qpsk_file_version(
        const lrpt_qpsk_file_t *file);

/** Differential coding modulation presence.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return \c true if differential coding was used and \c false otherwise. \c false will be
 * returned if \c NULL \p file was passed.
 */
LRPT_API bool lrpt_qpsk_file_is_diffcoded(
        const lrpt_qpsk_file_t *file);

/** Interleaving presence.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return \c true if interleaving was used and \c false otherwise. \c false will be returned if
 * \c NULL \p file was passed.
 */
LRPT_API bool lrpt_qpsk_file_is_interleaved(
        const lrpt_qpsk_file_t *file);

/** Check whether symbols are in hard format.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return \c true if symbols are in hard format and \c false in case of soft format. \c false
 * will be returned if \c NULL \p file was passed.
 */
LRPT_API bool lrpt_qpsk_file_is_hardsymboled(
        const lrpt_qpsk_file_t *file);

/** QPSK data file symbol rate.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return Symbol rate at which file was created or \c 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint32_t lrpt_qpsk_file_symrate(
        const lrpt_qpsk_file_t *file);

/** Number of QPSK symbols stored in file.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return Number of QPSK symbols stored in file or \c 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint64_t lrpt_qpsk_file_length(
        const lrpt_qpsk_file_t *file);

/** Set current position in QPSK data file stream.
 *
 * \param file Pointer to the QPSK data file object.
 * \param symbol Symbol index to set file pointer to. Index enumeration starts with 0.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull positioning and \c false otherwise (if \p file is \c NULL,
 * position setting was unsuccessful.
 *
 * \note If repositioning failed internal QPSK symbol pointer will not be changed.
 *
 * \note If seekable symbol index exceeds file length file pointer will be set to the end.
 *
 * \note Seeking is prohibited for files opened with write mode.
 */
LRPT_API bool lrpt_qpsk_file_goto(
        lrpt_qpsk_file_t *file,
        uint64_t symbol,
        lrpt_error_t *err);

/** Read QPSK data from file.
 *
 * Reads \p len consecutive QPSK symbols into QPSK data object \p data from file \p file.
 * Object will be auto-resized to proper length.
 *
 * \param[out] data Pointer to the QPSK data object.
 * \param file Pointer to the QPSK data file object.
 * \param len Number of QPSK symbols to read.
 * \param rewind If true, symbol position in file stream will be preserved after reading.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull reading and \c false otherwise.
 *
 * \note File with QPSK data is expected to be compatible with internal library format, e. g.
 * written with #lrpt_qpsk_data_write_to_file(). For more details see \ref lrptqpsk section.
 */
LRPT_API bool lrpt_qpsk_data_read_from_file(
        lrpt_qpsk_data_t *data,
        lrpt_qpsk_file_t *file,
        size_t len,
        bool rewind,
        lrpt_error_t *err);

/** Write QPSK data to file.
 *
 * Writes QPSK data pointed by \p data to file \p file.
 *
 * \param data Pointer to the QPSK data object.
 * \param file Pointer to the QPSK file object to write QPSK data to.
 * \param inplace Determines whether data length should be dumped as soon as possible (after every
 * chunk, slower) or at the end of writing (faster).
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull writing and \c false otherwise.
 *
 * \note Resulting file maintains internal library format. For more details see \ref lrptqpsk
 * section.
 */
LRPT_API bool lrpt_qpsk_data_write_to_file(
        const lrpt_qpsk_data_t *data,
        lrpt_qpsk_file_t *file,
        bool inplace,
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
 * \param num_poles Number of filter poles. Must be even and not greater than 252!
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
 * \return \c false if \p filter and/or \p data are empty and \c true otherwise.
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
 * \return \c false if \p dediff and/or \p data are empty and \c true otherwise.
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
 * \return \c true on successfull deinterleaving and \c false otherwise.
 */
LRPT_API bool lrpt_dsp_deinterleaver_exec(
        lrpt_qpsk_data_t *data,
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
 * \param interp_factor Interpolation factor. Usual value is 4.
 * \param demod_samplerate Demodulation sampling rate in samples/s.
 * \param symbol_rate PSK symbol rate in Sym/s.
 * \param rrc_order Costas' PLL root raised cosine filter order.
 * \param rrc_alpha Costas' PLL root raised cosine filter alpha factor.
 * \param pll_locked_threshold Costas' PLL locked threshold.
 * \param pll_unlocked_threshold Costas' PLL unlocked threshold. Should be strictly greater than
 * \p pll_locked_threshold!.
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

/** Costas PLL average phase error.
 *
 * \param demod Pointer to the demodulator object.
 *
 * \return Current PLL average phase error value or \c 0 in case of \c NULL \p demod parameter.
 */
LRPT_API double lrpt_demodulator_phaseerr(
        const lrpt_demodulator_t *demod);

/** Perform QPSK demodulation.
 *
 * Runs demodulation on given \p input I/Q samples. Input samples are filtered with Chebyshev
 * recursive filter and then demodulated with \p demod demodulator object. Resulting QPSK
 * symbols will be stored in \p output QPSK data object.
 *
 * \param demod Pointer to the demodulator object.
 * \param input Pointer to the I/Q samples array to demodulate.
 * \param[out] output Demodulated QPSK symbols.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull demodulation and \c false in case of error.
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
 * \return \c true on successfull decoding and \c false otherwise.
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

#endif
