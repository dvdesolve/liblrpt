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

/** Type for QPSK data storage */
typedef struct lrpt_qpsk_data__ lrpt_qpsk_data_t;

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
    LRPT_ERR_CODE_EOF,
    LRPT_ERR_CODE_DATACORR,
    LRPT_ERR_CODE_UNSUPP,
    LRPT_ERR_CODE_NAN,
    LRPT_ERR_CODE_INF,
    LRPT_ERR_CODE_MATH
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
    LRPT_IQ_FILE_VER_1 = 0x01 /**< Version 1 */
} lrpt_iq_file_version_t;

/** Type for QPSK data file */
typedef struct lrpt_qpsk_file__ lrpt_qpsk_file_t;

/** Supported QPSK data file format versions */
typedef enum lrpt_qpsk_file_version__ {
    LRPT_QPSK_FILE_VER_1 = 0x01 /**< Version 1 */
} lrpt_qpsk_file_version_t;

/** @} */

/** \addtogroup dsp DSP
 *
 * Digital signal processing routines.
 *
 * This API provides an interface for DSP operations such as filtering and FFT.
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

/** Dediffcoder object type */
typedef struct lrpt_dediffcoder__ lrpt_dediffcoder_t;

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

/** Length of soft frame in bits (produced by convolutional encoder, r = 1/2) */
LRPT_API extern const size_t LRPT_DECODER_SOFT_FRAME_LEN;

/** Length of hard frame in bytes (produced after Viterbi decoding) */
LRPT_API extern const size_t LRPT_DECODER_HARD_FRAME_LEN;

/** Supported spacecrafts */
typedef enum lrpt_decoder_spacecraft__ {
    LRPT_DECODER_SC_METEORM2  /**< Meteor-M2 */
} lrpt_decoder_spacecraft_t;

/** @} */

/*************************************************************************************************/

/** \addtogroup common
 * @{
 */

/** Allocate I/Q data storage object.
 *
 * Tries to allocate storage for I/Q data of requested \p len. User should free the object with
 * #lrpt_iq_data_free() after use.
 *
 * \param len Length of new I/Q data storage. If zero length is requested, empty storage
 * will be allocated but it will be possible to resize it later with #lrpt_iq_data_resize().
 *
 * \return Pointer to the allocated I/Q data storage object or \c NULL if allocation has failed.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_alloc(
        size_t len);

/** Free previously allocated I/Q data storage.
 *
 * \param data Pointer to the I/Q data storage object.
 */
LRPT_API void lrpt_iq_data_free(
        lrpt_iq_data_t *data);

/** Length of I/Q data storage.
 *
 * \param data Pointer to the I/Q data storage object.
 *
 * \return Number of I/Q samples currently stored in \p data.
 */
LRPT_API size_t lrpt_iq_data_length(
        const lrpt_iq_data_t *data);

/** Resize existing I/Q data storage.
 *
 * If valid \p data is provided it will be resized to accomodate \p new_len I/Q pairs. If new
 * storage was allocated during resize it will be initialized to 0.
 *
 * \param data Pointer to the I/Q data storage object.
 * \param new_len Length \p data will be resized to.
 *
 * \return \c true on successfull resize and \c false otherwise (original storage will not be
 * modified in that case).
 */
LRPT_API bool lrpt_iq_data_resize(
        lrpt_iq_data_t *data,
        size_t new_len);

/** Convert array of I/Q samples into library format.
 *
 * Converts array of I/Q samples \p iq of size \p len into library format of I/Q
 * data storage. Storage given with \p data will be auto-resized to fit requested data length.
 *
 * \param data Pointer to the I/Q data storage object.
 * \param iq Pointer to the array of I/Q samples.
 * \param len Number of samples to merge into I/Q data.
 *
 * \return \c true on successfull converting and \c false otherwise.
 *
 * \warning It's the user's responsibility to be sure that \p iq array contain at least
 * \p len samples!
 */
