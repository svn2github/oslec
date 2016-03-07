/*
 * SpanDSP - a series of DSP components for telephony
 *
 * hdlc.c
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2003 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: hdlc.c,v 1.44 2006/11/28 16:59:56 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "spandsp/telephony.h"
#include "spandsp/power_meter.h"
#include "spandsp/async.h"
#include "spandsp/hdlc.h"
#include "spandsp/fsk.h"
#include "spandsp/bit_operations.h"

#include "spandsp/timing.h"

static const uint32_t crc_itu32_table[] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint32_t crc_itu32_calc(const uint8_t *buf, int len, uint32_t crc)
{
    int i;

    for (i = 0;  i < len;  i++)
        crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_itu32_table[(crc ^ buf[i]) & 0xFF];
    return crc;
}
/*- End of function --------------------------------------------------------*/

int crc_itu32_append(uint8_t *buf, int len)
{
    uint32_t crc;
    int new_len;
    int i;

    crc = 0xFFFFFFFF;
    new_len = len + 4;
    for (i = 0;  i < len;  i++)
        crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_itu32_table[(crc ^ buf[i]) & 0xFF];
    crc ^= 0xFFFFFFFF;
    buf[i++] = (uint8_t) crc;
    buf[i++] = (uint8_t) (crc >> 8);
    buf[i++] = (uint8_t) (crc >> 16);
    buf[i++] = (uint8_t) (crc >> 24);
    return new_len;
}
/*- End of function --------------------------------------------------------*/

int crc_itu32_check(const uint8_t *buf, int len)
{
    uint32_t crc;
    int i;

    crc = 0xFFFFFFFF;
    for (i = 0;  i < len;  i++)
        crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_itu32_table[(crc ^ buf[i]) & 0xFF];
    return (crc == 0xDEBB20E3);
}
/*- End of function --------------------------------------------------------*/

static const uint16_t crc_itu16_table[] =
{
    0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
    0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
    0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
    0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
    0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
    0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
    0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
    0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
    0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
    0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
    0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
    0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
    0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
    0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
    0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
    0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
    0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
    0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
    0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
    0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
    0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
    0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
    0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
    0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
    0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
    0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
    0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
    0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
    0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
    0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
    0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
    0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78
};

uint16_t crc_itu16_calc(const uint8_t *buf, int len, uint16_t crc)
{
    int i;

    for (i = 0;  i < len;  i++)
        crc = (crc >> 8) ^ crc_itu16_table[(crc ^ buf[i]) & 0xFF];
    return crc;
}
/*- End of function --------------------------------------------------------*/

int crc_itu16_append(uint8_t *buf, int len)
{
    uint16_t crc;
    int new_len;
    int i;

    crc = 0xFFFF;
    new_len = len + 2;
    for (i = 0;  i < len;  i++)
        crc = (crc >> 8) ^ crc_itu16_table[(crc ^ buf[i]) & 0xFF];
    crc ^= 0xFFFF;
    buf[i++] = (uint8_t) crc;
    buf[i++] = (uint8_t) (crc >> 8);
    return new_len;
}
/*- End of function --------------------------------------------------------*/

int crc_itu16_check(const uint8_t *buf, int len)
{
    uint16_t crc;
    int i;

    crc = 0xFFFF;
    for (i = 0;  i < len;  i++)
        crc = (crc >> 8) ^ crc_itu16_table[(crc ^ buf[i]) & 0xFF];
    return (crc & 0xFFFF) == 0xF0B8;
}
/*- End of function --------------------------------------------------------*/

static void rx_special_condition(hdlc_rx_state_t *s, int condition)
{
    /* Special conditions */
    switch (condition)
    {
    case PUTBIT_CARRIER_UP:
    case PUTBIT_TRAINING_SUCCEEDED:
        /* Reset the HDLC receiver. */
        s->len = 0;
        s->num_bits = 0;
        s->flags_seen = 0;
        s->framing_ok_announced = FALSE;
        /* Fall through */
    case PUTBIT_CARRIER_DOWN:
    case PUTBIT_TRAINING_FAILED:
    case PUTBIT_END_OF_DATA:
        s->frame_handler(s->user_data, TRUE, NULL, condition);
        break;
    default:
        //printf("Eh!\n");
        break;
    }
}
/*- End of function --------------------------------------------------------*/

