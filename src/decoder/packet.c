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
 * Packet handling routines.
 *
 * This source file contains routines for performing packet manipulation for LRPT signals.
 */

/*************************************************************************************************/

#include "packet.h"

#include "../../include/lrpt.h"
#include "decoder.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* DEBUG */
#include <inttypes.h>
#include <stdio.h>
/* DEBUG */

/*************************************************************************************************/

/* TODO it's related to the M_PDU header pointer */
static const uint16_t PACKET_FULL_MARK = 2047; /**< Needed for discrimination of partial packets */

/*************************************************************************************************/

/** Parse APID 70 (onboard time).
 *
 * \param decoder Pointer to the decoder object.
 * \param p Data buffer.
 */
static void parse_70(
        lrpt_decoder_t *decoder,
        uint8_t *p);

/** Perform action for other APIDs.
 *
 * \param decoder Pointer to the decoder object.
 * \param p Data buffer.
 * \param apid APID number.
 * \param pck_cnt Packet count.
 */
static void act_apd(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        uint16_t apid,
        uint16_t pck_cnt);

/** Parse APIDs.
 *
 * \param decoder Pointer to the decoder object.
 * \param p Data buffer.
 */
static void parse_apd(
        lrpt_decoder_t *decoder,
        uint8_t *p);

/** Parse partial packet.
 *
 * \param decoder Pointer to the decoder object.
 * \param p Data buffer.
 * \param len Data length.
 *
 * \return Packet length.
 */
static size_t parse_partial(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        size_t len);

/*************************************************************************************************/

/* parse_70() */
static void parse_70(
        lrpt_decoder_t *decoder,
        uint8_t *p) {
    /* hour = p[8];
     * min = p[9];
     * sec = p[10];
     * msec = p[11] * 4
     */
}

/*************************************************************************************************/

/* act_apd() */
static void act_apd(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        uint16_t apid,
        uint16_t pck_cnt) {
    const uint8_t mcu_id = p[0];
    const uint8_t q = p[5];

    lrpt_decoder_jpeg_decode_mcus(decoder, p + 6, apid, pck_cnt, mcu_id, q);
}

/*************************************************************************************************/

/* parse_apd() */
static void parse_apd(
        lrpt_decoder_t *decoder,
        uint8_t *p) {
    /* TODO recheck cast */
    uint16_t w = (uint16_t)((p[0] << 8) | p[1]);
    uint16_t apid = (w & 0x07FF); /* TODO consult with LRPT documentation for limit values */

    uint16_t pck_cnt = (uint16_t)((p[2] << 8) | p[3]) & 0x3FFF;

    if (apid == 70) /* Parse onboard time data */
        parse_70(decoder, p + 14);
    else
        act_apd(decoder, p + 14, apid, pck_cnt);
}

/*************************************************************************************************/

/* parse_partial() */
static size_t parse_partial(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        size_t len) {
    if (len < 6) {
        decoder->packet_part = true;

        return 0;
    }

    size_t len_pck = (p[4] << 8) | p[5];

    if (len_pck >= (len - 6)) {
        decoder->packet_part = true;

        return 0;
    }

    parse_apd(decoder, p);

    decoder->packet_part = false;

    return (len_pck + 7);
}

/*************************************************************************************************/

/* lrpt_decoder_packet_parse_cvcdu() */
void lrpt_decoder_packet_parse_cvcdu(
        lrpt_decoder_t *decoder,
        size_t len) {
    uint8_t *p = decoder->ecced;

    /* TODO do we need that explicit casts? */
    /* TODO deal with VCDU primary header */
    uint32_t frame_cnt = (uint32_t)((p[2] << 16) | (p[3] << 8) | p[4]); /* TODO that should be VCDU counter */
    uint16_t w = (uint16_t)((p[0] << 8) | p[1]);
    uint8_t ver = w >> 14;
    uint8_t fid = w & 0x3F; /* TODO that should be VCDU-id */

    /* TODO deal with VCDU data unit zone */
    w = (uint16_t)((p[8] << 8) | p[9]);
    size_t hdr_off = (w & 0x07FF); /* TODO M_PDU header first pointer */

    /* DEBUG */
    fprintf(stderr, "lrpt_decoder_packet_parse_cvcdu(): frame_cnt = %" PRIu32 "; ver = %" PRIu8 "; fid = %" PRIu8 "; hdr_off = %zu\n", frame_cnt, ver, fid, hdr_off);
    /* DEBUG */

    /* Deal with empty packets */
    /* TODO review that and process only AVHRR LR packets for now */
    if ((ver == 0) || (fid == 0))
        return;

    /* TODO we're subtracting 10 octets because of CVCDU structure */
    size_t data_len = len - 10;

    if (frame_cnt == (decoder->last_frame + 1)) { /* TODO process consecutive frame */
        if (decoder->packet_part) {
            if (hdr_off == PACKET_FULL_MARK) { /* For packets which are larger than one frame */
                hdr_off = (len - 10);
                memcpy(decoder->packet_buf + decoder->packet_off, p + 10, hdr_off);
                decoder->packet_off += hdr_off;
            }
            else {
                memcpy(decoder->packet_buf + decoder->packet_off, p + 10, hdr_off);
                parse_partial(decoder, decoder->packet_buf, decoder->packet_off + hdr_off);
            }
        }
    }
    else {
        if (hdr_off == PACKET_FULL_MARK) /* For packets which are larger than one frame */
            return;

        decoder->packet_part = false;
        decoder->packet_off = 0;
    }

    /* Store index of last frame */
    decoder->last_frame = frame_cnt;

    data_len -= hdr_off;
    size_t off = hdr_off;

    while (data_len > 0) {
        size_t n = parse_partial(decoder, p + 10 + off, data_len);

        if (decoder->packet_part) {
            decoder->packet_off = data_len;
            memcpy(decoder->packet_buf, p + 10 + off, decoder->packet_off);

            break;
        }
        else {
            off += n;
            data_len -= n;
        }
    }
}

/*************************************************************************************************/

/** \endcond */
