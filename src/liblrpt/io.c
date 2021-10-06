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
#include "datatype.h"
#include "error.h"
#include "utils.h"

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/** Block size for I/O operations with I/Q data (in number of samples) */
static const size_t IO_IQ_DATA_N = 1024;

/** Block size for I/O operations with I/Q data (in number of symbols; must be a multiple of 4) */
static const size_t IO_QPSK_DATA_N = 1024;

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
    uint64_t hl = 7; /* Header length */

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

    hl += 1;

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

    const uint32_t sr = lrpt_utils_ds_uint32_t(sr_s, true);
    hl += 4;

    /* File position = 12 */

    /* Read bandwidth */
    unsigned char bw_s[4];

    if (fread(bw_s, 1, 4, fh) != 4) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "I/Q file ver. 1 bandwidth read error");

        return NULL;
    }

    const uint32_t bw = lrpt_utils_ds_uint32_t(bw_s, true);
    hl += 4;

    /* File position = 16 */

    /* Read in device name length */
    uint8_t name_l;

    if (fread(&name_l, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                    "I/Q file ver. 1 device name length read error");

        return NULL;
    }

    /* File position = 17 */
    hl += 1;

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

    hl += name_l;

    /* File position = 17 + name_l */

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

    const uint64_t data_l = lrpt_utils_ds_uint64_t(data_l_s, true);
    hl += 8;

    /* File position = 25 + name_l */

    /* Perform sanity checking - one complex I/Q sample is encoded as two doubles and each double
     * is serialized to the 10 unsigned chars
     */
    const uint64_t cur_pos = ftell(fh);
    fseek(fh, 0, SEEK_END);
    const uint64_t n_iq = ((ftell(fh) - cur_pos) / UTILS_COMPLEX_SER_SIZE);
    const uint8_t bytes_rem = ((ftell(fh) - cur_pos) % UTILS_COMPLEX_SER_SIZE);
    fseek(fh, cur_pos, SEEK_SET);

    if (n_iq != data_l) { /* Incorrect number of samples in I/Q file */
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FILECORR,
                    "Actual number of I/Q samples in file differs from internally stored value");

        return NULL;
    }
    else if ((bytes_rem > 0) && err) /* Extra bytes are possible */
        lrpt_error_set(err, LRPT_ERR_LVL_WARN, LRPT_ERR_CODE_FILECORR,
                "I/Q file contains not whole number of samples");

    /* Try to allocate temporary I/O buffer */
    unsigned char *iobuf = calloc(IO_IQ_DATA_N * UTILS_COMPLEX_SER_SIZE, sizeof(unsigned char));

    if (!iobuf) {
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/O buffer allocation failed");

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
    file->version = LRPT_IQ_FILE_VER1;
    file->flags = flags;
    file->samplerate = sr;
    file->bandwidth = bw;
    file->device_name = name;
    file->header_len = hl;
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
        case LRPT_IQ_FILE_VER1:
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
        uint32_t bandwidth,
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

    uint64_t hl = 0; /* Header length */

    /* File position = 0 */

    const uint8_t version = LRPT_IQ_FILE_VER1;

    /* Write file header and version */
    if (fwrite("lrptiq", 1, 6, fh) != 6) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file identifier write error");

        return NULL;
    }

    hl += 6;

    /* File position = 6 */

    if (fwrite(&version, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file version write error");

        return NULL;
    }

    hl += 1;

    /* File position = 7 */

    /* Write flags */
    unsigned char flags = 0;

    if (offset) /* Offset QPSK */
        flags |= LRPT_IQ_FILE_FLAGS_VER1_OFFSET;

    if (fwrite(&flags, 1, 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file ver. 1 flags write error");

        return NULL;
    }

    hl += 1;

    /* File position = 8 */

    /* Write sampling rate info */
    unsigned char sr_s[4];
    lrpt_utils_s_uint32_t(samplerate, sr_s, true);

    if (fwrite(sr_s, 1, 4, fh) != 4) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file ver. 1 sample rate write error");

        return NULL;
    }

    hl += 4;

    /* File position = 12 */

    /* Write bandwidth info */
    unsigned char bw_s[4];
    lrpt_utils_s_uint32_t(bandwidth, bw_s, true);

    if (fwrite(bw_s, 1, 4, fh) != 4) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file ver. 1 bandwidth write error");

        return NULL;
    }

    hl += 4;

    /* File position = 16 */

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

    hl += 1;

    /* File position = 17 */

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

    hl += name_l;

    /* File position = 17 + name_l */

    /* Write initial data length */
    unsigned char data_l_s[8];
    lrpt_utils_s_uint64_t(0, data_l_s, true);

    if (fwrite(data_l_s, 1, 8, fh) != 8) {
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "I/Q file ver. 1 data length write error");

        return NULL;
    }

    hl += 8;

    /* File position = 25 + name_l */

    /* Try to allocate temporary I/O buffer */
    unsigned char *iobuf = calloc(IO_IQ_DATA_N * UTILS_COMPLEX_SER_SIZE, sizeof(unsigned char));

    if (!iobuf) {
        free(name);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/O buffer allocation failed");

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
    file->version = LRPT_IQ_FILE_VER1;
    file->flags = flags;
    file->samplerate = samplerate;
    file->device_name = name;
    file->header_len = hl;
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

    return (file->flags & LRPT_IQ_FILE_FLAGS_VER1_OFFSET);
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

/* lrpt_iq_file_bandwidth() */
uint32_t lrpt_iq_file_bandwidth(
        const lrpt_iq_file_t *file) {
    if (!file)
        return 0;

    return file->bandwidth;
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
    if (!file) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "I/Q file object is NULL");

        return false;
    }

    if (file->write_mode) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FSEEK,
                    "Can't goto in write mode");

        return false;
    }

    if (sample > file->data_len)
        sample = file->data_len;

    if (file->version == LRPT_IQ_FILE_VER1) {
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

    /* In case of empty read just exit */
    if (len == 0)
        return true;

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

    if (file->version == LRPT_IQ_FILE_VER1) { /* Version 1 */
        /* Determine required number of reads */
        const size_t n_reads = (len / IO_IQ_DATA_N);

        for (size_t i = 0; i <= n_reads; i++) {
            /* Determine required number of samples for current read */
            const size_t toread = (i == n_reads) ? (len - n_reads * IO_IQ_DATA_N) : IO_IQ_DATA_N;

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

                if (!lrpt_utils_ds_double(v_s, &i_part, true)) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                                "Can't deserialize double value");

                    return false;
                }

                memcpy(v_s,
                        file->iobuf + UTILS_COMPLEX_SER_SIZE * j + UTILS_DOUBLE_SER_SIZE,
                        sizeof(unsigned char) * UTILS_DOUBLE_SER_SIZE); /* Q sample */

                if (!lrpt_utils_ds_double(v_s, &q_part, true)) {
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

/* TODO implement custom length writing (perhaps with start offset) */
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

    /* In case of empty write just exit */
    if (data->len == 0)
        return true;

    if (file->version == LRPT_IQ_FILE_VER1) { /* Version 1 */
        /* Determine required number of writes */
        const size_t len = data->len;
        const size_t n_writes = (len / IO_IQ_DATA_N);

        for (size_t i = 0; i <= n_writes; i++) {
            const size_t towrite = (i == n_writes) ? (len - n_writes * IO_IQ_DATA_N) : IO_IQ_DATA_N;

            if (towrite == 0)
                break;

            /* Prepare block */
            for (size_t j = 0; j < towrite; j++) {
                unsigned char v_s[10];

                /* I */
                if (!lrpt_utils_s_double(creal(data->iq[i * IO_IQ_DATA_N + j]), v_s, true)) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                                "Can't serialize double value");

                    return false;
                }

                memcpy(file->iobuf + UTILS_COMPLEX_SER_SIZE * j,
                        v_s,
                        sizeof(unsigned char) * UTILS_DOUBLE_SER_SIZE);

                /* Q */
                if (!lrpt_utils_s_double(cimag(data->iq[i * IO_IQ_DATA_N + j]), v_s, true)) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                                "Can't serialize double value");

                    return false;
                }

                memcpy(file->iobuf + UTILS_COMPLEX_SER_SIZE * j + UTILS_DOUBLE_SER_SIZE,
                        v_s,
                        sizeof(unsigned char) * UTILS_DOUBLE_SER_SIZE);
            }

            /* Write block */
            if (fwrite(file->iobuf, UTILS_COMPLEX_SER_SIZE, towrite, file->fhandle) != towrite) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                            "Error during block write to I/Q file");

                return false;
            }

            /* Update data pointers and counters */
            file->current += towrite;
            file->data_len += towrite;

            /* Flush data length if inplace is requested */
            if (inplace) {
                unsigned char v_s[8];

                lrpt_utils_s_uint64_t(file->data_len, v_s, true);
                fseek(file->fhandle, file->header_len - 8, SEEK_SET);

                if (fwrite(v_s, 1, 8, file->fhandle) != 8) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                                "I/Q data length write error");

                    return false;
                }

                fseek(
                        file->fhandle,
                        file->header_len + file->current * UTILS_COMPLEX_SER_SIZE,
                        SEEK_SET);
            }
        }

        /* Flush data length if inplace isn't requested */
        if (!inplace) {
            unsigned char v_s[8];

            lrpt_utils_s_uint64_t(file->data_len, v_s, true);
            fseek(file->fhandle, file->header_len - 8, SEEK_SET);

            if (fwrite(v_s, 1, 8, file->fhandle) != 8) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                            "I/Q data length write error");

                return false;
            }

            fseek(
                    file->fhandle,
                    file->header_len + file->current * UTILS_COMPLEX_SER_SIZE,
                    SEEK_SET);
        }
    }

    return true;
}