static void rx_flag_or_abort(hdlc_rx_state_t *s)
{
    if ((s->raw_bit_stream & 0x8000))
    {
        /* Hit HDLC abort */
        s->rx_aborts++;
        s->frame_handler(s->user_data, TRUE, NULL, PUTBIT_ABORT);
        if (s->flags_seen < s->framing_ok_threshold)
            s->flags_seen = 0;
    }
    else
    {
        /* Hit HDLC flag */
        if (s->flags_seen >= s->framing_ok_threshold)
        {
            /* Announce framing OK between frames. */
            if (!s->framing_ok_announced)
            {
                s->frame_handler(s->user_data, TRUE, NULL, PUTBIT_FRAMING_OK);
                s->framing_ok_announced = TRUE;
            }
            /* We may have a frame, or we may have back to back flags */
            if (s->len)
            {
                if (s->len >= s->crc_bytes  &&  s->len <= (int) sizeof(s->buffer))
                {
                    if ((s->crc_bytes == 2  &&  crc_itu16_check(s->buffer, s->len))
                        ||
                        (s->crc_bytes != 2  &&  crc_itu32_check(s->buffer, s->len)))
                    {
                        s->rx_frames++;
                        s->rx_bytes += s->len - s->crc_bytes;
                        s->frame_handler(s->user_data,
                                         TRUE,
                                         s->buffer,
                                         s->len - s->crc_bytes);
                    }
                    else
                    {
                        s->rx_crc_errors++;
                        if (s->report_bad_frames)
                        {
                            s->frame_handler(s->user_data,
                                             FALSE,
                                             s->buffer,
                                             s->len - s->crc_bytes);
                        }
                    }
                    s->framing_ok_announced = FALSE;
                }
                else
                {
                    /* Frame too short or too long */
                    if (s->report_bad_frames)
                    {
                        s->frame_handler(s->user_data,
                                         FALSE,
                                         s->buffer,
                                         s->len - s->crc_bytes);
                    }
                    s->rx_length_errors++;
                }
            }
        }
        else
        {
            /* Check the flags are back-to-back when testing for valid preamble. This
               greatly reduces the chances of false preamble detection, and anything
               which doesn't send them back-to-back is badly broken. */
            if (s->flags_seen  &&  s->num_bits != 7)
                s->flags_seen = 0;
            if (++s->flags_seen == s->framing_ok_threshold  &&  !s->framing_ok_announced)
            {
                s->frame_handler(s->user_data, TRUE, NULL, PUTBIT_FRAMING_OK);
                s->framing_ok_announced = TRUE;
            }
        }
    }
    s->len = 0;
    s->num_bits = 0;
}
/*- End of function --------------------------------------------------------*/

void hdlc_rx_put_bit(hdlc_rx_state_t *s, int new_bit)
{
    if (new_bit < 0)
    {
        rx_special_condition(s, new_bit);
        return;
    }
    s->raw_bit_stream = (s->raw_bit_stream << 1) | ((new_bit << 8) & 0x100);
    if ((s->raw_bit_stream & 0x3F00) == 0x3E00)
    {
        if ((s->raw_bit_stream & 0x4000))
        {
            rx_flag_or_abort(s);
        }
        else
        {
            if (s->flags_seen < s->framing_ok_threshold)
                s->num_bits++;
        }
    }
    else
    {
        s->num_bits++;
        if (s->flags_seen >= s->framing_ok_threshold)
        {
            s->byte_in_progress = (s->byte_in_progress | (s->raw_bit_stream & 0x100)) >> 1;
            if (s->num_bits == 8)
            {
                /* Ensure we do not overflow our buffer */
                if (s->len < (int) sizeof(s->buffer))
                    s->buffer[s->len++] = (uint8_t) s->byte_in_progress;
                else
                    s->len = (int) sizeof(s->buffer) + 1;
                s->num_bits = 0;
            }
        }
    }
}
/*- End of function --------------------------------------------------------*/

