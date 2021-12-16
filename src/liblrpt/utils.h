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

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Public internal API for helper library utilities.
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_UTILS_H
#define LRPT_LIBLRPT_UTILS_H

/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

/** Size of serialized double (in number of unsigned chars) */
extern const uint8_t UTILS_DOUBLE_SER_SIZE;

/** Size of serialized complex (in number of unsigned chars) */
extern const uint8_t UTILS_COMPLEX_SER_SIZE;

/*************************************************************************************************/

/** Serialize \c uint16_t value.
 *
 * \param x Value to be serialized.
 * \param[out] v Resulting value in serialized form (should be a size of at least 2).
 * \param be Whether to serialize as Big Endian (\c true) or Little Endian (\c false).
 */
void lrpt_utils_s_uint16_t(
        uint16_t x,
        unsigned char *v,
        bool be);

/** Deserialize to the \c uint16_t value.
 *
 * \param x Value to be deserialized (should be a size of at least 2).
 * \param be Whether to deserialize from Big Endian form (\c true) or Little Endian form (\c false).
 *
 * \return \c uint16_t value.
 */
uint16_t lrpt_utils_ds_uint16_t(
        const unsigned char *x,
        bool be);

/** Serialize \c int16_t value.
 *
 * \param x Value to be serialized.
 * \param[out] v Resulting value in serialized form (should be a size of at least 2).
 * \param be Whether to serialize as Big Endian (\c true) or Little Endian (c false).
 */
void lrpt_utils_s_int16_t(
        int16_t x,
        unsigned char *v,
        bool be);

/** Deserialize to the \c int16_t value.
 *
 * \param x Value to be deserialized (should be a size of at least 2).
 * \param be Whether to deserialize from Big Endian form (\c true) or Little Endian form (\c false).
 *
 * \return \c int16_t value.
 */
int16_t lrpt_utils_ds_int16_t(
        const unsigned char *x,
        bool be);

/** Serialize \c uint32_t value.
 *
 * \param x Value to be serialized.
 * \param[out] v Resulting value in serialized form (should be a size of at least 4).
 * \param be Whether to serialize as Big Endian (\c true) or Little Endian (c false).
 */
void lrpt_utils_s_uint32_t(
        uint32_t x,
        unsigned char *v,
        bool be);

/** Deserialize to the \c uint32_t value.
 *
 * \param x Value to be deserialized (should be a size of at least 4).
 * \param be Whether to deserialize from Big Endian form (\c true) or Little Endian form (\c false).
 *
 * \return \c uint32_t value.
 */
uint32_t lrpt_utils_ds_uint32_t(
        const unsigned char *x,
        bool be);

/** Serialize \c int32_t value.
 *
 * \param x Value to be serialized.
 * \param[out] v Resulting value in serialized form (should be a size of at least 4).
 * \param be Whether to serialize as Big Endian (\c true) or Little Endian (c false).
 */
void lrpt_utils_s_int32_t(
        int32_t x,
        unsigned char *v,
        bool be);

/** Deserialize to the \c int32_t value.
 *
 * \param x Value to be deserialized (should be a size of at least 4).
 * \param be Whether to deserialize from Big Endian form (\c true) or Little Endian form (\c false).
 *
 * \return \c int32_t value.
 */
int32_t lrpt_utils_ds_int32_t(
        const unsigned char *x,
        bool be);

/** Serialize \c uint64_t value.
 *
 * \param x Value to be serialized.
 * \param[out] v Resulting value in serialized form (should be a size of at least 8).
 * \param be Whether to serialize as Big Endian (\c true) or Little Endian (c false).
 */
void lrpt_utils_s_uint64_t(
        uint64_t x,
        unsigned char *v,
        bool be);

/** Deserialize to the \c uint64_t value.
 *
 * \param x Value to be deserialized (should be a size of at least 8).
 * \param be Whether to deserialize from Big Endian form (\c true) or Little Endian form (\c false).
 *
 * \return \c uint64_t value.
 */
uint64_t lrpt_utils_ds_uint64_t(
        const unsigned char *x,
        bool be);

/** Serialize \c int64_t value.
 *
 * \param x Value to be serialized.
 * \param[out] v Resulting value in serialized form (should be a size of at least 8).
 * \param be Whether to serialize as Big Endian (\c true) or Little Endian (c false).
 */
void lrpt_utils_s_int64_t(
        int64_t x,
        unsigned char *v,
        bool be);

/** Deserialize to the \c int64_t value.
 *
 * \param x Value to be deserialized (should be a size of at least 8).
 * \param be Whether to deserialize from Big Endian form (\c true) or Little Endian form (\c false).
 *
 * \return \c int64_t value.
 */
int64_t lrpt_utils_ds_int64_t(
        const unsigned char *x,
        bool be);

/** Serialize \c double value.
 *
 * Input \c double value is decomposed to the exponent and normalized mantissa. Exponent part is
 * serialized with the help of #lrpt_utils_s_int16_t() while mantissa is multiplied by factor
 * \c 2^53 and serialized with #lrpt_utils_s_int64_t(). Serialized exponent and mantissa are then
 * concatenated in mentioned order to form single serialized value.
 *
 * \param x Value to be serialized.
 * \param[out] v Resulting value in serialized form (should be a size of at least 10).
 * \param be Whether to serialize as Big Endian (\c true) or Little Endian (c false).
 *
 * \return \c true on successfull serialization and \c false in case of error.
 *
 * \note If \p x is NaN or infinity (of any sign) then this function will return \c false and \p v
 * will not be modified.
 */
bool lrpt_utils_s_double(
        double x,
        unsigned char *v,
        bool be);

/** Deserialize to the \c double value.
 *
 * Input value should be in form of serialized exponent and normalized mantissa (for more
 * details see #lrpt_utils_s_double()) parts concatenated in mentioned order.
 *
 * \param x Value to be deserialized (should be a size of at least 10).
 * \param[out] v Pointer to the resulting \c double value.
 * \param be Whether to deserialize from Big Endian form (\c true) or Little Endian form (\c false).
 *
 * \return \c true on successfull deserialization and \c false in case of error.
 *
 * \note If deserialized mantissa is NaN or infinity (of any sign) \c false will be returned and
 * \p v will not be modified. Also if numerical overflow has occured during exponent loading
 * \c false will be returned as well (\p v will not be modified too).
 */
bool lrpt_utils_ds_double(
        const unsigned char *x,
        double *v,
        bool be);

/** Perform gamma correction according to the BT.709 transfer function.
 *
 * \param val Linear pixel value.
 *
 * \return Corrected pixel value.
 */
uint8_t lrpt_utils_bt709_gamma_encode(
        uint8_t val);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
