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
 * I/O routines.
 *
 * Routines for dealing with I/O tasks.
 */

#include "io.h"

#include "../../include/lrpt.h"
#include "error.h"
#include "lrpt.h"
#include "utils.h"

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO deal with hard and soft QPSK symbols and related stuff in accessor functions (bytes vs symbols */

/*************************************************************************************************/

static const size_t IO_IQ_DATA_N = 1024; /**< Block size for I/O operations with I/Q data */
static const size_t IO_QPSK_DATA_N = 1024; /**< Block size for I/O operations with QPSK data */

/*************************************************************************************************/

/** Open I/Q samples file, Version 1 for reading.
 *
 * \param fh Pointer to the \c FILE object.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the I/Q file object or \c NULL in case of error.
 */
static lrpt_iq_file_t *iq_file_open_r_v1(
        FILE *fh,
        lrpt_error_t *err);

/** Open QPSK symbols data file, Version 1 for reading.
 *
 * \param fh Pointer to the \c FILE object.
 * \param err Pointer to the error object (set to \c NULL if no error reporting is needed).
 *
 * \return Pointer to the QPSK file object or \c NULL in case of error.
 */
static lrpt_qpsk_file_t *qpsk_file_open_r_v1(
        FILE *fh,
        lrpt_error_t *err);

/*************************************************************************************************/

/* iq_file_open_r_v1() */
static lrpt_iq_file_t *iq_file_open_r_v1(
        FILE *fh,
        lrpt_error_t *err) {
    /* File position = 7 */

    /* Read flags */
    unsigned char flags;

    if (fread(&flags, 1, 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "I/Q file ver. 1 flags read error");

        return NULL;
    }

    /* File position = 8 */

    /* Read sample rate */
    unsigned char sr_s[4];

    if (fread(sr_s, 1, 4, fh) != 4) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "I/Q file ver. 1 sample rate read error");

        return NULL;
    }

    const uint32_t sr = lrpt_utils_ds_uint32_t(sr_s);

    /* File position = 12 */

    /* Read in device name length */
    uint8_t name_l;

    if (fread(&name_l, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "I/Q file ver. 1 device name length read error");

        return NULL;
    }

    /* File position = 13 */

    /* Read device name info */
    char *name = NULL;

    /* Allocate storage for device name only if it's presented in I/Q file */
    if (name_l > 0) {
        name = calloc(name_l + 1, sizeof(char));

        if (!name || (fread(name, 1, name_l, fh) != name_l)) {
            free(name);
            fclose(fh);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                        "I/Q file ver. 1 device name allocation/read error");

            return NULL;
        }
    }

    /* File position = 13 + name_l */

    /* Read data length */
    unsigned char data_l_s[8];

    if (fread(data_l_s, 1, 8, fh) != 8) {
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "I/Q file ver. 1 data length read error");

        return NULL;
    }

    const uint64_t data_l = lrpt_utils_ds_uint64_t(data_l_s);

    /* File position = 21 + name_l */

    /* Perform sanity checking - one complex I/Q sample is encoded as two doubles and each double
     * is serialized to the 10 unsigned chars
     */
    const uint64_t cur_pos = ftell(fh);
    fseek(fh, 0, SEEK_END);
    const uint64_t n_iq = ((ftell(fh) - cur_pos) / UTILS_COMPLEX_SER_SIZE);
    const uint8_t bytes_rem = ((ftell(fh) - cur_pos) % UTILS_COMPLEX_SER_SIZE);
    fseek(fh, cur_pos, SEEK_SET);

    if ((bytes_rem > 0) && err)
        lrpt_error_set(err, LRPT_ERR_LVL_WARN, LRPT_ERR_CODE_FILECORR,
                "I/Q file contains not whole number of samples");

    if (n_iq != data_l) {
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FILECORR,
                    "Actual number of I/Q samples in file differs from internally stored value");

        return NULL;
    }

    /* Try to allocate temporary I/O buffer */
    unsigned char *iobuf =
        calloc(IO_IQ_DATA_N * UTILS_COMPLEX_SER_SIZE, sizeof(unsigned char));

    if (!iobuf) {
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Temporary I/O buffer allocation failed");

        return NULL;
    }

    /* Create I/Q data file object and return it */
    lrpt_iq_file_t *file = malloc(sizeof(lrpt_iq_file_t));

    if (!file) {
        free(iobuf);
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/Q data file object allocation failed");

        return NULL;
    }

    file->fhandle = fh;
    file->write_mode = false;
    file->version = LRPT_IQ_FILE_VER_1;
    file->flags = flags;
    file->samplerate = sr;
    file->device_name = name;
    file->header_len = 21 + name_l; /* Just a sum of all elements previously read */
    file->data_len = data_l;
    file->current = 0;
    file->iobuf = iobuf;

    return file;
}

