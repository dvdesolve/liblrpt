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
 * NOTE: source code of this module is based on xdemorse application and adapted for liblrpt
 * internal use!
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Different library utils.
 *
 * This source file contains routines for performing different library utils.
 */

/*************************************************************************************************/

#include "utils.h"

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*************************************************************************************************/

const uint8_t UTILS_DOUBLE_SER_SIZE = 10;
const uint8_t UTILS_COMPLEX_SER_SIZE = (UTILS_DOUBLE_SER_SIZE * 2);

/*************************************************************************************************/

/** Tells where we're running on Big Endian system.
 *
 * \return \c true if environment is Big Endian and \c false in case of Little Endian.
 */
static inline bool is_be(void);

/*************************************************************************************************/

static inline bool is_be(void) {
    const int i = 1;

    return ((*(char *)&i) == 0);
}

/*************************************************************************************************/

/* lrpt_utils_s_uint16_t() */
void lrpt_utils_s_uint16_t(
        uint16_t x,
        unsigned char *v) {
    union {
        uint16_t ui;
        unsigned char uc[2];
    } t;

    t.ui = x;

    if (is_be()) {
        for (uint8_t i = 0; i < 2; i++)
            v[i] = t.uc[i];
    }
    else {
        for (uint8_t i = 0; i < 2; i++)
            v[i] = t.uc[1 - i];
    }
}

/*************************************************************************************************/

/* lrpt_utils_ds_uint16_t() */
uint16_t lrpt_utils_ds_uint16_t(
        const unsigned char *x) {
    union {
        uint16_t ui;
        unsigned char uc[2];
    } t;

    if (is_be()) {
        for (uint8_t i = 0; i < 2; i++)
            t.uc[i] = x[i];
    }
    else {
        for (uint8_t i = 0; i < 2; i++)
            t.uc[i] = x[1 - i];
    }

    return t.ui;
}

/*************************************************************************************************/

/* lrpt_utils_s_int16_t() */
void lrpt_utils_s_int16_t(
        int16_t x,
        unsigned char *v) {
    union {
        int16_t si;
        unsigned char uc[2];
    } t;

    t.si = x;

    if (is_be()) {
        for (uint8_t i = 0; i < 2; i++)
            v[i] = t.uc[i];
    }
    else {
        for (uint8_t i = 0; i < 2; i++)
            v[i] = t.uc[1 - i];
    }
}

/*************************************************************************************************/

/* lrpt_utils_ds_int16_t() */
int16_t lrpt_utils_ds_int16_t(
        const unsigned char *x) {
    union {
        int16_t si;
        unsigned char uc[2];
    } t;

    if (is_be()) {
        for (uint8_t i = 0; i < 2; i++)
            t.uc[i] = x[i];
    }
    else {
        for (uint8_t i = 0; i < 2; i++)
            t.uc[i] = x[1 - i];
    }

    return t.si;
}

/*************************************************************************************************/

/* lrpt_utils_s_uint32_t() */
void lrpt_utils_s_uint32_t(
        uint32_t x,
        unsigned char *v) {
    union {
        uint32_t ui;
        unsigned char uc[4];
    } t;

    t.ui = x;

    if (is_be()) {
        for (uint8_t i = 0; i < 4; i++)
            v[i] = t.uc[i];
    }
    else {
        for (uint8_t i = 0; i < 4; i++)
            v[i] = t.uc[3 - i];
    }
}

/*************************************************************************************************/

/* lrpt_utils_ds_uint32_t() */
uint32_t lrpt_utils_ds_uint32_t(
        const unsigned char *x) {
    union {
        uint32_t ui;
        unsigned char uc[4];
    } t;

    if (is_be()) {
        for (uint8_t i = 0; i < 4; i++)
            t.uc[i] = x[i];
    }
    else {
        for (uint8_t i = 0; i < 4; i++)
            t.uc[i] = x[3 - i];
    }

    return t.ui;
}

/*************************************************************************************************/

/* lrpt_utils_s_int32_t() */
void lrpt_utils_s_int32_t(
        int32_t x,
        unsigned char *v) {
    union {
        int32_t si;
        unsigned char uc[4];
    } t;

    t.si = x;

    if (is_be()) {
        for (uint8_t i = 0; i < 4; i++)
            v[i] = t.uc[i];
    }
    else {
        for (uint8_t i = 0; i < 4; i++)
            v[i] = t.uc[3 - i];
    }
}