/*************************************************************************************************/

/* qpsk_file_open_r_v1() */
static lrpt_qpsk_file_t *qpsk_file_open_r_v1(
        FILE *fh,
        lrpt_error_t *err) {
    uint64_t hl = 9; /* Header length */

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

    hl += 1;

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

    const uint32_t sr = lrpt_utils_ds_uint32_t(sr_s, true);
    hl += 4;

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

    const uint64_t data_l = lrpt_utils_ds_uint64_t(data_l_s, true);
    hl += 8;

    /* File position = 22 */

    /* Perform sanity checking - one QPSK byte is encoded as one unsigned char; one QPSK soft symbol
     * consists of two QPSK bytes casted to int8_t; four QPSK hard symbols make one QPSK byte
     */
    const uint64_t cur_pos = ftell(fh);
    fseek(fh, 0, SEEK_END);
    const uint64_t n_bytes = (ftell(fh) - cur_pos);
    fseek(fh, cur_pos, SEEK_SET);

    if (flags & LRPT_QPSK_FILE_FLAGS_VER1_HARDSYMBOLED) { /* Checks for hard-symboled file */
        const uint64_t n_needed = (data_l == 0) ? 0 : ((data_l - 1) / 4 + 1);

        if (n_bytes != n_needed) { /* Incorrect number of bytes in QPSK file */
            fclose(fh);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FILECORR,
                        "Actual number of QPSK symbols in file differs from internally stored value");

            return NULL;
        }
        else if (((data_l % 4) != 0) && err) /* Extra symbols are possible */
            lrpt_error_set(err, LRPT_ERR_LVL_WARN, LRPT_ERR_CODE_FILECORR,
                    "QPSK file may contain extra symbols");
    }
    else { /* Checks for soft-symboled file */
        if ((n_bytes / 2) != data_l) { /* Incorrect number of bytes in QPSK file */
            fclose(fh);

            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FILECORR,
                        "Actual number of QPSK symbols in file differs from internally stored value");

            return NULL;
        }
        else if (((n_bytes % 2) != 0) && err)
            lrpt_error_set(err, LRPT_ERR_LVL_WARN, LRPT_ERR_CODE_FILECORR,
                    "QPSK file contains not whole number of symbols");
    }

    /* Try to allocate temporary I/O buffer */
    const size_t bufsize = (flags & LRPT_QPSK_FILE_FLAGS_VER1_HARDSYMBOLED) ?
        (IO_QPSK_DATA_N / 4) :
        (IO_QPSK_DATA_N * 2);
    unsigned char *iobuf = calloc(bufsize, sizeof(unsigned char));

    if (!iobuf) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/O buffer allocation failed");

        return NULL;
    }

    /* Create QPSK data file object and return it */
    lrpt_qpsk_file_t *file = malloc(sizeof(lrpt_qpsk_file_t));

    if (!file) {
        free(iobuf);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "QPSK data file object allocation failed");

        return NULL;
    }

    file->fhandle = fh;
    file->write_mode = false;
    file->version = LRPT_QPSK_FILE_VER1;
    file->flags = flags;
    file->symrate = sr;
    file->header_len = hl;
    file->data_len = data_l;
    file->current = 0;
    file->iobuf = iobuf;

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
        case LRPT_QPSK_FILE_VER1:
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

    uint64_t hl = 0; /* Header length */

    /* File position = 0 */

    const uint8_t version = LRPT_QPSK_FILE_VER1;

    /* Write file header and version */
    if (fwrite("lrptqpsk", 1, 8, fh) != 8) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file identifier write error");

        return NULL;
    }

    hl += 8;

    /* File position = 8 */

    if (fwrite(&version, sizeof(uint8_t), 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file version write error");

        return NULL;
    }

    hl += 1;

    /* File position = 9 */

    /* Write flags */
    unsigned char flags = 0;

    if (differential) /* Diffcoded QPSK */
        flags |= LRPT_QPSK_FILE_FLAGS_VER1_DIFFCODED;

    if (interleaved) /* Interleaved QPSK */
        flags |= LRPT_QPSK_FILE_FLAGS_VER1_INTERLEAVED;

    if (hard) /* Hard symbols */
        flags |= LRPT_QPSK_FILE_FLAGS_VER1_HARDSYMBOLED;

    if (fwrite(&flags, 1, 1, fh) != 1) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file ver. 1 flags write error");

        return NULL;
    }

    hl += 1;

    /* File position = 10 */

    /* Write symbol rate info */
    unsigned char sr_s[4];
    lrpt_utils_s_uint32_t(symrate, sr_s, true);

    if (fwrite(sr_s, 1, 4, fh) != 4) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file ver. 1 symbol rate write error");

        return NULL;
    }

    hl += 4;

    /* File position = 14 */

    /* Write initial data length */
    unsigned char data_l_s[8];
    lrpt_utils_s_uint64_t(0, data_l_s, true);

    if (fwrite(data_l_s, 1, 8, fh) != 8) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                    "QPSK file ver. 1 data length write error");

        return NULL;
    }

    hl += 8;

    /* File position = 22 */

    /* Try to allocate temporary I/O buffer */
    const size_t bufsize = (flags & LRPT_QPSK_FILE_FLAGS_VER1_HARDSYMBOLED) ?
        (IO_QPSK_DATA_N / 4) :
        (IO_QPSK_DATA_N * 2);
    unsigned char *iobuf = calloc(bufsize, sizeof(unsigned char));

    if (!iobuf) {
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "I/O buffer allocation failed");

        return NULL;
    }

    /* Create QPSK data file object and return it */
    lrpt_qpsk_file_t *file = malloc(sizeof(lrpt_qpsk_file_t));

    if (!file) {
        free(iobuf);
        fclose(fh);

        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                    "QPSK data file object allocation failed");

        return NULL;
    }

    file->fhandle = fh;
    file->write_mode = true;
    file->version = LRPT_QPSK_FILE_VER1;
    file->flags = flags;
    file->symrate = symrate;
    file->header_len = hl;
    file->data_len = 0;
    file->current = 0;
    file->iobuf = iobuf;

    return file;
}