/*************************************************************************************************/

/* lrpt_iq_file_open_r() */
lrpt_iq_file_t *lrpt_iq_file_open_r(
        const char *fname,
        lrpt_error_t *err) {
    if (!fname || (strlen(fname) == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "File name is NULL or empty");

        return NULL;
    }

    FILE *fh = fopen(fname, "rb");

    if (!fh) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open I/Q file for reading");

        return NULL;
    }

    /* File position = 0 */

    /* Check file header information. Header should be 6-character string "lrptiq" */
    char header[6];

    if ((fread(header, 1, 6, fh) != 6) || strncmp(header, "lrptiq", 6) != 0) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "I/Q file identifier read error");

        return NULL;
    }

    /* File position = 6 */

    /* Read file format version info */
    uint8_t ver;

    if (fread(&ver, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "I/Q file version read error");

        return NULL;
    }

    /* File position = 7 */

    switch (ver) {
        case LRPT_IQ_FILE_VER_1:
            return iq_file_open_r_v1(fh, err);

            break;

        default:
            fclose(fh);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_UNSUPP,
                        "Unsupported I/Q file version");

            return NULL;
    }
}

/*************************************************************************************************/

/* lrpt_iq_file_open_w_v1() */
lrpt_iq_file_t *lrpt_iq_file_open_w_v1(
        const char *fname,
        bool offset,
        uint32_t samplerate,
        const char *device_name,
        lrpt_error_t *err) {
    if (!fname || (strlen(fname) == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "File name is NULL or empty");

        return NULL;
    }

    FILE *fh = fopen(fname, "wb");

    if (!fh) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open I/Q file for writing");

        return NULL;
    }

    /* File position = 0 */

    const uint8_t version = LRPT_IQ_FILE_VER_1;

    /* Write file header and version */
    if (fwrite("lrptiq", 1, 6, fh) != 6) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file identifier write error");

        return NULL;
    }

    /* File position = 6 */

    if (fwrite(&version, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file version write error");

        return NULL;
    }

    /* File position = 7 */

    /* Write flags */
    unsigned char flags = 0;

    if (offset) /* Offset QPSK */
        flags |= 0x01;

    if (fwrite(&flags, 1, 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file ver. 1 flags write error");

        return NULL;
    }

    /* File position = 8 */

    /* Write sampling rate info */
    unsigned char sr_s[4];
    lrpt_utils_s_uint32_t(samplerate, sr_s);

    if (fwrite(sr_s, 1, 4, fh) != 4) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file ver. 1 sample rate write error");

        return NULL;
    }

    /* File position = 12 */

    /* Write device name */
    uint8_t name_l = 0;

    /* We'll write it only if user has requested it */
    if (device_name)
        name_l = strnlen(device_name, 255);

    if (fwrite(&name_l, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file ver. 1 device name length write error");

        return NULL;
    }

    /* File position = 13 */

    char *name = NULL;

    if (name_l > 0) {
        name = calloc(name_l + 1, sizeof(char));

        if (!name || (fwrite(device_name, 1, name_l, fh) != name_l)) {
            free(name);
            fclose(fh);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                        "I/Q file ver. 1 device name allocation/write error");

            return NULL;
        }

        strncpy(name, device_name, name_l);
    }

    /* File position = 13 + name_l */

    /* Write initial data length */
    unsigned char data_l_s[8];
    lrpt_utils_s_uint64_t(0, data_l_s);

    if (fwrite(data_l_s, 1, 8, fh) != 8) {
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file ver. 1 data length write error");

        return NULL;
    }

    /* File position = 21 + name_l */

    /* Try to allocate temporary I/O buffer */
    unsigned char *iobuf = calloc(IO_IQ_DATA_N * UTILS_COMPLEX_SER_SIZE, sizeof(unsigned char));

    if (!iobuf) {
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "Temporary I/O buffer allocation failed");

        return NULL;
    }

    /* Create I/Q data file object and return it */
    lrpt_iq_file_t *file = malloc(sizeof(lrpt_iq_file_t));

    if (!file) {
        free(iobuf);
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/Q data file object allocation failed");

        return NULL;
    }

    file->fhandle = fh;
    file->write_mode = true;
    file->version = LRPT_IQ_FILE_VER_1;
    file->flags = flags;
    file->samplerate = samplerate;
    file->device_name = name;
    file->header_len = 21 + name_l; /* Just a sum of all elements previously written */
    file->data_len = 0;
    file->current = 0;
    file->iobuf = iobuf;

    return file;
}

