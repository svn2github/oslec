/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t38_core.h - An implementation of T.38, less the packet exchange part
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2005 Steve Underwood
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
 * $Id: t38_core.h,v 1.11 2006/12/07 13:22:26 steveu Exp $
 */

/*! \file */

#if !defined(_T38_CORE_H_)
#define _T38_CORE_H_

/*! \page t38_core_page T.38 real time FAX over IP message handling
There are two ITU recommendations which address sending FAXes over IP networks. T.37 specifies a
method of encapsulating FAX images in e-mails, and transporting them to the recipient (an e-mail
box, or another FAX machine) in a store-and-forward manner. T.38 defines a protocol for
transmitting a FAX across an IP network in real time. The core T.38 modules implements the basic
message handling for the T.38, real time, FAX over IP (FoIP) protocol.

The T.38 protocol can operate between:
    - Internet-aware FAX terminals, which connect directly to an IP network. The T.38 terminal module
      extends this module to provide a complete T.38 terminal.
    - FAX gateways, which allow traditional PSTN FAX terminals to communicate through the Internet.
      The T.38 gateway module extends this module to provide a T.38 gateway.
    - A combination of terminals and gateways.

T.38 is the only standardised protocol which exists for real-time FoIP. Reliably transporting a
FAX between PSTN FAX terminals, through an IP network, requires use of the T.38 protocol at FAX
gateways. VoIP connections are not robust for modem use, including FAX modem use. Most use low
bit rate codecs, which cannot convey the modem signals accurately. Even when high bit rate
codecs are used, VoIP connections suffer dropouts and timing adjustments, which modems cannot
tolerate. In a LAN environment the dropout rate may be very low, but the timing adjustments which
occur in VoIP connections still make modem operation unreliable. T.38 FAX gateways deal with the
delays, timing jitter, and packet loss experienced in packet networks, and isolate the PSTN FAX
terminals from these as far as possible. In addition, by sending FAXes as image data, rather than
digitised audio, they reduce the required bandwidth of the IP network.

\section t38_core_page_sec_1 What does it do?

\section t38_core_page_sec_2 How does it work?

Timing differences and jitter between two T.38 entities can be a serious problem, if one of those
entities is a PSTN gateway.

Flow control for non-ECM image data takes advantage of several features of the T.30 specification.
First, an unspecified number of 0xFF octets may be sent at the start of transmission. This means we
can add endless extra 0xFF bytes at this point, without breaking the T.30 spec. In practice, we
cannot add too many, or we will affect the timing tolerance of the T.30 protocol by delaying the
response at the end of each image. Secondly, just before an end of line (EOL) marker we can pad
with zero bits. Again, the number is limited only by need to avoid upsetting the timing of the
step following the non-ECM data.
*/

enum t30_indicator_types_e
{
    T38_IND_NO_SIGNAL = 0,
    T38_IND_CNG,
    T38_IND_CED,
    T38_IND_V21_PREAMBLE,
    T38_IND_V27TER_2400_TRAINING,
    T38_IND_V27TER_4800_TRAINING,
    T38_IND_V29_7200_TRAINING,
    T38_IND_V29_9600_TRAINING,
    T38_IND_V17_7200_SHORT_TRAINING,
    T38_IND_V17_7200_LONG_TRAINING,
    T38_IND_V17_9600_SHORT_TRAINING,
    T38_IND_V17_9600_LONG_TRAINING,
    T38_IND_V17_12000_SHORT_TRAINING,
    T38_IND_V17_12000_LONG_TRAINING,
    T38_IND_V17_14400_SHORT_TRAINING,
    T38_IND_V17_14400_LONG_TRAINING,
    T38_IND_V8_ANSAM,
    T38_IND_V8_SIGNAL,
    T38_IND_V34_CNTL_CHANNEL_1200,
    T38_IND_V34_PRI_CHANNEL,
    T38_IND_V34_CC_RETRAIN,
    T38_IND_V33_12000_TRAINING,
    T38_IND_V33_14400_TRAINING
};

