/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t30.h - definitions for T.30 fax processing
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
 * $Id: t30.h,v 1.61 2006/11/01 12:58:21 steveu Exp $
 */

/*! \file */

#if !defined(_T30_H_)
#define _T30_H_

/*! \page t30_page T.30 FAX protocol handling

\section t30_page_sec_1 What does it do?
The T.30 protocol is the core protocol used for FAX transmission. This module
implements most of its key featrues. It does not interface to the outside work.
Seperate modules do that for T.38, analogue line, and other forms of FAX
communication.

Current features of this module include:

    - FAXing to and from multi-page TIFF/F files, whose images are one of the standard
      FAX sizes.
    - T.4 1D (MH), T.4 2D,(MR) and T.6 (MMR) compression.
    - Error correction (ECM)
    - All standard resolutions and page sizes

\section t30_page_sec_2 How does it work?

Some of the following is paraphrased from some notes found a while ago on the Internet.
I cannot remember exactly where they came from, but they are useful.

The answer (CED) tone

The T.30 standard says an answering fax device must send CED (a 2100Hz tone) for
approximately 3 seconds before sending the first handshake message. Some machines
send an 1100Hz or 1850Hz tone, and some send no tone at all. In fact, this answer
tone is so unpredictable, it cannot really be used. It should, however, always be
generated according to the specification.

Common Timing Deviations

The T.30 spec. specifies a number of time-outs. For example, after dialing a number,
a calling fax system should listen for a response for 35 seconds before giving up.
These time-out periods are as follows: 

    * T1 - 35+-5s: the maximum time for which two fax system will attempt to identify each other
    * T2 - 6+-1s:  a time-out used to start the sequence for changing transmit parameters
    * T3 - 10+-5s: a time-out used in handling operator interrupts
    * T5 - 60+-5s: a time-out used in error correction mode

These time-outs are sometimes misinterpreted. In addition, they are routinely
ignored, sometimes with good reason. For example, after placing a call, the
calling fax system is supposed to wait for 35 seconds before giving up. If the
answering unit does not answer on the first ring or if a voice answering machine
is connected to the line, or if there are many delays through the network,
the delay before answer can be much longer than 35 seconds. 

Fax units that support error correction mode (ECM) can respond to a post-image
handshake message with a receiver not ready (RNR) message. The calling unit then
queries the receiving fax unit with a receiver ready (RR) message. If the
answering unit is still busy (printing for example), it will repeat the RNR
message. According to the T.30 standard, this sequence (RR/RNR RR/RNR) can be
repeated for up to the end of T5 (60+-5s). However, many fax systems
ignore the time-out and will continue the sequence indefinitely, unless the user
manually overrides. 

All the time-outs are subject to alteration, and sometimes misuse. Good T.30
implementations must do the right thing, and tolerate others doing the wrong thing.
 
Variations in the inter-carrier gap

T.30 specifies 75+-20ms of silence between signals using different modulation
schemes. Examples are between the end of a DCS signal and the start of a TCF signal,
and between the end of an image and the start of a post-image signal. Many fax systems
violate this requirement, especially for the silent period between DCS and TCF.
This may be stretched to well over 100ms. If this period is too long, it can interfere with
handshake signal error recovery, should a packet be corrupted on the line. Systems
should ensure they stay within the prescribed T.30 limits, and be tolerant of others
being out of spec.. 

Other timing variations

Testing is required to determine the ability of a fax system to handle
variations in the duration of pauses between unacknowledged handshake message
repetitions, and also in the pauses between the receipt of a handshake command and
the start of a response to that command. In order to reduce the total
transmission time, many fax systems start sending a response message before the
end of the command has been received. 

Other deviations from the T.30 standard

There are many other commonly encountered variations between machines, including:

    * frame sequence deviations
    * preamble and flag sequence variations
    * improper EOM usage
    * unusual data rate fallback sequences
    * common training pattern detection algorithms
    * image transmission deviations
    * use of the talker echo protect tone
    * image padding and short lines
    * RTP/RTN handshake message usage
    * long duration lines
    * nonstandard disconnect sequences
    * DCN usage
*/

#define T30_MAX_DIS_DTC_DCS_LEN     22
#define T30_MAX_IDENT_LEN           21

typedef struct t30_state_s t30_state_t;