/*************************************************************************************************/

/* lrpt_iq_file_close() */
void lrpt_iq_file_close(
        lrpt_iq_file_t *file) {
    if (!file)
        return;

    free(file->iobuf);
    free(file->device_name);

    fclose(file->fhandle);

    free(file);
}

/*************************************************************************************************/

/* lrpt_iq_file_version() */
uint8_t lrpt_iq_file_version(
        const lrpt_iq_file_t *file) {
    if (!file)
        return 0;

    return file->version;
}

/*************************************************************************************************/

/* lrpt_iq_file_is_offsetted() */
bool lrpt_iq_file_is_offsetted(
        const lrpt_iq_file_t *file) {
    if (!file)
        return false;

    return (file->flags & 0x01);
}

/*************************************************************************************************/

/* lrpt_iq_file_samplerate() */
uint32_t lrpt_iq_file_samplerate(
        const lrpt_iq_file_t *file) {
    if (!file)
        return 0;

    return file->samplerate;
}

/*************************************************************************************************/

/* lrpt_iq_file_devicename() */
const char *lrpt_iq_file_devicename(
        const lrpt_iq_file_t *file) {
    if (!file)
        return NULL;

    return file->device_name;
}

/*************************************************************************************************/

/* lrpt_iq_file_length() */
uint64_t lrpt_iq_file_length(
        const lrpt_iq_file_t *file) {
    if (!file)
        return 0;

    return file->data_len;
}

/*************************************************************************************************/

/* lrpt_iq_file_goto() */
bool lrpt_iq_file_goto(
        lrpt_iq_file_t *file,
        uint64_t sample,
        lrpt_error_t *err) {
    if (!file || (sample > file->data_len)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q file object is NULL or sample index exceeds data length");

        return false;
    }

    if (file->version == LRPT_IQ_FILE_VER_1) {
        if (fseek(file->fhandle, file->header_len + sample * UTILS_COMPLEX_SER_SIZE, SEEK_SET) == 0)
            file->current = sample;
        else {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FSEEK,
                        "Error during seeking in I/Q file");

            return false;
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_read_from_file() */
bool lrpt_iq_data_read_from_file(
        lrpt_iq_data_t *data,
        lrpt_iq_file_t *file,
        size_t len,
        bool rewind,
        lrpt_error_t *err) {
    /* Sanity checks */
    if (!data || !file || file->write_mode) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object and/or file pointer are NULL or incorrect mode is used");

        return false;
    }

    /* Check if we have enough data to read */
    if (file->current == file->data_len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_WARN, LRPT_ERR_CODE_EOF,
                    "Reached EOF");

        return false;
    }

    /* Read up to the end if requested length is bigger than remaining data */
    if ((file->current + len) > file->data_len)
        len = (file->data_len - file->current);

    /* Resize storage */
    if (!lrpt_iq_data_resize(data, len, err))
        return false;

    if (file->version == LRPT_IQ_FILE_VER_1) { /* Version 1 */
        /* Determine required number of block reads */
        const size_t nreads = (len / IO_IQ_DATA_N);

        for (size_t i = 0; i <= nreads; i++) {
            const size_t toread = (i == nreads) ? (len - nreads * IO_IQ_DATA_N) : IO_IQ_DATA_N;

            if (toread == 0)
                break;

            /* Read block */
            if (fread(file->iobuf, UTILS_COMPLEX_SER_SIZE, toread, file->fhandle) != toread) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                            "Error during block read from I/Q file");

                return false;
            }

            /* Parse block */
            for (size_t j = 0; j < toread; j++) {
                unsigned char v_s[10];
                double i_part, q_part;

                memcpy(v_s,
                        file->iobuf + UTILS_COMPLEX_SER_SIZE * j,
                        sizeof(unsigned char) * UTILS_DOUBLE_SER_SIZE); /* I sample */

                if (!lrpt_utils_ds_double(v_s, &i_part)) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                                "Can't deserialize double value");

                    return false;
                }

                memcpy(v_s,
                        file->iobuf + UTILS_COMPLEX_SER_SIZE * j + UTILS_DOUBLE_SER_SIZE,
                        sizeof(unsigned char) * UTILS_DOUBLE_SER_SIZE); /* Q sample */

                if (!lrpt_utils_ds_double(v_s, &q_part)) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                                "Can't deserialize double value");

                    return false;
                }

                data->iq[i * IO_IQ_DATA_N + j] = i_part + q_part * I;
            }
        }
    }

    if (rewind) {
        if (!lrpt_iq_file_goto(file, file->current, err))
            return false;
    }
    else
        file->current += len;

    return true;
}