enum t38_data_types_e
{
    T38_DATA_NONE = -1,
    T38_DATA_V21 = 0,
    T38_DATA_V27TER_2400,
    T38_DATA_V27TER_4800,
    T38_DATA_V29_7200,
    T38_DATA_V29_9600,
    T38_DATA_V17_7200,
    T38_DATA_V17_9600,
    T38_DATA_V17_12000,
    T38_DATA_V17_14400,
    T38_DATA_V8,
    T38_DATA_V34_PRI_RATE,
    T38_DATA_V34_CC_1200,
    T38_DATA_V34_PRI_CH,
    T38_DATA_V33_12000,
    T38_DATA_V33_14400
};

enum t38_field_types_e
{
    T38_FIELD_HDLC_DATA = 0,
    T38_FIELD_HDLC_SIG_END,
    T38_FIELD_HDLC_FCS_OK,
    T38_FIELD_HDLC_FCS_BAD,
    T38_FIELD_HDLC_FCS_OK_SIG_END,
    T38_FIELD_HDLC_FCS_BAD_SIG_END,
    T38_FIELD_T4_NON_ECM_DATA,
    T38_FIELD_T4_NON_ECM_SIG_END,
    T38_FIELD_CM_MESSAGE,
    T38_FIELD_JM_MESSAGE,
    T38_FIELD_CI_MESSAGE,
    T38_FIELD_V34RATE
};

enum t38_field_classes_e
{
    T38_FIELD_CLASS_NONE = 0,
    T38_FIELD_CLASS_HDLC,
    T38_FIELD_CLASS_NON_ECM,
};

enum t38_message_types_e
{
    T38_TYPE_OF_MSG_T30_INDICATOR = 0,
    T38_TYPE_OF_MSG_T30_DATA
};

enum t38_transport_types_e
{
    T38_TRANSPORT_UDPTL = 0,
    T38_TRANSPORT_RTP,
    T38_TRANSPORT_TCP
};

#define T38_RX_BUF_LEN  2048
#define T38_TX_BUF_LEN  16384

typedef struct
{
    int                 numocts;
    const uint8_t       *data;
} asn1_dyn_oct_str_t;

typedef struct
{
    uint8_t             field_data_present;
    unsigned int       field_type;
    asn1_dyn_oct_str_t  field_data;
} data_field_element_t;

typedef struct t38_core_state_s t38_core_state_t;

typedef int (t38_tx_packet_handler_t)(t38_core_state_t *s, void *user_data, const uint8_t *buf, int len, int count);

typedef int (t38_rx_indicator_handler_t)(t38_core_state_t *s, void *user_data, int indicator);
typedef int (t38_rx_data_handler_t)(t38_core_state_t *s, void *user_data, int data_type, int field_type, const uint8_t *buf, int len);
typedef int (t38_rx_missing_handler_t)(t38_core_state_t *s, void *user_data, int rx_seq_no, int expected_seq_no);

#include <sys/time.h>

/*
    Core T.38 state, common to all modes of T.38.
*/
struct t38_core_state_s
{
    t38_tx_packet_handler_t *tx_packet_handler;
    void *tx_packet_user_data;

    t38_rx_indicator_handler_t *rx_indicator_handler;
    t38_rx_data_handler_t *rx_data_handler;
    t38_rx_missing_handler_t *rx_missing_handler;
    void *rx_user_data;

    /*! NOTE - Bandwidth reduction shall only be done on suitable Phase C data, i.e., MH, MR
        and - in the case of transcoding to JBIG - MMR. MMR and JBIG require reliable data
        transport such as that provided by TCP. When transcoding is selected, it shall be
        applied to every suitable page in a call. */

    /*! Method 1: Local generation of TCF (required for use with TCP).
        Method 2: Transfer of TCF is required for use with UDP (UDPTL or RTP).
        Method 2 is not recommended for use with TCP. */
    int data_rate_management_method;
    
    /*! The emitting gateway may indicate a preference for either UDP/UDPTL, or
        UDP/RTP, or TCP for transport of T.38 IFP Packets. The receiving device
        selects the transport protocol. */
    int data_transport_protocol;

    /*! Indicates the capability to remove and insert fill bits in Phase C, non-ECM
        data to reduce bandwidth in the packet network. Optional. See Note. */
    int fill_bit_removal;

    /*! Indicates the ability to convert to/from MMR from/to the line format to
        improve the compression of the data, and reduce the bandwidth, in the
        packet network. Optional. See Note. */
    int mmr_transcoding;