/*!
    T.30 phase B callback handler.
    \brief T.30 phase B callback handler.
    \param s The T.30 context.
    \param user_data An opaque pointer.
    \param result The phase B event code.
*/
typedef void (t30_phase_b_handler_t)(t30_state_t *s, void *user_data, int result);

/*!
    T.30 phase D callback handler.
    \brief T.30 phase D callback handler.
    \param s The T.30 context.
    \param user_data An opaque pointer.
    \param result The phase D event code.
*/
typedef void (t30_phase_d_handler_t)(t30_state_t *s, void *user_data, int result);

/*!
    T.30 phase E callback handler.
    \brief T.30 phase E callback handler.
    \param s The T.30 context.
    \param user_data An opaque pointer.
    \param completion_code The phase E completion code.
*/
typedef void (t30_phase_e_handler_t)(t30_state_t *s, void *user_data, int completion_code);

/*!
    T.30 document handler.
    \brief T.30 document handler.
    \param s The T.30 context.
    \param user_data An opaque pointer.
    \param result The document event code.
*/
typedef int (t30_document_handler_t)(t30_state_t *s, void *user_data, int status);

/*!
    T.30 set a receive or transmit type handler.
    \brief T.30 set a receive or transmit type handler.
    \param user_data An opaque pointer.
    \param type The modem, tone or silence to be sent or received.
    \param short_train TRUE if the short training sequence should be used (where one exists).
    \param use_hdlc FALSE for bit stream, TRUE for HDLC framing.
*/
typedef void (t30_set_handler_t)(void *user_data, int type, int short_train, int use_hdlc);

/*!
    T.30 send HDLC handler.
    \brief T.30 send HDLC handler.
    \param user_data An opaque pointer.
    \param msg The HDLC message.
    \param len The length of the message.
*/
typedef void (t30_send_hdlc_handler_t)(void *user_data, const uint8_t *msg, int len);

/*!
    T.30 protocol completion codes, at phase E.
*/
enum
{
    T30_ERR_OK = 0,         /* OK */

    /* External problems */
    T30_ERR_CEDTONE,        /* The CED tone exceeded 5s */
    T30_ERR_T0EXPIRED,      /* Timed out waiting for initial communication */
    T30_ERR_T1EXPIRED,      /* Timed out waiting for the first message */
    T30_ERR_T3EXPIRED,      /* Timed out waiting for procedural interrupt */
    T30_ERR_HDLCCARR,       /* The HDLC carrier did not stop in a timely manner */
    T30_ERR_CANNOTTRAIN,    /* Failed to train with any of the compatible modems */
    T30_ERR_OPERINTFAIL,    /* Operator intervention failed */
    T30_ERR_INCOMPATIBLE,   /* Far end is not compatible */
    T30_ERR_NOTRXCAPABLE,   /* Far end is not receive capable */
    T30_ERR_NOTTXCAPABLE,   /* Far end is not transmit capable */
    T30_ERR_UNEXPECTED,     /* Unexpected message received */
    T30_ERR_NORESSUPPORT,   /* Far end cannot receive at the resolution of the image */
    T30_ERR_NOSIZESUPPORT,  /* Far end cannot receive at the size of image */

    /* Internal problems */
    T30_ERR_FILEERROR,      /* TIFF/F file cannot be opened */
    T30_ERR_NOPAGE,         /* TIFF/F page not found */
    T30_ERR_BADTIFF,        /* TIFF/F format is not compatible */
    T30_ERR_UNSUPPORTED,    /* Unsupported feature */

    /* Phase E status values returned to a transmitter */
    T30_ERR_BADDCSTX,       /* Received bad response to DCS or training */
    T30_ERR_BADPGTX,        /* Received a DCN from remote after sending a page */
    T30_ERR_ECMPHDTX,       /* Invalid ECM response received from receiver */
    T30_ERR_ECMRNRTX,       /* Timer T5 expired, receiver not ready */
    T30_ERR_GOTDCNTX,       /* Received a DCN while waiting for a DIS */
    T30_ERR_INVALRSPTX,     /* Invalid response after sending a page */
    T30_ERR_NODISTX,        /* Received other than DIS while waiting for DIS */
    T30_ERR_NXTCMDTX,       /* Timed out waiting for next send_page command from driver */
    T30_ERR_PHBDEADTX,      /* Received no response to DCS, training or TCF */
    T30_ERR_PHDDEADTX,      /* No response after sending a page */

