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
 * Helper library utilities.
 */

/*************************************************************************************************/

#include "utils.h"

#include "../../include/lrpt.h"

#include <complex.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*************************************************************************************************/

/* liblrpt requires that IEEE 754 floating point standard is used. However some compilers doesn't
 * provide special macro for testing that. We're quite lenient in this requirement so just plain
 * warning will be given during compilation.
 */
#ifndef __STDC_IEC_559__
#pragma message ("Your compiler doesn't define __STDC_IEC_559__ macro!")
#pragma message ("Support for IEEE 754 floats and doubles may be unavailable or limited!")
#endif

/*************************************************************************************************/

const uint8_t UTILS_DOUBLE_SER_SIZE = 10;
const uint8_t UTILS_COMPLEX_SER_SIZE = (UTILS_DOUBLE_SER_SIZE * 2);

/*************************************************************************************************/

/** Tell if we're running on Big Endian system.
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
        unsigned char *v,
        bool be) {
    union {
        uint16_t ui;
        unsigned char uc[2];
    } t;

    t.ui = x;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        const unsigned char *x,
        bool be) {
    union {
        uint16_t ui;
        unsigned char uc[2];
    } t;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        unsigned char *v,
        bool be) {
    union {
        int16_t si;
        unsigned char uc[2];
    } t;

    t.si = x;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        const unsigned char *x,
        bool be) {
    union {
        int16_t si;
        unsigned char uc[2];
    } t;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        unsigned char *v,
        bool be) {
    union {
        uint32_t ui;
        unsigned char uc[4];
    } t;

    t.ui = x;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        const unsigned char *x,
        bool be) {
    union {
        uint32_t ui;
        unsigned char uc[4];
    } t;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        unsigned char *v,
        bool be) {
    union {
        int32_t si;
        unsigned char uc[4];
    } t;

    t.si = x;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        const unsigned char *x,
        bool be) {
    union {
        int32_t si;
        unsigned char uc[4];
    } t;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        unsigned char *v,
        bool be) {
    union {
        uint64_t ui;
        unsigned char uc[8];
    } t;

    t.ui = x;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        const unsigned char *x,
        bool be) {
    union {
        uint64_t ui;
        unsigned char uc[8];
    } t;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        unsigned char *v,
        bool be) {
    union {
        int64_t si;
        unsigned char uc[8];
    } t;

    t.si = x;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        const unsigned char *x,
        bool be) {
    union {
        int64_t si;
        unsigned char uc[8];
    } t;

    bool fwd = (is_be() == be);

    if (fwd) {
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
        unsigned char *v,
        bool be) {
    /* 2^53 - we must make use of every bit */
    const int64_t c_2to53 = 9007199254740992;

    if (isnan(x))
        return false;

    if (isinf(x))
        return false;

    int e;
    double m = frexp(x, &e);

    unsigned char ex[2];
    unsigned char mant[8];

    lrpt_utils_s_int16_t(e, ex, be);
    lrpt_utils_s_int64_t(c_2to53 * m, mant, be);

    memcpy(v, ex, sizeof(unsigned char) * 2); /* ex is 2 elements long */
    memcpy(v + 2, mant, sizeof(unsigned char) * 8); /* mant is 8 elements long */

    return true;
}

/*************************************************************************************************/

/* lrpt_utils_ds_double() */
bool lrpt_utils_ds_double(
        const unsigned char *x,
        double *v,
        bool be) {
    /* 2^53 - we must make use of every bit */
    const int64_t c_2to53 = 9007199254740992;

    unsigned char ex[2];
    unsigned char mant[8];

    memcpy(ex, x, sizeof(unsigned char) * 2); /* ex is 2 elements long */
    memcpy(mant, x + 2, sizeof(unsigned char) * 8); /* mant is 8 elements long */

    int16_t e = lrpt_utils_ds_int16_t(ex, be);
    double m = (double)lrpt_utils_ds_int64_t(mant, be) / c_2to53;

    if (isnan(m))
        return false;

    if (isinf(m))
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

/* lrpt_utils_s_complex() */
inline bool lrpt_utils_s_complex(
        complex double x,
        unsigned char *v,
        bool be) {
    return (lrpt_utils_s_double(creal(x), v, be) &&
            lrpt_utils_s_double(cimag(x), v + UTILS_DOUBLE_SER_SIZE, be));
}

/*************************************************************************************************/

/* lrpt_utils_ds_complex() */
bool lrpt_utils_ds_complex(
        const unsigned char *x,
        complex double *v,
        bool be) {
    double real, imag;

    bool result = (lrpt_utils_ds_double(x, &real, be) &&
            lrpt_utils_ds_double(x + UTILS_DOUBLE_SER_SIZE, &imag, be));

    if (result) {
        *v = real + imag * I;

        return true;
    }
    else
        return false;
}

/*************************************************************************************************/

/* lrpt_utils_bt709_gamma_encode() */
uint8_t lrpt_utils_bt709_gamma_encode(
        uint8_t val) {
    if ((val / 255.0) < 0.018)
        return (4.5 * val);
    else
        return (255 * (1.099 * pow(val / 255.0, 0.45) - 0.099));
}

/*************************************************************************************************/

/** \endcond */