/*************************************************************************************************/

/* lrpt_iq_data_write_to_file() */
bool lrpt_iq_data_write_to_file(
        const lrpt_iq_data_t *data,
        lrpt_iq_file_t *file,
        bool inplace,
        lrpt_error_t *err) {
    if (!data || (data->len == 0) || !data->iq || !file || !file->write_mode) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q data object and/or file pointer are NULL, incorrect mode is used or no data to write");

        return false;
    }

    if (file->version == LRPT_IQ_FILE_VER_1) { /* Version 1 */
        /* Determine required number of block writes */
        const size_t len = data->len;
        const size_t nwrites = (len / IO_IQ_DATA_N);

        for (size_t i = 0; i <= nwrites; i++) {
            const size_t towrite = (i == nwrites) ? (len - nwrites * IO_IQ_DATA_N) : IO_IQ_DATA_N;

            if (towrite == 0)
                break;

            /* Prepare block */
            for (size_t j = 0; j < towrite; j++) {
                unsigned char v_s[10];

                if (!lrpt_utils_s_double(creal(data->iq[i * IO_IQ_DATA_N + j]), v_s)) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                                "Can't serialize double value");

                    return false;
                }

                memcpy(file->iobuf + UTILS_COMPLEX_SER_SIZE * j,
                        v_s,
                        sizeof(unsigned char) * UTILS_DOUBLE_SER_SIZE); /* I sample */

                if (!lrpt_utils_s_double(cimag(data->iq[i * IO_IQ_DATA_N + j]), v_s)) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                                "Can't serialize double value");

                    return false;
                }

                memcpy(file->iobuf + UTILS_COMPLEX_SER_SIZE * j + UTILS_DOUBLE_SER_SIZE,
                        v_s,
                        sizeof(unsigned char) * UTILS_DOUBLE_SER_SIZE); /* Q sample */
            }

            /* Write block */
            if (fwrite(file->iobuf, 1, towrite * UTILS_COMPLEX_SER_SIZE, file->fhandle) !=
                    (towrite * UTILS_COMPLEX_SER_SIZE)) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                            "Error during block write to I/Q file");

                return false;
            }

            file->current += towrite;
            file->data_len += towrite;

            /* Flush data length if inplace is requested */
            if (inplace) {
                unsigned char v_s[8];

                lrpt_utils_s_uint64_t(file->data_len, v_s);
                fseek(file->fhandle, file->header_len - 8, SEEK_SET);

                if (fwrite(v_s, 1, 8, file->fhandle) != 8) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                                "I/Q data length write error");

                    return false;
                }

                if (!lrpt_iq_file_goto(file, file->current, err))
                    return false;
            }
        }

        /* Flush data length if inplace isn't requested */
        if (!inplace) {
            unsigned char v_s[8];

            lrpt_utils_s_uint64_t(file->data_len, v_s);
            fseek(file->fhandle, file->header_len - 8, SEEK_SET);

            if (fwrite(v_s, 1, 8, file->fhandle) != 8) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                            "I/Q data length write error");

                return false;
            }

            if (!lrpt_iq_file_goto(file, file->current, err))
                return false;
        }
    }

    return true;
}