    /* Phase E status values returned to a receiver */
    T30_ERR_ECMPHDRX,       /* Invalid ECM response received from transmitter */
    T30_ERR_GOTDCSRX,       /* DCS received while waiting for DTC */
    T30_ERR_INVALCMDRX,     /* Unexpected command after page received */
    T30_ERR_NOCARRIERRX,    /* Carrier lost during fax receive */
    T30_ERR_NOEOLRX,        /* Timed out while waiting for EOL (end Of line) */
    T30_ERR_NOFAXRX,        /* Timed out while waiting for first line */
    T30_ERR_NXTCMDRX,       /* Timed out waiting for next receive page command */
    T30_ERR_T2EXPDCNRX,     /* Timer T2 expired while waiting for DCN */
    T30_ERR_T2EXPDRX,       /* Timer T2 expired while waiting for phase D */
    T30_ERR_T2EXPFAXRX,     /* Timer T2 expired while waiting for fax page */
    T30_ERR_T2EXPMPSRX,     /* Timer T2 expired while waiting for next fax page */
    T30_ERR_T2EXPRRRX,      /* Timer T2 expired while waiting for RR command */
    T30_ERR_T2EXPRX,        /* Timer T2 expired while waiting for NSS, DCS or MCF */
    T30_ERR_DCNWHYRX,       /* Unexpected DCN while waiting for DCS or DIS */
    T30_ERR_DCNDATARX,      /* Unexpected DCN while waiting for image data */
    T30_ERR_DCNFAXRX,       /* Unexpected DCN while waiting for EOM, EOP or MPS */
    T30_ERR_DCNPHDRX,       /* Unexpected DCN after EOM or MPS sequence */
    T30_ERR_DCNRRDRX,       /* Unexpected DCN after RR/RNR sequence */
    T30_ERR_DCNNORTNRX,     /* Unexpected DCN after requested retransmission */

    T30_ERR_BADPAGE,        /* TIFF/F page number tag missing */
    T30_ERR_BADTAG,         /* Incorrect values for TIFF/F tags */
    T30_ERR_BADTIFFHDR,     /* Bad TIFF/F header - incorrect values in fields */
    T30_ERR_BADPARM,        /* Invalid value for fax parameter */
    T30_ERR_BADSTATE,       /* Invalid initial state value specified */
    T30_ERR_CMDDATA,        /* Last command contained invalid data */
    T30_ERR_DISCONNECT,     /* Fax call disconnected by the other station */
    T30_ERR_INVALARG,       /* Illegal argument to function */
    T30_ERR_INVALFUNC,      /* Illegal call to function */
    T30_ERR_NODATA,         /* Data requested is not available (NSF, DIS, DCS) */
    T30_ERR_NOMEM,          /* Cannot allocate memory for more pages */
    T30_ERR_NOPOLL,         /* Poll not accepted */
    T30_ERR_NOSTATE,        /* Initial state value not set */
    T30_ERR_RETRYDCN,       /* Disconnected after permitted retries */
    T30_ERR_CALLDROPPED     /* The call dropped prematurely */
};

/*!
    I/O modes for the T.30 protocol.
*/
enum
{
    T30_MODEM_NONE = 0,
    T30_MODEM_PAUSE,
    T30_MODEM_CED,
    T30_MODEM_CNG,
    T30_MODEM_V21,
    T30_MODEM_V27TER_2400,
    T30_MODEM_V27TER_4800,
    T30_MODEM_V29_7200,
    T30_MODEM_V29_9600,
    T30_MODEM_V17_7200,
    T30_MODEM_V17_9600,
    T30_MODEM_V17_12000,
    T30_MODEM_V17_14400,
    T30_MODEM_DONE
};

enum
{
    T30_SUPPORT_V27TER = 0x01,
    T30_SUPPORT_V29 = 0x02,
    T30_SUPPORT_V17 = 0x04,
    T30_SUPPORT_V34 = 0x08,
    T30_SUPPORT_IAF = 0x10,
};

enum
{
    T30_SUPPORT_NO_COMPRESSION = 0x01,
    T30_SUPPORT_T4_1D_COMPRESSION = 0x02,
    T30_SUPPORT_T4_2D_COMPRESSION = 0x04,
    T30_SUPPORT_T6_COMPRESSION = 0x08,
    T30_SUPPORT_T85_COMPRESSION = 0x10,     /* Monochrome JBIG */
    T30_SUPPORT_T43_COMPRESSION = 0x20,     /* Colour JBIG */
    T30_SUPPORT_T45_COMPRESSION = 0x40      /* Run length colour compression */
};

