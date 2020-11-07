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
 * Basic data types, memory management and I/O routines.
 *
 * This source file contains common routines for internal data types and objects management,
 * I/O tasks etc.
 */

/*************************************************************************************************/

#include "lrpt.h"

#include "../../include/lrpt.h"
#include "utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

/* lrpt_iq_data_alloc() */
lrpt_iq_data_t *lrpt_iq_data_alloc(
        size_t length) {
    lrpt_iq_data_t *handle = malloc(sizeof(lrpt_iq_data_t));

    if (!handle)
        return NULL;

    /* Set requested length and allocate storage for I and Q samples if <length> is not zero */
    handle->len = length;

    if (length > 0) {
        handle->iq = calloc(length, sizeof(lrpt_iq_raw_t));

        /* Return <NULL> only if allocation attempt has failed */
        if (!handle->iq) {
            lrpt_iq_data_free(handle);

            return NULL;
        }
    }
    else
        handle->iq = NULL;

    return handle;
}

/*************************************************************************************************/

/* lrpt_iq_data_free() */
void lrpt_iq_data_free(
        lrpt_iq_data_t *handle) {
    if (!handle)
        return;

    free(handle->iq);
    free(handle);
}

/*************************************************************************************************/

/* lrpt_iq_data_length() */
size_t lrpt_iq_data_length(
        const lrpt_iq_data_t *handle) {
    return handle->len;
}

/*************************************************************************************************/

