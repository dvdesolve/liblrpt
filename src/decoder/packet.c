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

/*************************************************************************************************/

/* Limit value of M_PDU header pointer. Needed for discrimination of partial packets. */
static const uint16_t PACKET_FULL_MARK = 2047;

/*************************************************************************************************/

/** Parse APID 70.
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
static void parse_img(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        uint16_t apid,
        uint16_t pck_cnt);

/** Parse APIDs.
 *
 * \param decoder Pointer to the decoder object.
 * \param p Data buffer.
 */
static void parse_apid(
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
static uint16_t parse_partial(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        uint16_t len);

/*************************************************************************************************/

/* parse_70() */
static void parse_70(
        lrpt_decoder_t *decoder,
        uint8_t *p) {
    switch (decoder->sc) {
        case LRPT_DECODER_SC_METEORM2:
            {
                /* TODO implement properly */
                /* For more information see appendix "A",
                 * http://planet.iitp.ru/spacecraft/meteor_m_n2_structure_2.pdf
                 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
                uint8_t hour = p[8];
                uint8_t min = p[9];
                uint8_t sec = p[10];
                uint16_t msec = (p[11] * 4);
#pragma GCC diagnostic pop
            }

            break;

        default:
            break;
    }
}

/*************************************************************************************************/

/* parse_img() */
static void parse_img(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        uint16_t apid,
        uint16_t pck_cnt) {
    switch (decoder->sc) {
        case LRPT_DECODER_SC_METEORM2:
            {
                /* For more information see section "I",
                 * http://planet.iitp.ru/spacecraft/meteor_m_n2_structure_2.pdf
                 */
                const uint8_t mcu_id = p[0];
                const uint8_t q = p[5];

                lrpt_decoder_jpeg_decode_mcus(decoder, p + 6, apid, pck_cnt, mcu_id, q);
            }

            break;

        default:
            break;
    }
}

/*************************************************************************************************/

/* parse_apid() */
static void parse_apid(
        lrpt_decoder_t *decoder,
        uint8_t *p) {
    /* For more information see section "3.2 Source Packet structure",
     * https://www-cdn.eumetsat.int/files/2020-04/pdf_mo_ds_esa_sy_0048_iss8.pdf
     */
    uint16_t apid = (((p[0] << 8) | p[1]) & 0x07FF);
    uint16_t pck_cnt = (((p[2] << 8) | p[3]) & 0x3FFF);

    /* TODO implement properly */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
    uint16_t pck_len = ((p[4] << 8) | p[5]);
#pragma GCC diagnostic pop

    /* 14 is an offset to get "User data" block directly */
    if ((apid >= 64) && (apid <= 69))
        parse_img(decoder, p + 14, apid, pck_cnt);
    else if (apid == 70)
        parse_70(decoder, p + 14);
}

/*************************************************************************************************/

/* parse_partial() */
static uint16_t parse_partial(
        lrpt_decoder_t *decoder,
        uint8_t *p,
        uint16_t len) {
    /* For more information see section "3.2 Source Packet structure",
     * https://www-cdn.eumetsat.int/files/2020-04/pdf_mo_ds_esa_sy_0048_iss8.pdf
     */
    /* Check if we have only part of primary header */
    if (len < 6) {
        decoder->packet_part = true;

        return 0;
    }

    uint16_t pck_len = ((p[4] << 8) | p[5]);

    /* If not all data is in packet... */
    if (pck_len >= (len - 6)) {
        decoder->packet_part = true;

        return 0;
    }

    parse_apid(decoder, p);

    decoder->packet_part = false;

    /* Compensate for -1 in counter and add primary header length */
    return (pck_len + 6 + 1);
}

/*************************************************************************************************/

/* lrpt_decoder_packet_parse_cvcdu() */
void lrpt_decoder_packet_parse_cvcdu(
        lrpt_decoder_t *decoder) {
    uint8_t *p = decoder->ecced;

    /* Parse VCDU primary header. For more details see section "5 DATA LINK LAYER",
     * https://www-cdn.eumetsat.int/files/2020-04/pdf_mo_ds_esa_sy_0048_iss8.pdf
     */
    uint16_t w = ((p[0] << 8) | p[1]); /* Version + VCDU-ID */
    uint8_t ver = (w >> 14); /* CCSDS structure version (should be 0x01 for version-2) */
    uint8_t vch_id = (w & 0x3F); /* Virtual channel identifier (should be 5 for LRPT AVHRR) */
    uint32_t vcdu_cnt = ((p[2] << 16) | (p[3] << 8) | p[4]); /* VCDU counter */

    /* Parse VCDU data unit zone */
    w = ((p[8] << 8) | p[9]);
    uint16_t hdr_off = (w & 0x07FF); /* M_PDU header first pointer */

    /* TODO as of now we're only dropping empty packets. However we can extend functionality for
     * other applications too.
     */
    if ((ver == 0) || (vch_id == 0))
        return;

    /* We're subtracting 132 because of 4 bytes of Reed-Solomon coding with interleaving
     * depth of 4 and 128 bytes of CVCDU check symbols inside CVCDU
     */
    const uint16_t len = (LRPT_DECODER_HARD_FRAME_LEN - 132);

    /* Subtract 10 octets because of CVCDU structure to get pointer to the M_PDU packet zone */
    uint16_t data_len = (len - 10);

    if (vcdu_cnt == (decoder->last_vcdu + 1)) { /* Process consecutive VCDUs */
        if (decoder->packet_part) {
            if (hdr_off == PACKET_FULL_MARK) { /* For packets which are larger than one frame */
                hdr_off = (len - 10);
                memcpy(decoder->packet_buf + decoder->packet_off,
                        p + 10,
                        sizeof(uint8_t) * hdr_off);
                decoder->packet_off += hdr_off;
            }
            else {
                memcpy(decoder->packet_buf + decoder->packet_off,
                        p + 10,
                        sizeof(uint8_t) * hdr_off);
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

    /* Store index of last VCDU */
    decoder->last_vcdu = vcdu_cnt;

    data_len -= hdr_off;
    uint16_t off = hdr_off;

    while (data_len > 0) {
        uint16_t n = parse_partial(decoder, p + 10 + off, data_len);

        if (decoder->packet_part) {
            decoder->packet_off = data_len;
            memcpy(decoder->packet_buf,
                    p + 10 + off,
                    sizeof(uint8_t) * decoder->packet_off);

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