enum
{
    T30_SUPPORT_STANDARD_RESOLUTION = 0x01,
    T30_SUPPORT_FINE_RESOLUTION = 0x02,
    T30_SUPPORT_SUPERFINE_RESOLUTION = 0x04,

    T30_SUPPORT_R4_RESOLUTION = 0x10000,
    T30_SUPPORT_R8_RESOLUTION = 0x20000,
    T30_SUPPORT_R16_RESOLUTION = 0x40000,

    T30_SUPPORT_300_300_RESOLUTION = 0x100000,
    T30_SUPPORT_400_400_RESOLUTION = 0x200000,
    T30_SUPPORT_600_600_RESOLUTION = 0x400000,
    T30_SUPPORT_1200_1200_RESOLUTION = 0x800000,
    T30_SUPPORT_300_600_RESOLUTION = 0x1000000,
    T30_SUPPORT_400_800_RESOLUTION = 0x2000000,
    T30_SUPPORT_600_1200_RESOLUTION = 0x4000000
};

enum
{
    T30_SUPPORT_215MM_WIDTH = 0x01,
    T30_SUPPORT_255MM_WIDTH = 0x02,
    T30_SUPPORT_303MM_WIDTH = 0x04,

    T30_SUPPORT_UNLIMITED_LENGTH = 0x10000,
    T30_SUPPORT_A4_LENGTH = 0x20000,
    T30_SUPPORT_B4_LENGTH = 0x40000,
    T30_SUPPORT_US_LETTER_LENGTH = 0x80000,
    T30_SUPPORT_US_LEGAL_LENGTH = 0x100000
};

enum
{
    T30_SUPPORT_SEP = 0x01,
    T30_SUPPORT_PSA = 0x02
};

enum
{
    T30_IAF_MODE_T37 = 0x01,
    T30_IAF_MODE_T38 = 0x02,
    T30_IAF_MODE_FLOW_CONTROL = 0x04,
    /*! Continuous flow mode means data is sent as fast as possible, usually across
        the Internet, where speed is not constrained by a PSTN modem. */
    T30_IAF_MODE_CONTINUOUS_FLOW = 0x08,
    /*! No TCF means TCF is not exchanged. The end points must sort out usable speed
        issues locally. */
    T30_IAF_MODE_NO_TCF = 0x10,
    /*! No fill bits means do not insert fill bits, even if the T.30 messages request
        them. */
    T30_IAF_MODE_NO_FILL_BITS = 0x20
};

/*!
    T.30 FAX channel descriptor. This defines the state of a single working
    instance of a T.30 FAX channel.
*/
struct t30_state_s
{
    /* This must be kept the first thing in the structure, so it can be pointed
       to reliably as the structures change over time. */
    t4_state_t t4;

    /*! \brief TRUE is behaving as the calling party */
    int calling_party;

    /*! \brief The local identifier string. */
    char local_ident[T30_MAX_IDENT_LEN];
    /*! \brief The identifier string supplied by the remote FAX machine. */
    char far_ident[T30_MAX_IDENT_LEN];
    /*! \brief The sub-address string to be sent to the remote FAX machine. */
    char local_sub_address[T30_MAX_IDENT_LEN];
    /*! \brief The sub-address string supplied by the remote FAX machine. */
    char far_sub_address[T30_MAX_IDENT_LEN];
    /*! \brief The selective polling sub-address supplied by the remote FAX machine. */
    char sep_address[T30_MAX_IDENT_LEN];
    /*! \brief The polled sub-address supplied by the remote FAX machine. */
    char psa_address[T30_MAX_IDENT_LEN];
    /*! \brief A password to be associated with the T.30 context. */
    char local_password[T30_MAX_IDENT_LEN];
    /*! \brief A password expected from the far end. */
    char far_password[T30_MAX_IDENT_LEN];
    /*! \brief The text which will be used in FAX page header. No text results
               in no header line. */
    char header_info[50 + 1];
    /*! \brief The country of origin of the remote machine, if known, else NULL. */
    const char *country;
    /*! \brief The vendor of the remote machine, if known, else NULL. */
    const char *vendor;
    /*! \brief The model of the remote machine, if known, else NULL. */
    const char *model;
    uint8_t local_nsf[100];
    int local_nsf_len;

