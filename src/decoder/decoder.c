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
 * Decoder routines.
 *
 * This source file contains routines for performing decoding of LRPT signals.
 */

/*************************************************************************************************/

#include "decoder.h"

/*************************************************************************************************/

void Decode_Image(uint8_t *in_buffer, int buf_len) {
  bool ok;
  gchar txt[16];

  while( mtd_record.pos < buf_len )
  {
    ok = Mtd_One_Frame( &mtd_record, in_buffer );
    if (ok) {
      Parse_Cvcdu( mtd_record.ecced_data, HARD_FRAME_LEN - 132 );
      ok_cnt++;

      /* LIBLRPT GUI-specific */
      {
      if( isFlagClear(FRAME_OK_ICON) )
      {
        Display_Icon( frame_icon, "gtk-yes" );
        SetFlag( FRAME_OK_ICON );
      }
      }
    }
    else {
        /* LIBLRPT GUI-specific */
        {
      if( isFlagSet(FRAME_OK_ICON) )
      {
        Display_Icon( frame_icon, "gtk-no" );
        ClearFlag( FRAME_OK_ICON );
      }
        }
    }

    total_cnt++;
  }

  /* LIBLRPT gui-specific */
  {
  /* Print decoder status data */
  snprintf( txt, sizeof(txt), "%d", mtd_record.sig_q );
  gtk_entry_set_text( GTK_ENTRY(sig_quality_entry), txt );
  int percent = ( 100 * ok_cnt ) / total_cnt;
  snprintf( txt, sizeof(txt), "%d:%d%%", ok_cnt, percent );
  gtk_entry_set_text( GTK_ENTRY(packet_cnt_entry), txt );
  }
}

/*************************************************************************************************/

/** \endcond */