/* lrpt_iq_data_resize() */
bool lrpt_iq_data_resize(
        lrpt_iq_data_t *handle,
        size_t new_length) {
    /* We accept only valid handles or simple empty handles */
    if (!handle || ((handle->len > 0) && !handle->iq))
        return false;

    /* Is sizes are the same just return true */
    if (handle->len == new_length)
        return true;

    /* In case of zero length create empty but valid handle */
    if (new_length == 0) {
        free(handle->iq);

        handle->len = 0;
        handle->iq = NULL;
    }
    else {
        lrpt_iq_raw_t *new_iq = reallocarray(handle->iq, new_length, sizeof(lrpt_iq_raw_t));

        if (!new_iq)
            return false;
        else {
            /* Wipe out newly allocated portion */
            if (new_length > handle->len)
                memset(new_iq + handle->len, 0, new_length - handle->len);

            handle->len = new_length;
            handle->iq = new_iq;
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_file_open() */
lrpt_iq_file_t *lrpt_iq_file_open(
        const char *fname) {
    FILE *fh = fopen(fname, "rb");

    if (!fh)
        return NULL;

    /* Check file header information. Header should be 6-character string "lrptiq" */
    char header[6];

    if ((fread(header, sizeof(char), 6, fh) != 6) || strncmp(header, "lrptiq", 6) != 0) {
        fclose(fh);

        return NULL;
    }

    /* Read file format version info */
    uint8_t ver;

    if (fread(&ver, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        return NULL;
    }

    /* Read sample rate */
    unsigned char sr_s[4];

    if (fread(sr_s, sizeof(unsigned char), 4, fh) != 4) {
        fclose(fh);

        return NULL;
    }

    uint32_t sr = lrpt_utils_ds_uint32_t(sr_s);

    /* Read in device name length */
    uint8_t name_l;

    if (fread(&name_l, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        return NULL;
    }

    /* Read device name info */
    char *name = NULL;

    /* Allocate storage for device name only if it's presented in I/Q file */
    if (name_l > 0) {
        name = calloc(name_l + 1, sizeof(char));

        if (!name || (fread(name, sizeof(char), name_l, fh) != name_l)) {
            free(name);
            fclose(fh);

            return NULL;
        }
    }

    /* Read data length */
    unsigned char data_l_s[8];

    if (fread(data_l_s, sizeof(unsigned char), 8, fh) != 8) {
        free(name);
        fclose(fh);

        return NULL;
    }

    uint64_t data_l = lrpt_utils_ds_uint64_t(data_l_s);

    /* Perform sanity checking - currently one complex I/Q sample is encoded as two doubles
     * and each double is serialized to the 10 unsigned chars
     */
    uint64_t cur_pos = ftell(fh);
    fseek(fh, 0, SEEK_END);
    uint64_t n_iq = (ftell(fh) - cur_pos) / (sizeof(unsigned char) * 10 * 2);
    fseek(fh, cur_pos, SEEK_SET);

    if (n_iq != data_l) {
        free(name);
        fclose(fh);

        return NULL;
    }

    /* Create I/Q data file object and return it */
    lrpt_iq_file_t *handle = malloc(sizeof(lrpt_iq_file_t));

    if (!handle) {
        free(name);
        fclose(fh);

        return NULL;
    }

    handle->fhandle = fh;
    handle->version = ver;
    handle->samplerate = sr;
    handle->device_name = name;
    handle->header_length = 20 + name_l; /* Just a sum of all elements previously read */
    handle->data_length = data_l;

    return handle;
}

/*************************************************************************************************/

/* lrpt_iq_file_close() */
void lrpt_iq_file_close(
        lrpt_iq_file_t *handle) {
    if (!handle)
        return;

    free(handle->device_name);
    fclose(handle->fhandle);
    free(handle);
}

/*************************************************************************************************/

/* lrpt_iq_file_version() */
uint8_t lrpt_iq_file_version(
        const lrpt_iq_file_t *handle) {
    return handle->version;
}

/*************************************************************************************************/

/* lrpt_iq_file_samplerate() */
uint32_t lrpt_iq_file_samplerate(
        const lrpt_iq_file_t *handle) {
    return handle->samplerate;
}

/*************************************************************************************************/

/* lrpt_iq_file_devicename() */
const char *lrpt_iq_file_devicename(
        const lrpt_iq_file_t *handle) {
    return handle->device_name;
}

/*************************************************************************************************/

/* lrpt_iq_file_length() */
uint64_t lrpt_iq_file_length(
        const lrpt_iq_file_t *handle) {
    return handle->data_length;
}
/*************************************************************************************************/

/* lrpt_iq_data_read_from_file() */
bool lrpt_iq_data_read_from_file(
        lrpt_iq_data_t *data,
        lrpt_iq_file_t *file,
        uint64_t start,
        uint64_t length) {
    /* Check if we have enough data to read */
    if ((start + length) > file->data_length)
        return false;

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, length))
        return false;

    /* Move to the desired position. The same offset math is used as in lrpt_iq_file_open(). */
    fseek(file->fhandle, file->header_length + start * 10 * 2, SEEK_SET);

    for (uint64_t k = 0; k < length; k++) {
        unsigned char v_s[10];

        if (fread(v_s, sizeof(unsigned char) * 10, 1, file->fhandle) != 1)
            return false;

        if (!lrpt_utils_ds_double(v_s, &(data->iq[k].i)))
            return false;

        if (fread(v_s, sizeof(unsigned char) * 10, 1, file->fhandle) != 1)
            return false;

        if (!lrpt_utils_ds_double(v_s, &(data->iq[k].q)))
            return false;
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_save_to_file() */
bool lrpt_iq_data_save_to_file(
        lrpt_iq_data_t *handle,
        const char *fname,
        uint8_t version,
        uint32_t samplerate,
        const char *device_name) {
    if (!handle || (handle->len == 0) || !handle->iq)
        return false;

    FILE *fh = fopen(fname, "wb");

    if (!fh)
        return false;

    /* Write file header and version */
    fwrite("lrptiq", sizeof(char), 6, fh); /* File format ID */
    fwrite(&version, sizeof(uint8_t), 1, fh); /* File format version */

    /* Write sampling rate info */
    unsigned char sr_s[4];

    lrpt_utils_s_uint32_t(samplerate, sr_s);
    fwrite(sr_s, sizeof(unsigned char), 4, fh); /* Sampling rate */

    /* Write device name */
    uint8_t name_l = 0;

    /* We'll write it only if user has requested it */
    if (device_name != NULL) {
        if (strlen(device_name) > 255)
            name_l = 255;
        else
            name_l = strlen(device_name);
    }

    fwrite(&name_l, sizeof(uint8_t), 1, fh); /* Device name length */

    if (name_l > 0)
        fwrite(device_name, sizeof(char), name_l, fh);

    /* Write data */
    unsigned char data_l_s[8];

    lrpt_utils_s_uint64_t(handle->len, data_l_s);
    fwrite(data_l_s, sizeof(unsigned char), 8, fh); /* Data length (in number of I/Q samples) */

    for (size_t k = 0; k < handle->len; k++) {
        unsigned char v_s[10];

        if (!lrpt_utils_s_double(handle->iq[k].i, v_s)) {
            fclose(fh);

            return false;
        }
        else
            fwrite(v_s, sizeof(unsigned char), 10, fh);

        if (!lrpt_utils_s_double(handle->iq[k].q, v_s)) {
            fclose(fh);

            return false;
        }
        else
            fwrite(v_s, sizeof(unsigned char), 10, fh);
    }

    fclose(fh);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_doubles() */
bool lrpt_iq_data_from_doubles(
        lrpt_iq_data_t *handle,
        const double *i,
        const double *q,
        size_t length) {
    if (!handle)
        return false;

    /* Resize storage */
    if (!lrpt_iq_data_resize(handle, length))
        return false;

    /* Repack doubles into I/Q data */
    for (size_t k = 0; k < length; k++) {
        handle->iq[k].i = *(i + k);
        handle->iq[k].q = *(q + k);
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_doubles() */
lrpt_iq_data_t *lrpt_iq_data_create_from_doubles(
        const double *i,
        const double *q,
        size_t length) {
    lrpt_iq_data_t *handle = lrpt_iq_data_alloc(length);

    if (!handle)
        return NULL;

    if (!lrpt_iq_data_from_doubles(handle, i, q, length)) {
        lrpt_iq_data_free(handle);

        return NULL;
    }

    return handle;
}

/*************************************************************************************************/

/* lrpt_iq_data_from_samples() */
bool lrpt_iq_data_from_samples(
        lrpt_iq_data_t *handle,
        const lrpt_iq_raw_t *iq,
        size_t length) {
    if (!handle)
        return false;

    /* Resize storage */
    if (!lrpt_iq_data_resize(handle, length))
        return false;

    /* Merge samples into I/Q data */
    for (size_t k = 0; k < length; k++)
        handle->iq[k] = *(iq + k);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_create_from_samples() */
lrpt_iq_data_t *lrpt_iq_data_create_from_samples(
        const lrpt_iq_raw_t *iq,
        size_t length) {
    lrpt_iq_data_t *handle = lrpt_iq_data_alloc(length);

    if (!handle)
        return NULL;

    if (!lrpt_iq_data_from_samples(handle, iq, length)) {
        lrpt_iq_data_free(handle);

        return NULL;
    }

    return handle;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_alloc() */
lrpt_qpsk_data_t *lrpt_qpsk_data_alloc(
        size_t length) {
    lrpt_qpsk_data_t *handle = malloc(sizeof(lrpt_qpsk_data_t));

    if (!handle)
        return NULL;

    /* Set requested length and allocate storage for soft symbols if <length> is not zero */
    handle->len = length;

    if (length > 0) {
        handle->s = calloc(length, sizeof(int8_t));

        /* Return <NULL> only if allocation attempt has failed */
        if (!handle->s) {
            lrpt_qpsk_data_free(handle);

            return NULL;
        }
    }
    else
        handle->s = NULL;

    return handle;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_free() */
void lrpt_qpsk_data_free(
        lrpt_qpsk_data_t *handle) {
    if (!handle)
        return;

    free(handle->s);
    free(handle);
}

/*************************************************************************************************/

/* lrpt_qpsk_data_resize() */
bool lrpt_qpsk_data_resize(
        lrpt_qpsk_data_t *handle,
        size_t new_length) {
    /* We accept only valid handles or simple empty handles */
    if (!handle || ((handle->len > 0) && !handle->s))
        return false;

    /* Is sizes are the same just return true */
    if (handle->len == new_length)
        return true;

    /* In case of zero length create empty but valid handle */
    if (new_length == 0) {
        free(handle->s);

        handle->len = 0;
        handle->s = NULL;
    }
    else {
        int8_t *new_s = reallocarray(handle->s, new_length, sizeof(int8_t));

        if (!new_s)
            return false;
        else {
            /* Wipe out newly allocated portion */
            if (new_length > handle->len)
                memset(new_s + handle->len, 0, new_length - handle->len);

            handle->len = new_length;
            handle->s = new_s;
        }
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