    /*! \brief A pointer to a callback routine to be called when phase B events
        occur. */
    t30_phase_b_handler_t *phase_b_handler;
    /*! \brief An opaque pointer supplied in event B callbacks. */
    void *phase_b_user_data;
    /*! \brief A pointer to a callback routine to be called when phase D events
        occur. */
    t30_phase_d_handler_t *phase_d_handler;
    /*! \brief An opaque pointer supplied in event D callbacks. */
    void *phase_d_user_data;
    /*! \brief A pointer to a callback routine to be called when phase E events
        occur. */
    t30_phase_e_handler_t *phase_e_handler;
    /*! \brief An opaque pointer supplied in event E callbacks. */
    void *phase_e_user_data;

    /*! \brief A pointer to a callback routine to be called when document events
        (e.g. end of transmitted document) occur. */
    t30_document_handler_t *document_handler;
    /*! \brief An opaque pointer supplied in document callbacks. */
    void *document_user_data;

    t30_set_handler_t *set_rx_type_handler;
    void *set_rx_type_user_data;
    t30_set_handler_t *set_tx_type_handler;
    void *set_tx_type_user_data;

    t30_send_hdlc_handler_t *send_hdlc_handler;
    void *send_hdlc_user_data;
   
    int phase;
    int next_phase;
    int state;
    int step;

    uint8_t dcs_frame[T30_MAX_DIS_DTC_DCS_LEN];
    int dcs_len;
    uint8_t dis_dtc_frame[T30_MAX_DIS_DTC_DCS_LEN];
    int dis_dtc_len;
    /*! \brief TRUE if a valid DIS has been received from the far end. */
    int dis_received;
    /*! \brief TRUE if a valid passwrod has been received from the far end. */
    int far_password_ok;

    /*! \brief A flag to indicate a message is in progress. */
    int in_message;

    /*! \brief TRUE if the short training sequence should be used. */
    int short_train;

    /*! \brief A count of the number of bits in the trainability test. */
    int training_test_bits;
    int training_current_zeros;
    int training_most_zeros;

    /*! \brief The current fallback step for the fast message transfer modem. */
    int current_fallback;
    /*! \brief TRUE if a carrier is present. Otherwise FALSE. */
    int rx_signal_present;
    /*! \brief TRUE if a modem has trained correctly. */
    int rx_trained;
    int current_rx_type;
    int current_tx_type;

    /*! \brief T0 is the answer timeout when calling another FAX machine.
        Placing calls is handled outside the FAX processing, but this timeout keeps
        running until V.21 modulation is sent or received.
        T1 is the remote terminal identification timeout (in audio samples). */
    int timer_t0_t1;
    /*! \brief T2 is the HDLC command timeout.
               T4 is the HDLC response timeout (in audio samples). */
    int timer_t2_t4;
    /*! \brief TRUE if the T2/T4 timer is actually timing T4 */
    int timer_is_t4;
    /*! \brief Procedural interrupt timeout (in audio samples). */
    int timer_t3;
    /*! \brief This is only used in error correcting mode. */
    int timer_t5;
    /*! \brief This is only used in full duplex (e.g. ISDN) modes. */
    int timer_t6;
    /*! \brief This is only used in full duplex (e.g. ISDN) modes. */
    int timer_t7;
    /*! \brief This is only used in full duplex (e.g. ISDN) modes. */
    int timer_t8;

    int far_end_detected;

    int local_interrupt_pending;
    int crp_enabled;
    int line_encoding;
    int min_row_bits;
    int x_resolution;
    int y_resolution;
    int image_width;
    int retries;
    int error_correcting_mode;
    int ppr_count;
    int octets_per_ecm_frame;
    uint8_t ecm_data[256][260];
    int16_t ecm_len[256];
    uint8_t ecm_frame_map[3 + 32];
    int ecm_page;
    int ecm_block;
    int ecm_frames;
    int ecm_current_frame;
    int ecm_at_page_end;
    int next_tx_step;
    int next_rx_step;
    char rx_file[256];
    int rx_stop_page;
    char tx_file[256];
    int tx_start_page;
    int tx_stop_page;
    int current_status;
    int iaf;
    int supported_modems;
    int supported_compressions;
    int supported_resolutions;
    int supported_image_sizes;
    int supported_polling_features;
    int ecm_allowed;
    /*! \brief Error and flow logging control */
    logging_state_t logging;
};