/*************************************************************************************************/

/* qpsk_file_open_r_v1() */
static lrpt_qpsk_file_t *qpsk_file_open_r_v1(
        FILE *fh,
        lrpt_error_t *err) {
    /* File position = 9 */

    /* Read flags */
    unsigned char flags;

    if (fread(&flags, 1, 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "QPSK file ver. 1 flags read error");

        return NULL;
    }

    /* File position = 10 */

    /* Read symbol rate */
    unsigned char sr_s[4];

    if (fread(sr_s, 1, 4, fh) != 4) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "QPSK file ver. 1 symbol rate read error");

        return NULL;
    }

    const uint32_t sr = lrpt_utils_ds_uint32_t(sr_s);

    /* File position = 14 */

    /* Read data length */
    unsigned char data_l_s[8];

    if (fread(data_l_s, 1, 8, fh) != 8) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "QPSK file ver. 1 data length read error");

        return NULL;
    }

    const uint64_t data_l = lrpt_utils_ds_uint64_t(data_l_s);

    /* File position = 22 */

    /* Perform sanity checking - one QPSK byte is encoded as one int8_t */
    /* TODO add code for hard symbols */
    const uint64_t cur_pos = ftell(fh);
    fseek(fh, 0, SEEK_END);
    const uint64_t n_sym = (ftell(fh) - cur_pos);
    fseek(fh, cur_pos, SEEK_SET);

    if (n_sym != data_l) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FILECORR,
                    "Actual number of QPSK symbols in file differs from internally stored value");

        return NULL;
    }

    /* Create QPSK data file object and return it */
    lrpt_qpsk_file_t *file = malloc(sizeof(lrpt_qpsk_file_t));

    if (!file) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "QPSK data file object allocation failed");

        return NULL;
    }

    file->fhandle = fh;
    file->write_mode = false;
    file->version = LRPT_QPSK_FILE_VER_1;
    file->flags = flags;
    file->symrate = sr;
    file->header_len = 22; /* Just a sum of all elements previously read */
    file->data_len = data_l; /* TODO add code for hard symbols */
    file->current = 0;

    return file;
}

/*************************************************************************************************/

/* lrpt_qpsk_file_open_r() */
lrpt_qpsk_file_t *lrpt_qpsk_file_open_r(
        const char *fname,
        lrpt_error_t *err) {
    if (!fname || (strlen(fname) == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "File name is NULL or empty");

        return NULL;
    }

    FILE *fh = fopen(fname, "rb");

    if (!fh) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open QPSK file for reading");

        return NULL;
    }

    /* File position = 0 */

    /* Check file header information. Header should be 8-character string "lrptqpsk" */
    char header[8];

    if ((fread(header, 1, 8, fh) != 8) || strncmp(header, "lrptqpsk", 8) != 0) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "QPSK file identifier read error");

        return NULL;
    }

    /* File position = 8 */

    /* Read file format version info */
    uint8_t ver;

    if (fread(&ver, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "QPSK file version read error");

        return NULL;
    }

    /* File position = 9 */

    switch (ver) {
        case LRPT_QPSK_FILE_VER_1:
            return qpsk_file_open_r_v1(fh, err);

            break;

        default:
            fclose(fh);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_UNSUPP,
                        "Unsupported QPSK file version");

            return NULL;
    }
}

/*************************************************************************************************/