LRPT_API bool lrpt_iq_data_from_samples(
        lrpt_iq_data_t *data,
        const complex double *iq,
        size_t len);

/** Create I/Q storage object from I/Q samples.
 *
 * This function behaves much like #lrpt_iq_data_from_samples(), however, it allocates
 * I/Q storage automatically.
 *
 * \param iq Pointer to the array of I/Q samples.
 * \param len Number of samples in I and Q arrays to repack into I/Q data.
 *
 * \return Pointer to the allocated I/Q data storage object or \c NULL if allocation was
 * unsuccessful.
 *
 * \warning Because code logic is the same as with #lrpt_iq_data_from_samples(), the same
 * caution about \p iq array length is actual.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_samples(
        const complex double *iq,
        size_t len);

/** Merge separate arrays of double-typed I/Q samples into library format.
 *
 * Merges arrays of I and Q samples (\p i and \p q) of size \p len each into library format
 * of I/Q data storage. Storage given with \p data will be auto-resized to fit
 * requested data length.
 *
 * \param data Pointer to the I/Q data storage object.
 * \param i Pointer to the array of I samples.
 * \param q Pointer to the array of Q samples.
 * \param len Number of samples in I and Q arrays to merge into I/Q data.
 *
 * \return \c true on successfull merging and \c false otherwise.
 *
 * \warning It's the user's responsibility to be sure that \p i and \p q arrays contain at least
 * \p len samples!
 */
LRPT_API bool lrpt_iq_data_from_doubles(
        lrpt_iq_data_t *data,
        const double *i,
        const double *q,
        size_t len);

/** Create I/Q storage object from double-typed I/Q samples.
 *
 * This function behaves much like #lrpt_iq_data_from_doubles(), however, it allocates
 * I/Q storage automatically.
 *
 * \param i Pointer to the array of I samples.
 * \param q Pointer to the array of Q samples.
 * \param len Number of samples in I and Q arrays to repack into I/Q data.
 *
 * \return Pointer to the allocated I/Q data storage object or \c NULL if allocation was
 * unsuccessful.
 *
 * \warning Because code logic is the same as with #lrpt_iq_data_from_doubles(), the same
 * caution about \p i and \p q array lengths is actual.
 */
LRPT_API lrpt_iq_data_t *lrpt_iq_data_create_from_doubles(
        const double *i,
        const double *q,
        size_t len);

/** Allocate QPSK data storage object.
 *
 * Tries to allocate storage for QPSK data of requested \p len. User should free the object with
 * #lrpt_qpsk_data_free() after use.
 *
 * \param len Length of new QPSK data storage. If zero length is requested,
 * empty storage will be allocated but it's still possible to resize it later with
 * #lrpt_qpsk_data_resize().
 *
 * \return Pointer to the allocated QPSK data storage object or \c NULL if allocation has failed.
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(
        size_t len);

/** Free previously allocated QPSK data storage.
 *
 * \param data Pointer to the QPSK data storage object.
 */
LRPT_API void lrpt_qpsk_data_free(
        lrpt_qpsk_data_t *data);

/** Length of QPSK data storage.
 *
 * \param data Pointer to the QPSK data storage object.
 *
 * \return Number of QPSK bytes currently stored in \p data. Actual number of soft symbols is half
 * as much as the number of QPSK bytes.
 */
LRPT_API size_t lrpt_qpsk_data_length(
        const lrpt_qpsk_data_t *data);

/** Resize existing QPSK soft symbol data storage.
 *
 * If valid \p data is provided it will be resized to accomodate \p new_len QPSK soft symbols.
 *
 * \param data Pointer to the QPSK soft symbol data storage object.
 * \param new_len New length \p data will be resized to.
 *
 * \return \c true on successfull resize and \c false otherwise (original storage will not be
 * modified in that case).
 */
LRPT_API bool lrpt_qpsk_data_resize(
        lrpt_qpsk_data_t *data,
        size_t new_len);

