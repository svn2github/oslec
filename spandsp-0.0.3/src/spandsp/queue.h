/*
 * SpanDSP - a series of DSP components for telephony
 *
 * queue.h - simple in process message queuing
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2004 Steve Underwood
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
 * $Id: queue.h,v 1.4 2006/10/24 13:45:28 steveu Exp $
 */

/*! \file */

/*! \page queue_page Queuing
\section queue_page_sec_1 What does it do?
???.

\section queue_page_sec_2 How does it work?
???.
*/

#if !defined(_QUEUE_H_)
#define _QUEUE_H_

#define QUEUE_READ_ATOMIC   0x0001
#define QUEUE_WRITE_ATOMIC  0x0002

typedef struct
{
    int len;
    int iptr;
    int optr;
    int flags;
    uint8_t *data;
} queue_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! Check if a queue is empty.
    \brief Check if a queue is empty.
    \param p The queue context.
    \return TRUE if empty, else FALSE. */
int queue_empty(queue_t *p);

/*! Check the available free space in a queue's buffer.
    \brief Check available free space.
    \param p The queue context.
    \return The number of bytes of free space. */
int queue_free_space(queue_t *p);

/*! Check the contents of a queue.
    \brief Check the contents of a queue.
    \param p The queue context.
    \return The number of bytes in the queue. */
int queue_contents(queue_t *p);

/*! Flush the contents of a queue.
    \brief Flush the contents of a queue.
    \param p The queue context. */
void queue_flush(queue_t *p);

/*! Copy bytes from a queue. This is similar to queue_read, but
    the data remains in the queue.
    \brief Copy bytes from a queue.
    \param p The queue context.
    \param buf The buffer into which the bytes will be read.
    \param len The length of the buffer.
    \return the number of bytes returned. */
int queue_view(queue_t *p, uint8_t *buf, int len);

/*! Read bytes from a queue.
    \brief Read bytes from a queue.
    \param p The queue context.
    \param buf The buffer into which the bytes will be read.
    \param len The length of the buffer.
    \return the number of bytes returned. */
int queue_read(queue_t *p, uint8_t *buf, int len);

/*! Write bytes to a queue.
    \brief Write bytes to a queue.
    \param p The queue context.
    \param buf The buffer containing the bytes to be written.
    \param len The length of the buffer.
    \return the number of bytes actually written. */
int queue_write(queue_t *p, const uint8_t *buf, int len);

/*! Test the length of the message at the head of a queue.
    \brief Test message length.
    \param p The queue context.
    \return The length of the next message, in byte. If there are
            no messages in the queue, -1 is returned. */
int queue_test_msg(queue_t *p);

/*! Read a message from a queue. If the message is longer than the buffer
    provided, only the first len bytes of the message will be returned. The
    remainder of the message will be discarded.
    \brief Read a message from a queue.
    \param p The queue context.
    \param buf The buffer into which the message will be read.
    \param len The length of the buffer.
    \return The number of bytes returned. If there are
            no messages in the queue, -1 is returned. */
int queue_read_msg(queue_t *p, uint8_t *buf, int len);

/*! Write a message to a queue.
    \brief Write a message to a queue.
    \param p The queue context.
    \param buf The buffer from which the message will be written.
    \param len The length of the message.
    \return The number of bytes actually written. */
int queue_write_msg(queue_t *p, const uint8_t *buf, int len);

/*! Create a queue.
    \brief Create a queue.
    \param p The queue context.
    \param len The length of the queue's buffer.
    \param flags Flags controlling the operation of the queue.
           Valid flags are QUEUE_READ_ATOMIC and QUEUE_WRITE_ATOMIC.
    \return 0 if created OK, else -1. */
int queue_create(queue_t *p, int len, int flags);

/*! Delete a queue.
    \brief Delete a queue.
    \param p The queue context.
    \return 0 if deleted OK, else -1. */
int queue_delete(queue_t *p);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
