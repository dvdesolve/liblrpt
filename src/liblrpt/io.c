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
 * I/O routines.
 *
 * This source file contains common routines for working with I/O tasks.
 */

#include "io.h"

#include "../../include/lrpt.h"
#include "lrpt.h"
#include "utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/** Open I/Q samples file of Version 1 for reading.
 *
 * \param fh Pointer to the \c FILE object.
 *
 * \return Pointer to the I/Q file object or \c NULL in case of error.
 */
static lrpt_iq_file_t *file_open_r_v1(
        FILE *fh);

/*************************************************************************************************/

/* file_open_r_v1() */
static lrpt_iq_file_t *file_open_r_v1(
        FILE *fh) {
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
    lrpt_iq_file_t *file = malloc(sizeof(lrpt_iq_file_t));

    if (!file) {
        free(name);
        fclose(fh);

        return NULL;
    }

    file->fhandle = fh;
    file->write_mode = false;
    file->version = LRPT_IQ_FILE_VER_1;
    file->samplerate = sr;
    file->device_name = name;
    file->header_length = 20 + name_l; /* Just a sum of all elements previously read */
    file->data_length = data_l;

    return file;
}

/*************************************************************************************************/

/* lrpt_iq_file_open_r() */
lrpt_iq_file_t *lrpt_iq_file_open_r(
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

    switch (ver) {
        case LRPT_IQ_FILE_VER_1:
            return file_open_r_v1(fh);

            break;

        default:
            fclose(fh);

            return NULL;
    }
}

/*************************************************************************************************/

/* lrpt_iq_file_open_w_v1() */
lrpt_iq_file_t *lrpt_iq_file_open_w_v1(
        const char *fname,
        uint32_t samplerate,
        const char *device_name) {
    FILE *fh = fopen(fname, "wb");

    if (!fh)
        return NULL;

    const uint8_t version = LRPT_IQ_FILE_VER_1;

    /* Write file header and version */
    if (fwrite("lrptiq", sizeof(char), 6, fh) != 6) {
        fclose(fh);

        return NULL;
    }

    if (fwrite(&version, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        return NULL;
    }

    /* Write sampling rate info */
    unsigned char sr_s[4];
    lrpt_utils_s_uint32_t(samplerate, sr_s);

    if (fwrite(sr_s, sizeof(unsigned char), 4, fh) != 4) {
        fclose(fh);

        return NULL;
    }

    /* Write device name */
    uint8_t name_l = 0;

    /* We'll write it only if user has requested it */
    if (device_name != NULL) {
        if (strlen(device_name) > 255)
            name_l = 255;
        else
            name_l = strlen(device_name);
    }

    if (fwrite(&name_l, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        return NULL;
    }

    char *name = NULL;

    if (name_l > 0) {
        name = calloc(name_l + 1, sizeof(char));

        if (!name || (fwrite(device_name, sizeof(char), name_l, fh) != name_l)) {
            free(name);
            fclose(fh);

            return NULL;
        }

        strncpy(name, device_name, name_l);
    }

    /* Write initial data length */
    unsigned char data_l_s[8];
    lrpt_utils_s_uint64_t(0, data_l_s);

    if (fwrite(data_l_s, sizeof(unsigned char), 8, fh) != 8) {
        free(name);
        fclose(fh);

        return NULL;
    }

    /* Create I/Q data file object and return it */
    lrpt_iq_file_t *file = malloc(sizeof(lrpt_iq_file_t));

    if (!file) {
        free(name);
        fclose(fh);

        return NULL;
    }

    file->fhandle = fh;
    file->write_mode = true;
    file->version = LRPT_IQ_FILE_VER_1;
    file->samplerate = samplerate;
    file->device_name = name;
    file->header_length = 20 + name_l; /* Just a sum of all elements previously written */
    file->data_length = 0;

    return file;
}

/*************************************************************************************************/

/* lrpt_iq_file_close() */
void lrpt_iq_file_close(
        lrpt_iq_file_t *file) {
    if (!file)
        return;

    free(file->device_name);
    fclose(file->fhandle);
    free(file);
}

/*************************************************************************************************/

/* lrpt_iq_file_version() */
uint8_t lrpt_iq_file_version(
        const lrpt_iq_file_t *file) {
    return file->version;
}

/*************************************************************************************************/

/* lrpt_iq_file_samplerate() */
uint32_t lrpt_iq_file_samplerate(
        const lrpt_iq_file_t *file) {
    return file->samplerate;
}

/*************************************************************************************************/

/* lrpt_iq_file_devicename() */
const char *lrpt_iq_file_devicename(
        const lrpt_iq_file_t *file) {
    return file->device_name;
}

/*************************************************************************************************/

/* lrpt_iq_file_length() */
uint64_t lrpt_iq_file_length(
        const lrpt_iq_file_t *file) {
    return file->data_length;
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
        lrpt_iq_data_t *data,
        const char *fname,
        uint8_t version,
        uint32_t samplerate,
        const char *device_name) {
    if (!data || (data->len == 0) || !data->iq)
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

    lrpt_utils_s_uint64_t(data->len, data_l_s);
    fwrite(data_l_s, sizeof(unsigned char), 8, fh); /* Data length (in number of I/Q samples) */

    for (size_t k = 0; k < data->len; k++) {
        unsigned char v_s[10];

        if (!lrpt_utils_s_double(data->iq[k].i, v_s)) {
            fclose(fh);

            return false;
        }
        else
            fwrite(v_s, sizeof(unsigned char), 10, fh);

        if (!lrpt_utils_s_double(data->iq[k].q, v_s)) {
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

/** \endcond */