/*************************************************************************************************/

/* lrpt_utils_ds_int32_t() */
int32_t lrpt_utils_ds_int32_t(
        const unsigned char *x) {
    union {
        int32_t si;
        unsigned char uc[4];
    } t;

    if (is_be()) {
        for (uint8_t i = 0; i < 4; i++)
            t.uc[i] = x[i];
    }
    else {
        for (uint8_t i = 0; i < 4; i++)
            t.uc[i] = x[3 - i];
    }

    return t.si;
}

/*************************************************************************************************/

/* lrpt_utils_s_uint64_t() */
void lrpt_utils_s_uint64_t(
        uint64_t x,
        unsigned char *v) {
    union {
        uint64_t ui;
        unsigned char uc[8];
    } t;

    t.ui = x;

    if (is_be()) {
        for (uint8_t i = 0; i < 8; i++)
            v[i] = t.uc[i];
    }
    else {
        for (uint8_t i = 0; i < 8; i++)
            v[i] = t.uc[7 - i];
    }
}

/*************************************************************************************************/

/* lrpt_utils_ds_uint64_t() */
uint64_t lrpt_utils_ds_uint64_t(
        const unsigned char *x) {
    union {
        uint64_t ui;
        unsigned char uc[8];
    } t;

    if (is_be()) {
        for (uint8_t i = 0; i < 8; i++)
            t.uc[i] = x[i];
    }
    else {
        for (uint8_t i = 0; i < 8; i++)
            t.uc[i] = x[7 - i];
    }

    return t.ui;
}

/*************************************************************************************************/

/* lrpt_utils_s_int64_t() */
void lrpt_utils_s_int64_t(
        int64_t x,
        unsigned char *v) {
    union {
        int64_t si;
        unsigned char uc[8];
    } t;

    t.si = x;

    if (is_be()) {
        for (uint8_t i = 0; i < 8; i++)
            v[i] = t.uc[i];
    }
    else {
        for (uint8_t i = 0; i < 8; i++)
            v[i] = t.uc[7 - i];
    }
}

/*************************************************************************************************/

/* lrpt_utils_ds_int64_t() */
int64_t lrpt_utils_ds_int64_t(
        const unsigned char *x) {
    union {
        int64_t si;
        unsigned char uc[8];
    } t;

    if (is_be()) {
        for (uint8_t i = 0; i < 8; i++)
            t.uc[i] = x[i];
    }
    else {
        for (uint8_t i = 0; i < 8; i++)
            t.uc[i] = x[7 - i];
    }

    return t.si;
}

/*************************************************************************************************/

/* lrpt_utils_s_double() */
bool lrpt_utils_s_double(
        double x,
        unsigned char *v) {
    /* 2^53 - we must make use of every bit */
    const int64_t c_2to53 = 9007199254740992;

    if (isnan(x) || isinf(x))
        return false;

    int e;
    double m = frexp(x, &e);

    unsigned char ex[2];
    unsigned char mant[8];

    lrpt_utils_s_int16_t(e, ex);
    lrpt_utils_s_int64_t(c_2to53 * m, mant);

    memcpy(v, ex, sizeof(unsigned char) * 2); /* ex is 2 elements long */
    memcpy(v + 2, mant, sizeof(unsigned char) * 8); /* mant is 8 elements long */

    return true;
}

/*************************************************************************************************/

/* lrpt_utils_ds_double() */
bool lrpt_utils_ds_double(
        const unsigned char *x,
        double *v) {
    /* 2^53 - we must make use of every bit */
    const int64_t c_2to53 = 9007199254740992;

    unsigned char ex[2];
    unsigned char mant[8];

    memcpy(ex, x, sizeof(unsigned char) * 2); /* ex is 2 elements long */
    memcpy(mant, x + 2, sizeof(unsigned char) * 8); /* mant is 8 elements long */

    int16_t e = lrpt_utils_ds_int16_t(ex);
    double m = (double)lrpt_utils_ds_int64_t(mant) / c_2to53;

    if (isnan(m) || isinf(m))
        return false;

    double t = ldexp(m, e);

    if (errno == ERANGE)
        return false;
    else {
        *v = t;
        return true;
    }
}

/*************************************************************************************************/

/** \endcond */