/** Convert array of QPSK symbols into library format.
 *
 * Converts array of QPSK symbols \p qpsk of size \p len into library format of QPSK
 * data storage. Storage given with \p data will be auto-resized to fit requested data length.
 *
 * \param data Pointer to the QPSK data storage object.
 * \param qpsk Pointer to the array of QPSK symbols.
 * \param len Number of symbols to merge into QPSK data.
 *
 * \return \c true on successfull converting and \c false otherwise.
 *
 * \warning It's the user's responsibility to be sure that \p qpsk array contain at least
 * \p len symbols!
 */
LRPT_API bool lrpt_qpsk_data_from_symbols(
        lrpt_qpsk_data_t *data,
        const int8_t *qpsk,
        size_t len);

/** Create QPSK storage object from QPSK symbols.
 *
 * This function behaves much like #lrpt_qpsk_data_from_symbols(), however, it allocates
 * QPSK storage automatically.
 *
 * \param qpsk Pointer to the array of QPSK symbols.
 * \param len Number of QPSK symbols to repack into QPSK data.
 *
 * \return Pointer to the allocated QPSK data storage object or \c NULL if allocation was
 * unsuccessful.
 *
 * \warning Because code logic is the same as with #lrpt_qpsk_data_from_symbols(), the same
 * caution about \p qpsk array length is actual.
 */
LRPT_API lrpt_qpsk_data_t *lrpt_qpsk_data_create_from_symbols(
        const int8_t *qpsk,
        size_t len);

/** Return QPSK data as an int8_t bytes.
 *
 * Copies \p len QPSK symbols to the resulting \p qpsk array. Caller should be responsible
 * that \p qpsk is large enough to hold at least \p len elements!
 *
 * \param data Pointer to the QPSK data storage object.
 * \param qpsk Pointer to the resulting storage.
 * \param len Number of QPSK symbols to copy.
 *
 * \return \c true on successful copy and \c false otherwise.
 */
LRPT_API bool lrpt_qpsk_data_to_ints(
        const lrpt_qpsk_data_t *data,
        int8_t *qpsk,
        size_t len);

/** Free resources claimed for error object.
 *
 * \param err Pointer to the error object.
 */
LRPT_API void lrpt_error_cleanup(
        lrpt_error_t *err);

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
 * \param samplerate Sampling rate.
 * \param device_name Device name string. If \p device_name is \c NULL no device name info will
 * be written.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the writable I/Q file object or \c NULL in case of error.
 */