typedef struct
{
    /*! \brief The current bit rate for image transfer. */
    int bit_rate;
    /*! \brief TRUE if error correcting mode is used. */
    int error_correcting_mode;
    /*! \brief The number of pages transferred so far. */
    int pages_transferred;
    /*! \brief The number of horizontal pixels in the most recent page. */
    int width;
    /*! \brief The number of vertical pixels in the most recent page. */
    int length;
    /*! \brief The number of bad pixel rows in the most recent page. */
    int bad_rows;
    /*! \brief The largest number of bad pixel rows in a block in the most recent page. */
    int longest_bad_row_run;
    /*! \brief The horizontal column-to-column resolution of the page in pixels per metre */
    int x_resolution;
    /*! \brief The vertical row-to-row resolution of the page in pixels per metre */
    int y_resolution;
    /*! \brief The type of compression used between the FAX machines */
    int encoding;
    /*! \brief The size of the image, in bytes */
    int image_size;
    /*! \brief Current status */
    int current_status;
} t30_stats_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! Initialise a T.30 context.
    \brief Initialise a T.30 context.
    \param s The T.30 context.
    \param calling_party TRUE if the context is for a calling party. FALSE if the
           context is for an answering party.
    \return 0 for OK, else -1. */
int t30_init(t30_state_t *s,
             int calling_party,
             t30_set_handler_t *set_rx_type_handler,
             void *set_rx_type_user_data,
             t30_set_handler_t *set_tx_type_handler,
             void *set_tx_type_user_data,
             t30_send_hdlc_handler_t *send_hdlc_handler,
             void *send_hdlc_user_data);

/*! Release a T.30 context.
    \brief Release a T.30 context.
    \param s The T.30 context. */
void t30_release(t30_state_t *s);

/*! Restart a T.30 context.
    \brief Restart a T.30 context.
    \param s The T.30 context.
    \return 0 for OK, else -1. */
int t30_restart(t30_state_t *s);

/*! Create and initialise a T.30 context.
    \brief Create and initialise a T.30 context.
    \param calling_party TRUE if the context is for a calling party. FALSE if the
           context is for an answering party.
    \return A pointer to the FAX context, or NULL if one could not be created.
*/
t30_state_t *t30_create(int calling_party,
                        t30_set_handler_t *set_rx_type_handler,
                        void *set_rx_type_user_data,
                        t30_set_handler_t *set_tx_type_handler,
                        void *set_tx_type_user_data,
                        t30_send_hdlc_handler_t *send_hdlc_handler,
                        void *send_hdlc_user_data);

/*! Free a T.30 context.
    \brief Free a T.30 context.
    \param s The T.30 context. */
void t30_free(t30_state_t *s);

/*! Cleanup a T.30 context if the call terminates.
    \brief Cleanup a T.30 context if the call terminates.
    \param s The T.30 context. */
void t30_terminate(t30_state_t *s);

/*! Return a text name for a T.30 frame type.
    \brief Return a text name for a T.30 frame type.
    \param x The frametype octet.
    \return A pointer to the text name for the frame type. If the frame type is
            not value, the string "???" is returned. */
const char *t30_frametype(uint8_t x);

/*! Decode a DIS, DTC or DCS frame, and log the contents.
    \brief Decode a DIS, DTC or DCS frame, and log the contents.
    \param s The T.30 context.
    \param dis A pointer to the frame to be decoded.
    \param len The length of the frame. */
void t30_decode_dis_dtc_dcs(t30_state_t *s, const uint8_t *dis, int len);

/*! Convert a phase E completion code to a short text description.
    \brief Convert a phase E completion code to a short text description.
    \param result The result code.
    \return A pointer to the description. */
const char *t30_completion_code_to_str(int result);

/*! Set Internet aware FAX (IAF) mode.
    \brief Set Internet aware FAX (IAF) mode.
    \param s The T.30 context.
    \param iaf TRUE for IAF, or FALSE for non-IAF. */
void t30_set_iaf_mode(t30_state_t *s, int iaf);

/*! Set the sub-address associated with a T.30 context.
    \brief Set the sub-address associated with a T.30 context.
    \param s The T.30 context.
    \param sub_address A pointer to the sub-address.
    \return 0 for OK, else -1. */