/* lrpt_qpsk_file_open_w_v1() */
lrpt_qpsk_file_t *lrpt_qpsk_file_open_w_v1(
        const char *fname,
        bool differential,
        bool interleaved,
        bool hard,
        uint32_t symrate,
        lrpt_error_t *err) {
    if (!fname || (strlen(fname) == 0)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "File name is NULL or empty");

        return NULL;
    }

    FILE *fh = fopen(fname, "wb");

    if (!fh) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FOPEN,
                    "Can't open QPSK file for writing");

        return NULL;
    }

    /* File position = 0 */

    const uint8_t version = LRPT_QPSK_FILE_VER_1;

    /* Write file header and version */
    if (fwrite("lrptqpsk", 1, 8, fh) != 8) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file identifier write error");

        return NULL;
    }

    /* File position = 8 */

    if (fwrite(&version, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file version write error");

        return NULL;
    }

    /* File position = 9 */

    /* Write flags */
    unsigned char flags = 0;

    if (differential) /* Diffcoded QPSK */
        flags |= 0x01;

    if (interleaved) /* Interleaved QPSK */
        flags |= 0x02;

    if (hard) /* Hard symbols */
        flags |= 0x04;

    if (fwrite(&flags, 1, 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file ver. 1 flags write error");

        return NULL;
    }

    /* File position = 10 */

    /* Write symbol rate info */
    unsigned char sr_s[4];
    lrpt_utils_s_uint32_t(symrate, sr_s);

    if (fwrite(sr_s, 1, 4, fh) != 4) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file ver. 1 symbol rate write error");

        return NULL;
    }

    /* File position = 14 */

    /* Write initial data length */
    unsigned char data_l_s[8];
    lrpt_utils_s_uint64_t(0, data_l_s);

    if (fwrite(data_l_s, 1, 8, fh) != 8) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file ver. 1 data length write error");

        return NULL;
    }

    /* File position = 22 */

    /* Create QPSK data file object and return it */
    lrpt_qpsk_file_t *file = malloc(sizeof(lrpt_qpsk_file_t));

    if (!file) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "QPSK data file object allocation failed");

        return NULL;
    }

    file->fhandle = fh;
    file->write_mode = true;
    file->version = LRPT_QPSK_FILE_VER_1;
    file->flags = flags;
    file->symrate = symrate;
    file->header_len = 22; /* Just a sum of all elements previously written */
    file->data_len = 0; /* TODO add code for hard symbols */
    file->current = 0;

    return file;
}

/*************************************************************************************************/

/* lrpt_qpsk_file_close() */
void lrpt_qpsk_file_close(
        lrpt_qpsk_file_t *file) {
    if (!file)
        return;

    fclose(file->fhandle);

    free(file);
}

/*************************************************************************************************/

/* lrpt_qpsk_file_version() */
uint8_t lrpt_qpsk_file_version(
        const lrpt_qpsk_file_t *file) {
    if (!file)
        return 0;

    return file->version;
}

/*************************************************************************************************/

/* lrpt_qpsk_file_is_diffcoded() */
bool lrpt_qpsk_file_is_diffcoded(
        const lrpt_qpsk_file_t *file) {
    if (!file)
        return false;

    return (file->flags & 0x01);
}

/*************************************************************************************************/

/* lrpt_qpsk_file_is_interleaved() */
bool lrpt_qpsk_file_is_interleaved(
        const lrpt_qpsk_file_t *file) {
    if (!file)
        return false;

    return (file->flags & 0x02);
}

/*************************************************************************************************/

/* lrpt_qpsk_file_is_hardsymboled() */
bool lrpt_qpsk_file_is_hardsymboled(
        const lrpt_qpsk_file_t *file) {
    if (!file)
        return false;

    return (file->flags & 0x04);
}

/*************************************************************************************************/

/* lrpt_qpsk_file_symrate() */
uint32_t lrpt_qpsk_file_symrate(
        const lrpt_qpsk_file_t *file) {
    if (!file)
        return 0;

    return file->symrate;
}

/*************************************************************************************************/

/* lrpt_qpsk_file_length() */
uint64_t lrpt_qpsk_file_length(
        const lrpt_qpsk_file_t *file) {
    if (!file)
        return 0;

    return file->data_len; /* TODO add code for hard symbols */
}

/*************************************************************************************************/