void hdlc_rx_put_byte(hdlc_rx_state_t *s, int new_byte)
{
    int i;

    if (new_byte < 0)
    {
        rx_special_condition(s, new_byte);
        return;
    }
    s->raw_bit_stream |= new_byte;

    i = 0;
    if (s->flags_seen < s->framing_ok_threshold)
    {
        for (  ;  i < 8;  i++)
        {
            s->raw_bit_stream <<= 1;
            if ((s->raw_bit_stream & 0x7F00) == 0x7E00)
            {
                rx_flag_or_abort(s);
                if (s->flags_seen >= s->framing_ok_threshold)
                {
                    i++;
                    break;
                }
            }
            else
            {
                s->num_bits++;
            }
        }
    }
    for (  ;  i < 8;  i++)
    {
        s->raw_bit_stream <<= 1;
        if ((s->raw_bit_stream & 0x3F00) == 0x3E00)
        {
            if ((s->raw_bit_stream & 0x4000))
                rx_flag_or_abort(s);
        }
        else
        {
            s->byte_in_progress = (s->byte_in_progress | (s->raw_bit_stream & 0x100)) >> 1;
            if (++s->num_bits == 8)
            {
                /* Ensure we do not overflow our buffer */
                if (s->len < (int) sizeof(s->buffer))
                    s->buffer[s->len++] = (uint8_t) s->byte_in_progress;
                else
                    s->len = (int) sizeof(s->buffer) + 1;
                s->num_bits = 0;
            }
        }
    }
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_frame(hdlc_tx_state_t *s, const uint8_t *frame, int len)
{
    if (len <= 0)
    {
        s->tx_end = TRUE;
        return 0;
    }
    if (s->len + len > s->max_frame_len)
        return -1;
    if (s->progressive)
    {
        /* Only lock out if we are in the CRC section. */
        if (s->pos >= HDLC_MAXFRAME_LEN)
            return -1;
    }
    else
    {
        /* Lock out if there is anything in the buffer. */
        if (s->len)
            return -1;
    }
    memcpy(s->buffer + s->len, frame, len);
    if (s->crc_bytes == 2)
        s->crc = crc_itu16_calc(frame, len, (uint16_t) s->crc);
    else
        s->crc = crc_itu32_calc(frame, len, s->crc);
    if (s->progressive)
        s->len += len;
    else
        s->len = len;
    s->tx_end = FALSE;
    return 0;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_preamble(hdlc_tx_state_t *s, int len)
{
    /* Some HDLC applications require the ability to force a period of HDLC
       flag words. */
    if (s->pos)
        return -1;
    if (len < 0)
        s->flag_octets += -len;
    else
        s->flag_octets = len;
    s->report_flag_underflow = TRUE;
    s->tx_end = FALSE;
    return 0;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_corrupt_frame(hdlc_tx_state_t *s)
{
    if (s->len <= 0)
        return -1;
    s->crc ^= 0xFFFF;
    s->buffer[HDLC_MAXFRAME_LEN] ^= 0xFF;
    s->buffer[HDLC_MAXFRAME_LEN + 1] ^= 0xFF;
    s->buffer[HDLC_MAXFRAME_LEN + 2] ^= 0xFF;
    s->buffer[HDLC_MAXFRAME_LEN + 3] ^= 0xFF;
    return 0;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_get_byte(hdlc_tx_state_t *s)
{
    int i;
    int byte_in_progress;
    int txbyte;

    if (s->flag_octets > 0)
    {
        /* We are in a timed flag section (preamble, inter frame gap, etc.) */
        if (--s->flag_octets <= 0  &&  s->report_flag_underflow)
        {
            if (s->len == 0)
            {
                /* The timed preamble has finished, there is nothing else queued to go,
                   and we have been told to report this underflow. */
                if (s->underflow_handler)
                    s->underflow_handler(s->user_data);
            }
            s->report_flag_underflow = FALSE;
        }
        return s->idle_octet;
    }
    if (s->len)
    {
        if (s->num_bits >= 8)
        {
            s->num_bits -= 8;
            return (s->octets_in_progress >> s->num_bits) & 0xFF;
        }
        if (s->pos >= s->len)
        {
            if (s->pos == s->len)
            {
                s->crc ^= 0xFFFFFFFF;
                s->buffer[HDLC_MAXFRAME_LEN] = (uint8_t) s->crc;
                s->buffer[HDLC_MAXFRAME_LEN + 1] = (uint8_t) (s->crc >> 8);
                if (s->crc_bytes == 4)
                {
                    s->buffer[HDLC_MAXFRAME_LEN + 2] = (uint8_t) (s->crc >> 16);
                    s->buffer[HDLC_MAXFRAME_LEN + 3] = (uint8_t) (s->crc >> 24);
                }
                s->pos = HDLC_MAXFRAME_LEN;
            }
            else if (s->pos == HDLC_MAXFRAME_LEN + s->crc_bytes)
            {
                /* Finish off the current byte with some flag bits. If we are at the
                   start of a byte we need a at least one whole byte of flag to ensure
                   we cannot end up with back to back frames, and no flag octet at all */
                txbyte = (uint8_t) ((s->octets_in_progress << (8 - s->num_bits)) | (0x7E >> s->num_bits));
                /* Create a rotated octet of flag for idling... */
                s->idle_octet = (0x7E7E >> s->num_bits) & 0xFF;
                /* ...and the partial flag octet needed to start off the next message. */
                s->octets_in_progress = s->idle_octet >> (8 - s->num_bits);
                s->flag_octets = s->inter_frame_flags - 1;
                s->len = 0;
                s->pos = 0;
                if (s->crc_bytes == 2)
                    s->crc = 0xFFFF;
                else
                    s->crc = 0xFFFFFFFF;
                /* Report the underflow now. If there is a timed preamble still in progress, loading the
                   next frame right now will be harmless. */
                s->report_flag_underflow = FALSE;
                if (s->underflow_handler)
                    s->underflow_handler(s->user_data);
                /* Make sure we finish off with at least one flag octet, if the underflow report did not result
                   in a new frame being sent. */
                if (s->len == 0  &&  s->flag_octets < 2)
                    s->flag_octets = 2;
                return txbyte;
            }
        }
        byte_in_progress = s->buffer[s->pos++];
        i = bottom_bit(byte_in_progress | 0x100);
        s->octets_in_progress <<= i;
        byte_in_progress >>= i;
        for (  ;  i < 8;  i++)
        {
            s->octets_in_progress = (s->octets_in_progress << 1) | (byte_in_progress & 0x01);
            byte_in_progress >>= 1;
            if ((s->octets_in_progress & 0x1F) == 0x1F)
            {
                /* There are 5 ones - stuff */
                s->octets_in_progress <<= 1;
                s->num_bits++;
            }
        }
        /* An input byte will generate between 8 and 10 output bits */
        return (s->octets_in_progress >> s->num_bits) & 0xFF;
    }
    /* Untimed idling on flags */
    if (s->tx_end)
    {
        s->tx_end = FALSE;
        return PUTBIT_END_OF_DATA;
    }
    return s->idle_octet;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_get_bit(hdlc_tx_state_t *s)
{
    int txbit;

    if (s->bits == 0)
    {
        if ((s->byte = hdlc_tx_get_byte(s)) < 0)
            return s->byte;
        s->bits = 8;
    }
    s->bits--;
    txbit = (s->byte >> s->bits) & 0x01;
    return  txbit;
}
/*- End of function --------------------------------------------------------*/

hdlc_rx_state_t *hdlc_rx_init(hdlc_rx_state_t *s,
                              int crc32,
                              int report_bad_frames,
                              int framing_ok_threshold,
                              hdlc_frame_handler_t handler,
                              void *user_data)
{
    memset(s, 0, sizeof(*s));
    s->frame_handler = handler;
    s->user_data = user_data;
    s->crc_bytes = (crc32)  ?  4  :  2;
    s->report_bad_frames = report_bad_frames;
    s->framing_ok_threshold = (framing_ok_threshold < 1)  ?  1  :  framing_ok_threshold;
    return s;
}
/*- End of function --------------------------------------------------------*/

int hdlc_rx_get_stats(hdlc_rx_state_t *s,
                      hdlc_rx_stats_t *t)
{
    t->bytes = s->rx_bytes;
    t->good_frames = s->rx_frames;
    t->crc_errors = s->rx_crc_errors;
    t->length_errors = s->rx_length_errors;
    t->aborts = s->rx_aborts;
    return 0;
}
/*- End of function --------------------------------------------------------*/

void hdlc_tx_set_max_frame_len(hdlc_tx_state_t *s, int max_len)
{
    s->max_frame_len = (max_len <= HDLC_MAXFRAME_LEN)  ?  max_len  :  HDLC_MAXFRAME_LEN;
}
/*- End of function --------------------------------------------------------*/

hdlc_tx_state_t *hdlc_tx_init(hdlc_tx_state_t *s,
                              int crc32,
                              int inter_frame_flags,
                              int progressive,
                              hdlc_underflow_handler_t handler,
                              void *user_data)
{
    memset(s, 0, sizeof(*s));
    s->idle_octet = 0x7E;
    s->underflow_handler = handler;
    s->user_data = user_data;
    s->inter_frame_flags = (inter_frame_flags < 1)  ?  1  :  inter_frame_flags;
    if (crc32)
    {
        s->crc_bytes = 4;
        s->crc = 0xFFFFFFFF;
    }
    else
    {
        s->crc_bytes = 2;
        s->crc = 0xFFFF;
    }
    s->progressive = progressive;
    s->max_frame_len = HDLC_MAXFRAME_LEN;
    return s;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
