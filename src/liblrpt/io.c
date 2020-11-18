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
    /* File position = 7 */

    /* Read sample rate */
    unsigned char sr_s[4];

    if (fread(sr_s, sizeof(unsigned char), 4, fh) != 4) {
        fclose(fh);

        return NULL;
    }

    uint32_t sr = lrpt_utils_ds_uint32_t(sr_s);

    /* File position = 11 */

    /* Read in device name length */
    uint8_t name_l;

    if (fread(&name_l, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        return NULL;
    }

    /* File position = 12 */

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

    /* File position = 12 + name_l */

    /* Read data length */
    unsigned char data_l_s[8];

    if (fread(data_l_s, sizeof(unsigned char), 8, fh) != 8) {
        free(name);
        fclose(fh);

        return NULL;
    }

    uint64_t data_l = lrpt_utils_ds_uint64_t(data_l_s);

    /* File position = 20 + name_l */

    /* Perform sanity checking - one complex I/Q sample is encoded as two doubles and each double
     * is serialized to the 10 unsigned chars
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
    file->current = 0;

    return file;
}

/*************************************************************************************************/

/* lrpt_iq_file_open_r() */
lrpt_iq_file_t *lrpt_iq_file_open_r(
        const char *fname) {
    FILE *fh = fopen(fname, "rb");

    if (!fh)
        return NULL;

    /* File position = 0 */

    /* Check file header information. Header should be 6-character string "lrptiq" */
    char header[6];

    if ((fread(header, sizeof(char), 6, fh) != 6) || strncmp(header, "lrptiq", 6) != 0) {
        fclose(fh);

        return NULL;
    }

    /* File position = 6 */

    /* Read file format version info */
    uint8_t ver;

    if (fread(&ver, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        return NULL;
    }

    /* File position = 7 */

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

    /* File position = 0 */

    const uint8_t version = LRPT_IQ_FILE_VER_1;

    /* Write file header and version */
    if (fwrite("lrptiq", sizeof(char), 6, fh) != 6) {
        fclose(fh);

        return NULL;
    }

    /* File position = 6 */

    if (fwrite(&version, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        return NULL;
    }

    /* File position = 7 */

    /* Write sampling rate info */
    unsigned char sr_s[4];
    lrpt_utils_s_uint32_t(samplerate, sr_s);

    if (fwrite(sr_s, sizeof(unsigned char), 4, fh) != 4) {
        fclose(fh);

        return NULL;
    }

    /* File position = 11 */

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

    /* File position = 12 */

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

    /* File position = 12 + name_l */

    /* Write initial data length */
    unsigned char data_l_s[8];
    lrpt_utils_s_uint64_t(0, data_l_s);

    if (fwrite(data_l_s, sizeof(unsigned char), 8, fh) != 8) {
        free(name);
        fclose(fh);

        return NULL;
    }

    /* File position = 20 + name_l */

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
    file->current = 0;

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

/* lrpt_iq_file_goto() */
bool lrpt_iq_file_goto(
        lrpt_iq_file_t *file,
        uint64_t sample) {
    if (!file || !file->fhandle || sample >= file->data_length)
        return false;

    fseek(file->fhandle, file->header_length + sample * 10 * 2, SEEK_SET);

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_read_from_file() */
bool lrpt_iq_data_read_from_file(
        lrpt_iq_data_t *data,
        lrpt_iq_file_t *file,
        size_t length,
        bool rewind) {
    /* TODO implement block read */
    /* Check if we have enough data to read */
    if (!data || !data->iq ||
            !file || !file->fhandle || file->write_mode || file->current == file->data_length)
        return false;

    /* Read up to the end if requested length is bigger than remaining data */
    if ((file->current + length) > file->data_length)
        length = file->data_length - file->current;

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, length))
        return false;

    for (uint64_t i = 0; i < length; i++) {
        /* TODO read at once */
        unsigned char v_s[10];

        if (fread(v_s, sizeof(unsigned char) * 10, 1, file->fhandle) != 1)
            return false;

        if (!lrpt_utils_ds_double(v_s, &(data->iq[i].i)))
            return false;

        if (fread(v_s, sizeof(unsigned char) * 10, 1, file->fhandle) != 1)
            return false;

        if (!lrpt_utils_ds_double(v_s, &(data->iq[i].q)))
            return false;
    }

    if (rewind)
        lrpt_iq_file_goto(file, file->current);
    else
        file->current += length;

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_write_to_file() */
bool lrpt_iq_data_write_to_file(
        const lrpt_iq_data_t *data,
        lrpt_iq_file_t *file) {
    /* TODO implement block read */
    if (!data || (data->len == 0) || !data->iq ||
            !file || !file->fhandle || !file->write_mode)
        return false;

    for (size_t i = 0; i < data->len; i++) {
        /* TODO write at once */
        unsigned char v_s[10];

        if (!lrpt_utils_s_double(data->iq[i].i, v_s))
            return false;

        if (fwrite(v_s, sizeof(unsigned char), 10, file->fhandle) != 10)
            return false;

        if (!lrpt_utils_s_double(data->iq[i].q, v_s))
            return false;

        if (fwrite(v_s, sizeof(unsigned char), 10, file->fhandle) != 10)
            return false;

        file->current += 1;
        file->data_length += 1;
        /* TODO implement backwriting of data length - may be at file close */
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