/* lrpt_qpsk_file_goto() */
bool lrpt_qpsk_file_goto(
        lrpt_qpsk_file_t *file,
        uint64_t symbol,
        lrpt_error_t *err) {
    if (!file || (symbol > file->data_len)) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK file object is NULL or symbol index exceeds data length");

        return false;
    }

    /* TODO add code for hard symbols */
    if (file->version == LRPT_QPSK_FILE_VER_1) {
        if (fseek(file->fhandle, file->header_len + symbol, SEEK_SET) == 0)
            file->current = symbol;
        else {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FSEEK,
                        "Error during seeking in QPSK file");

            return false;
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_read_from_file() */
bool lrpt_qpsk_data_read_from_file(
        lrpt_qpsk_data_t *data,
        lrpt_qpsk_file_t *file,
        size_t len,
        bool rewind,
        lrpt_error_t *err) {
    /* Sanity checks */
    if (!data || !file || file->write_mode) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object and/or file pointer are NULL or incorrect mode is used");

        return false;
    }

    /* Check if we have enough data to read */
    if (file->current == file->data_len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_WARN, LRPT_ERR_CODE_EOF,
                    "Reached EOF");

        return false;
    }

    /* Read up to the end if requested length is bigger than remaining data */
    if ((file->current + len) > file->data_len)
        len = (file->data_len - file->current);

    /* Resize storage */
    if (!lrpt_qpsk_data_resize(data, len, err))
        return false;

    if (file->version == LRPT_QPSK_FILE_VER_1) { /* Version 1 */
        /* Determine required number of block reads */
        const size_t nreads = (len / IO_QPSK_DATA_N);

        for (size_t i = 0; i <= nreads; i++) {
            const size_t toread = (i == nreads) ? (len - nreads * IO_QPSK_DATA_N) : IO_QPSK_DATA_N;

            if (toread == 0)
                break;

            /* Read block */
            if (fread(data->qpsk + i * IO_QPSK_DATA_N, 1, toread, file->fhandle) != toread) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                            "Error during block read from QPSK file");

                return false;
            }
        }
    }

    if (rewind) {
        if (!lrpt_qpsk_file_goto(file, file->current, err))
            return false;
    }
    else
        file->current += len;

    return true;
}

/*************************************************************************************************/

/* lrpt_qpsk_data_write_to_file() */
bool lrpt_qpsk_data_write_to_file(
        const lrpt_qpsk_data_t *data,
        lrpt_qpsk_file_t *file,
        bool inplace,
        lrpt_error_t *err) {
    if (!data || (data->len == 0) || !data->qpsk || !file || !file->write_mode) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK data object and/or file pointer are NULL, incorrect mode is used or no data to write");

        return false;
    }

    if (file->version == LRPT_QPSK_FILE_VER_1) { /* Version 1 */
        /* Determine required number of block writes */
        const size_t len = data->len;
        const size_t nwrites = (len / IO_QPSK_DATA_N);

        for (size_t i = 0; i <= nwrites; i++) {
            const size_t towrite = (i == nwrites) ? (len - nwrites * IO_QPSK_DATA_N) : IO_QPSK_DATA_N;

            if (towrite == 0)
                break;

            /* Write block */
            if (fwrite(data->qpsk + i * IO_QPSK_DATA_N, 1, towrite, file->fhandle) != towrite) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                            "Error during block write to QPSK file");

                return false;
            }

            file->current += towrite;
            file->data_len += towrite;

            /* Flush data length if inplace is requested */
            if (inplace) {
                unsigned char v_s[8];

                lrpt_utils_s_uint64_t(file->data_len, v_s);
                fseek(file->fhandle, file->header_len - 8, SEEK_SET);

                if (fwrite(v_s, 1, 8, file->fhandle) != 8) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                                "QPSK data length write error");

                    return false;
                }

                if (!lrpt_qpsk_file_goto(file, file->current, err))
                    return false;
            }
        }

        /* Flush data length if inplace isn't requested */
        if (!inplace) {
            unsigned char v_s[8];

            lrpt_utils_s_uint64_t(file->data_len, v_s);
            fseek(file->fhandle, file->header_len - 8, SEEK_SET);

            if (fwrite(v_s, 1, 8, file->fhandle) != 8) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                            "QPSK data length write error");

                return false;
            }

            if (!lrpt_qpsk_file_goto(file, file->current, err))
                return false;
        }
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