int t30_set_local_sub_address(t30_state_t *s, const char *sub_address);

/*! Set the header information associated with a T.30 context.
    \brief Set the header information associated with a T.30 context.
    \param s The T.30 context.
    \param info A pointer to the information string.
    \return 0 for OK, else -1. */
int t30_set_header_info(t30_state_t *s, const char *info);

/*! Set the local identifier associated with a T.30 context.
    \brief Set the local identifier associated with a T.30 context.
    \param s The T.30 context.
    \param id A pointer to the identifier.
    \return 0 for OK, else -1. */
int t30_set_local_ident(t30_state_t *s, const char *id);

int t30_set_local_nsf(t30_state_t *s, const uint8_t *nsf, int len);

/*! Get the sub-address associated with a T.30 context.
    \brief Get the sub-address associated with a T.30 context.
    \param s The T.30 context.
    \param sub_address A pointer to a buffer for the sub-address.  The buffer
           should be at least 21 bytes long.
    \return the length of the string. */
size_t t30_get_sub_address(t30_state_t *s, char *sub_address);

/*! Get the header information associated with a T.30 context.
    \brief Get the header information associated with a T.30 context.
    \param s The T.30 context.
    \param sub_address A pointer to a buffer for the header information.  The buffer
           should be at least 51 bytes long.
    \return the length of the string. */
size_t t30_get_header_info(t30_state_t *s, char *info);

/*! Get the local FAX machine identifier associated with a T.30 context.
    \brief Get the local identifier associated with a T.30 context.
    \param s The T.30 context.
    \param id A pointer to a buffer for the identifier. The buffer should
           be at least 21 bytes long.
    \return the length of the string. */
size_t t30_get_local_ident(t30_state_t *s, char *id);

/*! Get the remote FAX machine identifier associated with a T.30 context.
    \brief Get the remote identifier associated with a T.30 context.
    \param s The T.30 context.
    \param id A pointer to a buffer for the identifier. The buffer should
           be at least 21 bytes long.
    \return the length of the string. */
size_t t30_get_far_ident(t30_state_t *s, char *id);

/*! Get the country of origin of the remote FAX machine associated with a T.30 context.
    \brief Get the country of origin of the remote FAX machine associated with a T.30 context.
    \param s The T.30 context.
    \return a pointer to the country name, or NULL if the country is not known. */
const char *t30_get_far_country(t30_state_t *s);

/*! Get the name of the vendor of the remote FAX machine associated with a T.30 context.
    \brief Get the name of the vendor of the remote FAX machine associated with a T.30 context.
    \param s The T.30 context.
    \return a pointer to the vendor name, or NULL if the vendor is not known. */
const char *t30_get_far_vendor(t30_state_t *s);

/*! Get the name of the model of the remote FAX machine associated with a T.30 context.
    \brief Get the name of the model of the remote FAX machine associated with a T.30 context.
    \param s The T.30 context.
    \return a pointer to the model name, or NULL if the model is not known. */
const char *t30_get_far_model(t30_state_t *s);

/*! Get the current transfer statistics for the file being sent or received.
    \brief Get the current transfer statistics.
    \param s The T.30 context.
    \param t A pointer to a buffer for the statistics. */
void t30_get_transfer_statistics(t30_state_t *s, t30_stats_t *t);

/*! Set a callback function for T.30 phase B handling.
    \brief Set a callback function for T.30 phase B handling.
    \param s The T.30 context.
    \param handler The callback function
    \param user_data An opaque pointer passed to the callback function. */
void t30_set_phase_b_handler(t30_state_t *s, t30_phase_b_handler_t *handler, void *user_data);

/*! Set a callback function for T.30 phase D handling.
    \brief Set a callback function for T.30 phase D handling.
    \param s The T.30 context.
    \param handler The callback function
    \param user_data An opaque pointer passed to the callback function. */
void t30_set_phase_d_handler(t30_state_t *s, t30_phase_d_handler_t *handler, void *user_data);

/*! Set a callback function for T.30 phase E handling.
    \brief Set a callback function for T.30 phase E handling.
    \param s The T.30 context.
    \param handler The callback function
    \param user_data An opaque pointer passed to the callback function. */
void t30_set_phase_e_handler(t30_state_t *s, t30_phase_e_handler_t *handler, void *user_data);

