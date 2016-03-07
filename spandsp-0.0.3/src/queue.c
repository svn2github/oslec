/*
 * SpanDSP - a series of DSP components for telephony
 *
 * queue.c - simple in process message queuing
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
 * $Id: queue.c,v 1.10 2006/11/19 14:07:25 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>

#include <tiffio.h>

#include "spandsp/queue.h"

int queue_empty(queue_t *p)
{
    return (p->iptr == p->optr);
}
/*- End of function --------------------------------------------------------*/

int queue_free_space(queue_t *p)
{
    if (p->iptr < p->optr)
        return ((p->optr - p->iptr) - 1);
    /*endif*/
    return (p->len - (p->iptr - p->optr));
}
/*- End of function --------------------------------------------------------*/

int queue_contents(queue_t *p)
{
    if (p->iptr < p->optr)
        return (p->len - (p->optr - p->iptr) + 1);
    /*endif*/
    return (p->iptr - p->optr);
}
/*- End of function --------------------------------------------------------*/

void queue_flush(queue_t *p)
{
    p->iptr =
    p->optr = 0;
}
/*- End of function --------------------------------------------------------*/

int queue_view(queue_t *p, uint8_t *buf, int len)
{
    int real_len;
    int to_end;

    real_len = queue_contents(p);
    if (real_len < len)
    {
        if (p->flags & QUEUE_READ_ATOMIC)
            return -1;
        /*endif*/
    }
    else
    {
        real_len = len;
    }
    /*endif*/
    if (real_len == 0)
        return 0;
    /*endif*/
    to_end = p->len + 1 - p->optr;
    if (p->iptr < p->optr  &&  to_end < real_len)
    {
        /* A two step process */
        if (buf)
        {
            memcpy(buf, p->data + p->optr, to_end);
            memcpy(buf + to_end, p->data, real_len - to_end);
        }
        /*endif*/
    }
    else
    {
        /* A one step process */
        if (buf)
            memcpy(buf, p->data + p->optr, real_len);
        /*endif*/
    }
    /*endif*/
    return real_len;
}
/*- End of function --------------------------------------------------------*/

int queue_read(queue_t *p, uint8_t *buf, int len)
{
    int real_len;
    int to_end;

    real_len = queue_contents(p);
    if (real_len < len)
    {
        if (p->flags & QUEUE_READ_ATOMIC)
            return -1;
        /*endif*/
    }
    else
    {
        real_len = len;
    }
    /*endif*/
    if (real_len == 0)
        return 0;
    /*endif*/
    to_end = p->len + 1 - p->optr;
    if (p->iptr < p->optr  &&  to_end < real_len)
    {
        /* A two step process */
        if (buf)
        {
            memcpy(buf, p->data + p->optr, to_end);
            memcpy(buf + to_end, p->data, real_len - to_end);
        }
        /*endif*/
        p->optr += (real_len - p->len + 1);
    }
    else
    {
        /* A one step process */
        if (buf)
            memcpy(buf, p->data + p->optr, real_len);
        /*endif*/
        p->optr += real_len;
        if (p->optr > (p->len + 1))
            p->optr = 0;
        /*endif*/
    }
    /*endif*/
    return real_len;
}
/*- End of function --------------------------------------------------------*/

int queue_write(queue_t *p, const uint8_t *buf, int len)
{
    int real_len;
    int to_end;

    real_len = queue_free_space(p);
    if (real_len < len)
    {
        if (p->flags & QUEUE_WRITE_ATOMIC)
            return -1;
        /*endif*/
    }
    else
    {
        real_len = len;
    }
    /*endif*/
    if (real_len == 0)
        return 0;
    /*endif*/
    to_end = p->len + 1 - p->iptr;
    if (p->iptr < p->optr  ||  to_end >= real_len)
    {
        /* A one step process */
        memcpy(p->data + p->iptr, buf, real_len);
        p->iptr += real_len;
        if (p->iptr > (p->len + 1))
            p->iptr = 0;
        /*endif*/
    }
    else
    {
        /* A two step process */
        memcpy(p->data + p->iptr, buf, to_end);
        memcpy(p->data, buf + to_end, real_len - to_end);
        p->iptr += (real_len - p->len + 1);
    }
    /*endif*/
    return real_len;
}
/*- End of function --------------------------------------------------------*/

int queue_test_msg(queue_t *p)
{
    uint16_t lenx;

    if (queue_view(p, (uint8_t *) &lenx, sizeof(uint16_t)) != sizeof(uint16_t))
        return -1;
    /*endif*/
    return lenx;
}
/*- End of function --------------------------------------------------------*/

int queue_read_msg(queue_t *p, uint8_t *buf, int len)
{
    uint16_t lenx;

    if (queue_read(p, (uint8_t *) &lenx, sizeof(uint16_t)) != sizeof(uint16_t))
        return -1;
    /*endif*/
    if (lenx == 0)
        return 0;
    /*endif*/
    if ((int) lenx > len)
    {
        len = queue_read(p, buf, len);
        /* Discard the rest of the message */
        queue_read(p, NULL, lenx - len);
        return len;
    }
    /*endif*/
    return queue_read(p, buf, lenx);
}
/*- End of function --------------------------------------------------------*/

int queue_write_msg(queue_t *p, const uint8_t *buf, int len)
{
    uint16_t lenx;

    if (queue_free_space(p) < (int) (len + sizeof(uint16_t)))
        return 0;
    /*endif*/
    lenx = (uint16_t) len;
    if (queue_write(p, (uint8_t *) &lenx, sizeof(uint16_t)) != sizeof(uint16_t))
        return -1;
    /*endif*/
    if (len == 0)
        return 0;
    return queue_write(p, buf, len);
}
/*- End of function --------------------------------------------------------*/

int queue_create(queue_t *p, int len, int flags)
{
    p->iptr =
    p->optr = 0;
    p->flags = flags;
    p->len = len;
    p->data = malloc(len + 1);
    if (p->data == NULL)
    {
        p->flags = 0;
        p->len = 0;
        return -1;
    }
    /*endif*/
    return 0;
}
/*- End of function --------------------------------------------------------*/

int queue_delete(queue_t *p)
{
    p->flags = 0;
    p->iptr =
    p->optr = 0;
    p->len = 0;
    if (p->data)
    {
        free(p->data);
        p->data = NULL;
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