LRPT_API lrpt_iq_file_t *lrpt_iq_file_open_w_v1(
        const char *fname,
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
 * \return File version number info or 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint8_t lrpt_iq_file_version(
        const lrpt_iq_file_t *file);

/** I/Q data file sampling rate.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Sampling rate at which file was created or 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint32_t lrpt_iq_file_samplerate(
        const lrpt_iq_file_t *file);

/** Device name used to write file.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Pointer to the device name string or \c NULL in case of \c NULL \p file parameter.
 */
LRPT_API const char *lrpt_iq_file_devicename(
        const lrpt_iq_file_t *file);

/** Number of I/Q samples in file.
 *
 * \param file Pointer to the I/Q data file object.
 *
 * \return Number of I/Q samples stored in file or 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint64_t lrpt_iq_file_length(
        const lrpt_iq_file_t *file);

/** Set current position in I/Q data file stream.
 *
 * \param file Pointer to the I/Q data file object.
 * \param sample Sample index to set file pointer to. Index enumeration starts with 0.
 *
 * \return \c true on successfull positioning and \c false otherwise (if \p file is \c NULL,
 * position setting was unsuccessful or \p sample exceeds current number of samples).
 *
 * \note If repositioning failed internal I/Q sample pointer will not be changed.
 */
LRPT_API bool lrpt_iq_file_goto(
        lrpt_iq_file_t *file,
        uint64_t sample);

/** Read I/Q data from file.
 *
 * Reads \p len consecutive I/Q samples into I/Q storage \p data from file \p file.
 * Storage will be auto-resized to proper length. If requested \p len exceeds number of samples
 * remaining all samples up to the end of file will be read.
 *
 * \param[out] data Pointer to the I/Q data storage object.
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
 * \param data Pointer to the I/Q data storage object.
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
 * \param offset Whether offset QPSK was used or not.
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
        bool offset,
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
 * \return File version number info or 0 in case of \c NULL \p file parameter..
 */
LRPT_API uint8_t lrpt_qpsk_file_version(
        const lrpt_qpsk_file_t *file);

/** Offset QPSK modulation presence.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return \c true if offset QPSK was used and \c false otherwise. \c false will be returned if
 * \c NULL \p file was passed.
 */
LRPT_API bool lrpt_qpsk_file_is_offsetted(
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
 * \return Symbol rate at which file was created or 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint32_t lrpt_qpsk_file_symrate(
        const lrpt_qpsk_file_t *file);

/** Number of QPSK symbols stored in file.
 *
 * \param file Pointer to the QPSK data file object.
 *
 * \return Number of QPSK symbols stored in file or 0 in case of \c NULL \p file parameter.
 */
LRPT_API uint64_t lrpt_qpsk_file_length(
        const lrpt_qpsk_file_t *file);

/** Set current position in QPSK data file stream.
 *
 * \param file Pointer to the QPSK data file object.
 * \param symbol Symbol index to set file pointer to. Index enumeration starts with 0.
 *
 * \return \c true on successfull positioning and \c false otherwise (if \p file is \c NULL,
 * position setting was unsuccessful or \p symbol exceeds current number of symbols).
 *
 * \note If repositioning failed internal QPSK symbol pointer will not be changed.
 */
LRPT_API bool lrpt_qpsk_file_goto(
        lrpt_qpsk_file_t *file,
        uint64_t symbol);

/** Read QPSK data from file.
 *
 * Reads \p len consecutive QPSK symbols into QPSK storage \p data from file \p file.
 * Storage will be auto-resized to proper length.
 *
 * \param[out] data Pointer to the QPSK data storage object.
 * \param file Pointer to the QPSK data file object.
 * \param len Number of QPSK samples to read.
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
 * \param data Pointer to the QPSK data storage object.
 * \param file Pointer to the QPSK file object to write QPSK data to.
 * \param inplace Determines whether data length should be dumped as soon as possible (after every
 * chunk, slower) or at the end of writing (faster).
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return \c true on successfull writing and \c false otherwise.
 *
 * \note Resulting file maintains internal library format. For more details see
 * \ref lrptqpsk section.
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

/** Free initialized Chebyshev filter.
 *
 * \param filter Pointer to the Chebyshev filter object.
 */
LRPT_API void lrpt_dsp_filter_deinit(
        lrpt_dsp_filter_t *filter);

/** Apply recursive Chebyshev filter to the I/Q data.
 *
 * \param filter Pointer to the Chebyshev filter object.
 * \param data Pointer to the I/Q data object.
 *
 * \return \c false if \p filter is empty and \c true otherwise.
 */
LRPT_API bool lrpt_dsp_filter_apply(
        lrpt_dsp_filter_t *filter,
        lrpt_iq_data_t *data);

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
        double pll_unlocked_threshold);

/** Free previously allocated demodulator object.
 *
 * \param demod Pointer to the demodulator object.
 */
LRPT_API void lrpt_demodulator_deinit(
        lrpt_demodulator_t *demod);

/** Current gain applied by demodulator.
 *
 * Returns current gain applied by demodulator's AGC.
 *
 * \param demod Pointer to the demodulator object.
 * \param gain Pointer to the resulting value.
 *
 * \return \c true if valid demodulator object was given and \c false otherwise.
 */
LRPT_API bool lrpt_demodulator_gain(
        const lrpt_demodulator_t *demod,
        double *gain);

/** Current signal level.
 *
 * \param demod Pointer to the demodulator object.
 * \param level Pointer to the resulting value.
 *
 * \return \c true if valid demodulator object was given and \c false otherwise.
 */
LRPT_API bool lrpt_demodulator_siglvl(
        const lrpt_demodulator_t *demod,
        double *level);

/** Current Costas PLL average phase error.
 *
 * \param demod Pointer to the demodulator object.
 * \param error Pointer to the resulting value.
 *
 * \return \c true if valid demodulator object was given and \c false otherwise.
 */
LRPT_API bool lrpt_demodulator_pllavg(
        const lrpt_demodulator_t *demod,
        double *error);

/** Perform QPSK demodulation.
 *
 * Runs demodulation on given \p input I/Q samples. Input samples are filtered with Chebyshev
 * recursive filter and then demodulated with \p demod demodulator object. Resulting QPSK soft
 * symbols will be stored in \p output QPSK data storage.
 *
 * \param demod Pointer to the demodulator object.
 * \param input Pointer to the I/Q samples array to demodulate.
 * \param[out] output Demodulated QPSK soft symbols.
 *
 * \return \c true on successfull demodulation and \c false in case of error.
 */
LRPT_API bool lrpt_demodulator_exec(
        lrpt_demodulator_t *demod,
        const lrpt_iq_data_t *input,
        lrpt_qpsk_data_t *output);

/** Allocate and initialize dediffcoder object.
 *
 * Allocates and initializes dediffcoder object for use with QPSK diffcoded data.
 *
 * \return Pointer to the dediffcoder object or \c NULL in case of error.
 */
LRPT_API lrpt_dediffcoder_t *lrpt_dediffcoder_init(void);

/** Free previously allocated dediffcoder object.
 *
 * \param dediff Pointer to the dediffcoder object.
 */
LRPT_API void lrpt_dediffcoder_deinit(
        lrpt_dediffcoder_t *dediff);

/** Perform dediffcoding of QPSK data.
 *
 * Performs dediffcoding of given QPSK data. Data length should be at least 2 QPSK symbols long and
 * be an even.
 *
 * \param dediff Pointer to the dediffcoder object.
 * \param[in,out] data Pointer to the diffcoded QPSK data.
 *
 * \return \c true on successfull dediffcoding and \c false otherwise.
 */
LRPT_API bool lrpt_dediffcoder_exec(
        lrpt_dediffcoder_t *dediff,
        lrpt_qpsk_data_t *data);

/** Resynchronize and deinterleave a stream of QPSK soft symbols.
 *
 * Performs resynchronization and deinterleaving of QPSK symbols stream.
 *
 * \param[in,out] data Pointer to the QPSK data storage.
 *
 * \return \c true on successfull deinterleaving and \c false otherwise.
 */
LRPT_API bool lrpt_deinterleaver_exec(
        lrpt_qpsk_data_t *data);

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
 *
 * \return Pointer to the decoder object or \c NULL in case of error.
 */
LRPT_API lrpt_decoder_t *lrpt_decoder_init(
        lrpt_decoder_spacecraft_t sc);

/** Free previously allocated decoder object.
 *
 * \param decoder Pointer to the decoder object.
 */
LRPT_API void lrpt_decoder_deinit(
        lrpt_decoder_t *decoder);

/** Perform LRPT decoding for given data block.
 *
 * Performs full decoding for given soft-symbols data chunk.
 *
 * \param decoder Pointer to the decoder object.
 * \param input Input soft-symbols data.
 * \param buf_len Length of given data chunk.
 */
LRPT_API void lrpt_decoder_exec(
        lrpt_decoder_t *decoder,
        lrpt_qpsk_data_t *input,
        size_t buf_len);

/** @} */

/*************************************************************************************************/

#endif