/*************************************************************************************************/

/* lrpt_qpsk_file_close() */
void lrpt_qpsk_file_close(
        lrpt_qpsk_file_t *file) {
    if (!file)
        return;

    free(file->iobuf);

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

    return (file->flags & LRPT_QPSK_FILE_FLAGS_VER1_DIFFCODED);
}

/*************************************************************************************************/

/* lrpt_qpsk_file_is_interleaved() */
bool lrpt_qpsk_file_is_interleaved(
        const lrpt_qpsk_file_t *file) {
    if (!file)
        return false;

    return (file->flags & LRPT_QPSK_FILE_FLAGS_VER1_INTERLEAVED);
}

/*************************************************************************************************/

/* lrpt_qpsk_file_is_hardsymboled() */
bool lrpt_qpsk_file_is_hardsymboled(
        const lrpt_qpsk_file_t *file) {
    if (!file)
        return false;

    return (file->flags & LRPT_QPSK_FILE_FLAGS_VER1_HARDSYMBOLED);
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

    return file->data_len;
}

/*************************************************************************************************/

/* lrpt_qpsk_file_goto() */
bool lrpt_qpsk_file_goto(
        lrpt_qpsk_file_t *file,
        uint64_t symbol,
        lrpt_error_t *err) {
    if (!file) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "QPSK file object is NULL");

        return false;
    }

    if (file->write_mode) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FSEEK,
                    "Can't goto in write mode");

        return false;
    }

    if (symbol > file->data_len)
        symbol = file->data_len;

    if (file->version == LRPT_QPSK_FILE_VER1) {
        const uint64_t offset =
            lrpt_qpsk_file_is_hardsymboled(file) ? (symbol / 4) : (2 * symbol);

        if (fseek(file->fhandle, file->header_len + offset, SEEK_SET) == 0)
            file->current = symbol;
        else {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FSEEK,
                        "Error during seeking in QPSK file");

            return false;
        }

        if (lrpt_qpsk_file_is_hardsymboled(file) && ((symbol % 4) != 0)) {
            if (fread(&file->last_hardsym, 1, 1, file->fhandle) != 1) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                            "Can't get last hard symbol");

                return false;
            }

            if (fseek(file->fhandle, -1, SEEK_CUR) != 0) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FSEEK,
                            "Error during seeking in QPSK file");

                return false;
            }
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

    /* In case of empty read just exit */
    if (len == 0)
        return true;

    /* Check if we have enough data to read */
    if (file->current == file->data_len) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_WARN, LRPT_ERR_CODE_EOF,
                    "Reached EOF");

        return false;
    }

    /* Partial index in current hard symbol byte */
    const uint8_t hardsym_off = (lrpt_qpsk_file_is_hardsymboled(file)) ? (file->current % 4) : 0;

    /* Remaining number of hard symbols to read */
    const uint8_t hardsym_rem = (hardsym_off == 0) ? 0 : (4 - hardsym_off);

    /* Read up to the end if requested length is bigger than remaining data */
    if ((file->current + len) > file->data_len)
        len = (file->data_len - file->current);

    if (file->version == LRPT_QPSK_FILE_VER1) { /* Version 1 */
        /* If we're in the middle of the byte in hardsymboled file restore remnants */
        if (lrpt_qpsk_file_is_hardsymboled(file) && (hardsym_off != 0)) {
            /* Offset from the start of hard symbol byte */
            const unsigned char hardsyms = (file->last_hardsym << (2 * hardsym_off));

            /* Use previously stored hard symbol byte */
            if (!lrpt_qpsk_data_from_hard(data, &hardsyms, hardsym_rem, err)) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                            "Can't convert hanging hard symbols");

                return false;
            }
        }

        /* Resize storage */
        if (!lrpt_qpsk_data_resize(data, len, err))
            return false;

        /* Determine required number of block reads */
        const size_t len_corr = (len - hardsym_rem); /* Account for the remnants */
        const size_t n_reads = (len_corr / IO_QPSK_DATA_N);

        for (size_t i = 0; i <= n_reads; i++) {
            /* Determine required number of symbols for current read */
            const size_t toread =
                (i == n_reads) ? (len_corr - n_reads * IO_QPSK_DATA_N) : IO_QPSK_DATA_N;

            if (toread == 0)
                break;

            if (lrpt_qpsk_file_is_hardsymboled(file)) {
                /* Number of bytes to read */
                const size_t n_bytes = ((toread - 1) / 4 + 1);

                /* Read block of QPSK bytes into temporary buffer */
                if (fread(file->iobuf, 1, n_bytes, file->fhandle) != n_bytes) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                                "Error during block read from QPSK file");

                    return false;
                }

                /* Parse block */
                for (size_t j = 0; j < n_bytes; j++) {
                    for (uint8_t k = 0; k < 8; k++) {
                        /* Prevent excess reading when we're on the last requested symbol */
                        if (
                                (i == n_reads) &&
                                (j == (n_bytes - 1)) &&
                                ((toread % 4) != 0) &&
                                (k == (2 * (toread % 4))))
                            break;

                        const unsigned char b = ((file->iobuf[j] >> (7 - k)) & 0x01);

                        data->qpsk[2 * hardsym_rem + 2 * i * IO_QPSK_DATA_N + 8 * j + k] =
                            (b == 0x01) ? 127 : -127;
                    }

                    /* On the very last read save the value of hard symbol byte */
                    if ((i == n_reads) && (j == (n_bytes - 1)))
                        file->last_hardsym = file->iobuf[j];
                }
            }
            else {
                /* Read block of QPSK bytes directly */
                if (fread(data->qpsk + 2 * i * IO_QPSK_DATA_N, 2, toread, file->fhandle) != toread) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FREAD,
                                "Error during block read from QPSK file");

                    return false;
                }
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

