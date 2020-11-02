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
 * Public internal API for different library utils.
 */

/*************************************************************************************************/

#ifndef LRPT_LIBLRPT_UTILS_H
#define LRPT_LIBLRPT_UTILS_H

/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************************************/

/** Serializes uint16_t value to Big Endian representation.
 *
 * \param x Input value to be serialized.
 * \param[out] v Resulting value in Big Endian form.
 */
void lrpt_utils_s_uint16_t(uint16_t x, unsigned char *v);

/** Deserializes Big Endian representation to the uint16_t value.
 *
 * \param x Input value to be deserialized (in Big Endian form).
 *
 * \return uint16_t value.
 */
uint16_t lrpt_utils_ds_uint16_t(const unsigned char *x);

/** Serializes int16_t value to Big Endian representation.
 *
 * \param x Input value to be serialized.
 * \param[out] v Resulting value in Big Endian form.
 */
void lrpt_utils_s_int16_t(int16_t x, unsigned char *v);

/** Deserializes Big Endian representation to the int16_t value.
 *
 * \param x Input value to be deserialized (in Big Endian form).
 *
 * \return int16_t value.
 */
int16_t lrpt_utils_ds_int16_t(const unsigned char *x);

/** Serializes uint32_t value to Big Endian representation.
 *
 * \param x Input value to be serialized.
 * \param[out] v Resulting value in Big Endian form.
 */
void lrpt_utils_s_uint32_t(uint32_t x, unsigned char *v);

/** Deserializes Big Endian representation to the uint32_t value.
 *
 * \param x Input value to be deserialized (in Big Endian form).
 *
 * \return uint32_t value.
 */
uint32_t lrpt_utils_ds_uint32_t(const unsigned char *x);

/** Serializes int32_t value to Big Endian representation.
 *
 * \param x Input value to be serialized.
 * \param[out] v Resulting value in Big Endian form.
 */
void lrpt_utils_s_int32_t(int32_t x, unsigned char *v);

/** Deserializes Big Endian representation to the int32_t value.
 *
 * \param x Input value to be deserialized (in Big Endian form).
 *
 * \return int32_t value.
 */
int32_t lrpt_utils_ds_int32_t(const unsigned char *x);

/** Serializes uint64_t value to Big Endian representation.
 *
 * \param x Input value to be serialized.
 * \param[out] v Resulting value in Big Endian form.
 */
void lrpt_utils_s_uint64_t(uint64_t x, unsigned char *v);

/** Deserializes Big Endian representation to the uint64_t value.
 *
 * \param x Input value to be deserialized (in Big Endian form).
 *
 * \return uint64_t value.
 */
uint64_t lrpt_utils_ds_uint64_t(const unsigned char *x);

/** Serializes int64_t value to Big Endian representation.
 *
 * \param x Input value to be serialized.
 * \param[out] v Resulting value in Big Endian form.
 */
void lrpt_utils_s_int64_t(int64_t x, unsigned char *v);

/** Deserializes Big Endian representation to the int64_t value.
 *
 * \param x Input value to be deserialized (in Big Endian form).
 *
 * \return int64_t value.
 */
int64_t lrpt_utils_ds_int64_t(const unsigned char *x);

/** Serializes double value to Big Endian portable representation.
 *
 * Input double value is decomposed to the exponent and normalized mantissa. Exponent part is
 * serialized with the help of #lrpt_utils_s_int16_t() while mantissa is multiplied by 2^53 and
 * serialized with #lrpt_utils_s_int64_t(). Serialized exponent and mantissa then concatenated
 * to form single serialized value.
 *
 * \param x Input value to be serialized.
 * \param[out] v Resulting value in Big Endian form.
 *
 * \return true on successfull serialization and false otherwise.
 *
 * \note If \p x is NaN or infinity (of any sign) then this function will return false and \p v
 * will not be modified.
 */
bool lrpt_utils_s_double(double x, unsigned char *v);

/** Deserializes Big Endian portable representation to the double value.
 *
 * Input value should be in form of serialized exponent and normalized mantissa (for more
 * details see #lrpt_utils_s_double()) parts concatenated in order.
 *
 * \param x Input value to be deserialized (in Big Endian form).
 * \param[out] v Pointer to the resulting double value.
 *
 * \return true on successfull deserialization and false otherwise.
 *
 * \note If deserialized mantissa is NaN or infinity (of any sign) false will be returned and
 * \p v will not be modified. Also if numerical overflow has occured during exponent loading
 * false will be returned as well (\p v will not be modified too).
 */
bool lrpt_utils_ds_double(const unsigned char *x, double *v);

/*************************************************************************************************/

#endif

/*************************************************************************************************/

/** \endcond */