/*! Set a callback function for T.30 end of document handling.
    \brief Set a callback function for T.30 end of document handling.
    \param s The T.30 context.
    \param handler The callback function
    \param user_data An opaque pointer passed to the callback function. */
void t30_set_document_handler(t30_state_t *s, t30_document_handler_t *handler, void *user_data);

/*! Specify the file name of the next TIFF file to be received by a T.30
    context.
    \brief Set next receive file name.
    \param s The T.30 context.
    \param file The file name
    \param stop_page The maximum page to receive. -1 for no restriction. */
void t30_set_rx_file(t30_state_t *s, const char *file, int stop_page);

/*! Specify the file name of the next TIFF file to be transmitted by a T.30
    context.
    \brief Set next transmit file name.
    \param s The T.30 context.
    \param file The file name
    \param start_page The first page to send. -1 for no restriction.
    \param stop_page The last page to send. -1 for no restriction. */
void t30_set_tx_file(t30_state_t *s, const char *file, int start_page, int stop_page);

/*! Specify which modem types are supported by a T.30 context.
    \brief Specify supported modems.
    \param s The T.30 context.
    \param supported_modems Bit field list of the supported modems. */
void t30_set_supported_modems(t30_state_t *s, int supported_modems);

/*! Specify which compression types are supported by a T.30 context.
    \brief Specify supported compression types.
    \param s The T.30 context.
    \param supported_compressions Bit field list of the supported compression types. */
void t30_set_supported_compressions(t30_state_t *s, int supported_compressions);

/*! Specify which resolutions are supported by a T.30 context.
    \brief Specify supported resolutions.
    \param s The T.30 context.
    \param supported_compressions Bit field list of the supported resolutions. */
void t30_set_supported_resolutions(t30_state_t *s, int supported_resolutions);

/*! Specify which images sizes are supported by a T.30 context.
    \brief Specify supported image sizes.
    \param s The T.30 context.
    \param supported_image_sizes Bit field list of the supported widths and lengths. */
void t30_set_supported_image_sizes(t30_state_t *s, int supported_image_sizes);

/*! Specify if error correction mode (ECM) is allowed by a T.30 context.
    \brief Select ECM capability.
    \param s The T.30 context.
    \param enabled TRUE for ECM capable, FALSE for not ECM capable. */
void t30_set_ecm_capability(t30_state_t *s, int enabled);

/*! Request a local interrupt of FAX exchange.
    \brief Request a local interrupt of FAX exchange.
    \param s The T.30 context.
    \param state TRUE to enable interrupt request, else FALSE. */
void t30_local_interrupt_request(t30_state_t *s, int state);

/*! Inform the T.30 engine the current transmission has completed.
    \brief Inform the T.30 engine the current transmission has completed.
    \param s The T.30 context. */
void t30_send_complete(void *user_data);

/*! Inform the T.30 engine the current receive has completed. This is
    only needed to report an unexpected end of the receive operation, as
    might happen with T.38 dying.
    \brief Inform the T.30 engine the current receive has completed.
    \param s The T.30 context. */
void t30_receive_complete(void *user_data);

/*! Get a bit of received non-ECM image data.
    \brief Get a bit of received non-ECM image data.
    \param user_data An opaque pointer, which must point to the T.30 context.
    \return bit The next bit to transmit. */
int t30_non_ecm_get_bit(void *user_data);

/*! Process a bit of received non-ECM image data.
    \brief Process a bit of received non-ECM image data
    \param user_data An opaque pointer, which must point to the T.30 context.
    \param bit The received bit. */
void t30_non_ecm_put_bit(void *user_data, int bit);

/*! Process a byte of received non-ECM image data.
    \brief Process a byte of received non-ECM image data
    \param user_data An opaque pointer, which must point to the T.30 context.
    \param byte The received byte. */
void t30_non_ecm_putbyte(void *user_data, int byte);

/*! Process a received HDLC frame.
    \brief Process a received HDLC frame.
    \param s The T.30 context.
    \param ok TRUE if the frame was received without error.
    \param msg The HDLC message.
    \param int The length of the message, in octets. */
void t30_hdlc_accept(void *user_data, int ok, const uint8_t *msg, int len);

/*! Report the passage of time to the T.30 engine.
    \brief Report the passage of time to the T.30 engine.
    \param s The T.30 context.
    \param samples The time change in 1/8000th second steps. */
void t30_timer_update(t30_state_t *s, int samples);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