/* TODO implement custom length writing (perhaps with start offset) */
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

    /* Get number of QPSK symbols */
    size_t len = data->len;

    /* In case of empty write just exit */
    if (len == 0)
        return true;

    if (file->version == LRPT_QPSK_FILE_VER1) { /* Version 1 */
        /* Final buffer for combined write */
        lrpt_qpsk_data_t *fin = NULL;

        /* Check if we're in the middle of the byte in hardsymboled file */
        if (lrpt_qpsk_file_is_hardsymboled(file) && ((file->current % 4) != 0)) {
            /* Offset from the start of hard symbol byte */
            const uint8_t hardsym_off = (file->current % 4);

            fin = lrpt_qpsk_data_create_from_hard(&file->last_hardsym, file->current % 4, err);

            if (!fin) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_ALLOC,
                            "Can't allocate temporary buffer for hanging hard symbols");

                return false;
            }

            if (!lrpt_qpsk_data_append(fin, data, 0, data->len, err)) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                            "Can't append symbols to temporary buffer");

                return false;
            }

            data = fin;
            len += hardsym_off;

            file->current -= hardsym_off;
            file->data_len -= hardsym_off;
        }

        /* Determine required number of block writes */
        const size_t n_writes = (len / IO_QPSK_DATA_N);

        for (size_t i = 0; i <= n_writes; i++) {
            const size_t towrite = (i == n_writes) ? (len - n_writes * IO_QPSK_DATA_N) : IO_QPSK_DATA_N;

            if (towrite == 0)
                break;

            if (lrpt_qpsk_file_is_hardsymboled(file)) {
                /* Number of bytes to write */
                const size_t n_bytes = ((towrite - 1) / 4 + 1);

                /* Prepare block */
                for (size_t j = 0; j < n_bytes; j++) {
                    unsigned char b = 0x00;

                    for (uint8_t k = 0; k < 8; k++) {
                        /* Stop filling bits in case of extra space remains */
                        if (
                                (i == n_writes) &&
                                (j == (n_bytes - 1)) &&
                                ((towrite % 4) != 0) &&
                                (k == (2 * (towrite % 4))))
                            break;

                        if (data->qpsk[2 * i * IO_QPSK_DATA_N + 8 * j + k] >= 0)
                            b |= (1 << (7 - k));
                    }

                    file->iobuf[j] = b;

                    /* On the very last write save the value of hard symbol byte */
                    if ((i == n_writes) && (j == (n_bytes - 1)))
                        file->last_hardsym = b;
                }

                /* Write block */
                if (fwrite(file->iobuf, 1, n_bytes, file->fhandle) != n_bytes) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                                "Error during block write to QPSK file");

                    return false;
                }
            }
            else {
                /* Write block of QPSK bytes directly */
                if (fwrite(data->qpsk + 2 * i * IO_QPSK_DATA_N, 2, towrite, file->fhandle) != towrite) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                                "Error during block write to QPSK file");

                    return false;
                }
            }

            file->current += towrite;
            file->data_len += towrite;

            /* Flush data length if inplace is requested */
            if (inplace) {
                unsigned char v_s[8];

                lrpt_utils_s_uint64_t(file->data_len, v_s, true);
                fseek(file->fhandle, file->header_len - 8, SEEK_SET);

                if (fwrite(v_s, 1, 8, file->fhandle) != 8) {
                    if (err)
                        lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                                "QPSK data length write error");

                    return false;
                }

                fseek(
                        file->fhandle,
                        file->header_len + (lrpt_qpsk_file_is_hardsymboled(file) ?
                            (file->current / 4) :
                            (2 * file->current)),
                        SEEK_SET);
            }
        }

        /* Flush data length if inplace isn't requested */
        if (!inplace) {
            unsigned char v_s[8];

            lrpt_utils_s_uint64_t(file->data_len, v_s, true);
            fseek(file->fhandle, file->header_len - 8, SEEK_SET);

            if (fwrite(v_s, 1, 8, file->fhandle) != 8) {
                if (err)
                    lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_FWRITE,
                            "QPSK data length write error");

                return false;
            }

            fseek(
                    file->fhandle,
                    file->header_len + (lrpt_qpsk_file_is_hardsymboled(file) ?
                        (file->current / 4) :
                        (2 * file->current)),
                    SEEK_SET);
        }

        lrpt_qpsk_data_free(fin);
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
