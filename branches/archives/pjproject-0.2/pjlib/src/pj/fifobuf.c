/* $Header: /cvs/pjproject-0.2.9.3/pjlib/src/pj/fifobuf.c,v 1.1 2005/12/02 20:02:29 nn Exp $ */
/* 
 * PJLIB - PJ Foundation Library
 * (C)2003-2005 Benny Prijono <bennylp@bulukucing.org>
 *
 * Author:
 *  Benny Prijono <bennylp@bulukucing.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "fifobuf.h"
#include <pj/log.h>

#define THIS_FILE   "fifobuf"

#define SZ  sizeof(unsigned)

PJ_DEF(void)
pj_fifobuf_init (pj_fifobuf_t *fifobuf, void *buffer, unsigned size)
{
    PJ_LOG(6, (THIS_FILE, 
	       "fifobuf_init fifobuf=%p buffer=%p, size=%d", 
	       fifobuf, buffer, size));

    fifobuf->first = buffer;
    fifobuf->last = fifobuf->first + size;
    fifobuf->ubegin = fifobuf->uend = fifobuf->first;
    fifobuf->full = 0;
}

PJ_DEF(unsigned)
pj_fifobuf_max_size (pj_fifobuf_t *fifobuf)
{
    unsigned s1, s2;

    if (fifobuf->uend >= fifobuf->ubegin) {
	s1 = fifobuf->last - fifobuf->uend;
	s2 = fifobuf->ubegin - fifobuf->first;
    } else {
	s1 = s2 = fifobuf->ubegin - fifobuf->uend;
    }
    
    return s1<s2 ? s2 : s1;
}

PJ_DEF(void*)
pj_fifobuf_alloc (pj_fifobuf_t *fifobuf, unsigned size)
{
    unsigned available;
    char *start;

    if (fifobuf->full) {
	PJ_LOG(6, (THIS_FILE, 
		   "fifobuf_alloc fifobuf=%p, size=%d: full!", 
		   fifobuf, size));
	return NULL;
    }

    /* try to allocate from the end part of the fifo */
    if (fifobuf->uend >= fifobuf->ubegin) {
	available = fifobuf->last - fifobuf->uend;
	if (available >= size+SZ) {
	    char *ptr = fifobuf->uend;
	    fifobuf->uend += (size+SZ);
	    if (fifobuf->uend == fifobuf->last)
		fifobuf->uend = fifobuf->first;
	    if (fifobuf->uend == fifobuf->ubegin)
		fifobuf->full = 1;
	    *(unsigned*)ptr = size+SZ;
	    ptr += SZ;

	    PJ_LOG(6, (THIS_FILE, 
		       "fifobuf_alloc fifobuf=%p, size=%d: returning %p, p1=%p, p2=%p", 
		       fifobuf, size, ptr, fifobuf->ubegin, fifobuf->uend));
	    return ptr;
	}
    }

    /* try to allocate from the start part of the fifo */
    start = (fifobuf->uend <= fifobuf->ubegin) ? fifobuf->uend : fifobuf->first;
    available = fifobuf->ubegin - start;
    if (available >= size+SZ) {
	char *ptr = start;
	fifobuf->uend = start + size + SZ;
	if (fifobuf->uend == fifobuf->ubegin)
	    fifobuf->full = 1;
	*(unsigned*)ptr = size+SZ;
	ptr += SZ;

	PJ_LOG(6, (THIS_FILE, 
		   "fifobuf_alloc fifobuf=%p, size=%d: returning %p, p1=%p, p2=%p", 
		   fifobuf, size, ptr, fifobuf->ubegin, fifobuf->uend));
	return ptr;
    }

    PJ_LOG(6, (THIS_FILE, 
	       "fifobuf_alloc fifobuf=%p, size=%d: no space left! p1=%p, p2=%p", 
	       fifobuf, size, fifobuf->ubegin, fifobuf->uend));
    return NULL;
}

PJ_DEF(pj_status_t)
pj_fifobuf_unalloc (pj_fifobuf_t *fifobuf, void *buf)
{
    char *ptr = buf;
    char *endptr;
    unsigned sz;

    ptr -= SZ;
    sz = *(unsigned*)ptr;

    endptr = fifobuf->uend;
    if (endptr == fifobuf->first)
	endptr = fifobuf->last;

    if (ptr+sz != endptr) {
	pj_assert(!"Invalid pointer to undo alloc");
	return -1;
    }

    fifobuf->uend = ptr;
    fifobuf->full = 0;

    PJ_LOG(6, (THIS_FILE, 
	       "fifobuf_unalloc fifobuf=%p, ptr=%p, size=%d, p1=%p, p2=%p", 
	       fifobuf, buf, sz, fifobuf->ubegin, fifobuf->uend));

    return 0;
}

PJ_DEF(pj_status_t)
pj_fifobuf_free (pj_fifobuf_t *fifobuf, void *buf)
{
    char *ptr = buf;
    char *end;
    unsigned sz;

    ptr -= SZ;
    if (ptr < fifobuf->first || ptr >= fifobuf->last) {
	pj_assert(!"Invalid pointer to free");
	return -1;
    }

    if (ptr != fifobuf->ubegin && ptr != fifobuf->first) {
	pj_assert(!"Invalid free() sequence!");
	return -1;
    }

    end = (fifobuf->uend > fifobuf->ubegin) ? fifobuf->uend : fifobuf->last;
    sz = *(unsigned*)ptr;
    if (ptr+sz > end) {
	pj_assert(!"Invalid size!");
	return -1;
    }

    fifobuf->ubegin = ptr + sz;

    /* Rollover */
    if (fifobuf->ubegin == fifobuf->last)
	fifobuf->ubegin = fifobuf->first;

    /* Reset if fifobuf is empty */
    if (fifobuf->ubegin == fifobuf->uend)
	fifobuf->ubegin = fifobuf->uend = fifobuf->first;

    fifobuf->full = 0;

    PJ_LOG(6, (THIS_FILE, 
	       "fifobuf_free fifobuf=%p, ptr=%p, size=%d, p1=%p, p2=%p", 
	       fifobuf, buf, sz, fifobuf->ubegin, fifobuf->uend));

    return 0;
}