    /*! Indicates the ability to convert to/from JBIG to reduce bandwidth. Optional.
        See Note. */
    int jbig_transcoding;

    /*! For UDP (UDPTL or RTP) modes, this option indicates the maximum
        number of octets that can be stored on the remote device before an overflow
        condition occurs. It is the responsibility of the transmitting application to
        limit the transfer rate to prevent an overflow. The negotiated data rate
        should be used to determine the rate at which data is being removed from
        the buffer. */
    int max_buffer_size;

    /*! This option indicates the maximum size of a UDPTL packet or the
        maximum size of the payload within an RTP packet that can be accepted by
        the remote device. */
    int max_datagram_size;

    /*! This is the version number of ITU-T Rec. T.38. New versions shall be
        compatible with previous versions. */
    int t38_version;
    
    /*! The fastest data rate supported by the T.38 channel. */
    int fastest_image_data_rate;
    
    /*! Internet aware FAX (IAF) mode. */
    int iaf;

    int tx_seq_no;
    int rx_expected_seq_no;

    int current_rx_indicator;
    int current_tx_indicator;

    int missing_packets;

    logging_state_t logging;
};

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Convert the code for an indicator to a short text name.
    \param indicator The type of indicator.
    \return A pointer to a short text name for the indicator. */
const char *t38_indicator(int indicator);

/*! \brief Convert the code for a type of data to a short text name.
    \param data_type The data type.
    \return A pointer to a short text name for the data type. */
const char *t38_data_type(int data_type);

/*! \brief Convert the code for a type of data field to a short text name.
    \param field_type The field type.
    \return A pointer to a short text name for the field type. */
const char *t38_field_type(int field_type);

int t38_core_send_indicator(t38_core_state_t *s, int indicator, int count);

int t38_core_send_data(t38_core_state_t *s, int data_type, int field_type, const uint8_t *msg, int msglen);

/*! \brief Process a received T.38 IFP packet.
    \param s The T.38 context.
    \param seq_no The packet sequence number.
    \param buf The packet contents.
    \param len The length of the packet contents.
    \return 0 for OK, else -1. */
int t38_core_rx_ifp_packet(t38_core_state_t *s, int seq_no, const uint8_t *buf, int len);

/*! Set the method to be used for data rate management, as per the T.38 spec.
    \param s The T.38 context.
    \param method 1 for pass TCF across the T.38 link, 2 for handle TCF locally.
*/
void t38_set_data_rate_management_method(t38_core_state_t *s, int method);

/*! Set the data transport protocol.
    \param s The T.38 context.
    \param data_transport_protocol UDPTL, RTP or TPKT.
*/
void t38_set_data_transport_protocol(t38_core_state_t *s, int data_transport_protocol);

/*! Set the non-ECM fill bit removal mode.
    \param s The T.38 context.
    \param fill_bit_removal TRUE to remove fill bits across the T.38 link, else FALSE.
*/
void t38_set_fill_bit_removal(t38_core_state_t *s, int fill_bit_removal);

/*! Set the MMR transcoding mode.
    \param s The T.38 context.
    \param mmr_transcoding TRUE to transcode to MMR across the T.38 link, else FALSE.
*/
void t38_set_mmr_transcoding(t38_core_state_t *s, int mmr_transcoding);

/*! Set the JBIG transcoding mode.
    \param s The T.38 context.
    \param jbig_transcoding TRUE to transcode to JBIG across the T.38 link, else FALSE.
*/
void t38_set_jbig_transcoding(t38_core_state_t *s, int jbig_transcoding);

void t38_set_max_buffer_size(t38_core_state_t *s, int max_buffer_size);

void t38_set_max_datagram_size(t38_core_state_t *s, int max_datagram_size);

int t38_get_fastest_image_data_rate(t38_core_state_t *s);

/*! Set the T.38 version to be emulated.
    \param s The T.38 context.
    \param t38_version Version number, as in the T.38 spec.
*/
void t38_set_t38_version(t38_core_state_t *s, int t38_version);

t38_core_state_t *t38_core_init(t38_core_state_t *s,
                                t38_rx_indicator_handler_t *rx_indicator_handler,
                                t38_rx_data_handler_t *rx_data_handler,
                                t38_rx_missing_handler_t *rx_missing_handler,
                                void *rx_user_data);
#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
