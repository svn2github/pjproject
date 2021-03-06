/* $Id$ */
/* 
 * Copyright (C) 2003-2006 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJMEDIA_JBUF_H__
#define __PJMEDIA_JBUF_H__


/**
 * @file jbuf.h
 * @brief Adaptive jitter buffer implementation.
 */
#include <pjmedia/types.h>

/**
 * @defgroup PJMED_JBUF Adaptive jitter buffer
 * @ingroup PJMEDIA
 * @{
 *
 */


PJ_BEGIN_DECL


/**
 * Types of frame returned by the jitter buffer.
 */
enum pjmedia_jb_frame_type 
{
    PJMEDIA_JB_MISSING_FRAME   = 0, /**< No frame because it's missing.	    */
    PJMEDIA_JB_NORMAL_FRAME    = 1, /**< Normal frame is being returned.    */
    PJMEDIA_JB_ZERO_FRAME      = 2, /**< Zero frame is being returned.	    */
};


/**
 * The constant PJMEDIA_JB_DEFAULT_INIT_DELAY specifies default jitter
 * buffer prefetch count during jitter buffer creation.
 */
#define PJMEDIA_JB_DEFAULT_INIT_DELAY    15


/**
 * Create the jitter buffer. This function may allocate large chunk of
 * memory to keep the frames in the buffer.
 *
 * @param pool		The pool to allocate memory.
 * @param frame_size	The size of each frame that will be kept in the
 *			jitter buffer. The value here normaly corresponds
 *			to the RTP payload size according to the codec
 *			being used.
 * @param init_delay	Initial jitter buffer delay, in number of frames.
 * @param max_count	Maximum jitter buffer delay, in number of frames.
 * @param p_jb		Pointer to receive jitter buffer instance.
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_create(pj_pool_t *pool, 
					int frame_size, 
					int init_delay, 
					int max_count,
					pjmedia_jbuf **p_jb);

/**
 * Destroy jitter buffer instance.
 *
 * @param jb		The jitter buffer.
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_destroy(pjmedia_jbuf *jb);


/**
 * Put a frame to the jitter buffer. If the frame can be accepted (based
 * on the sequence number), the jitter buffer will copy the frame and put
 * it in the appropriate position in the buffer.
 *
 * Application MUST manage it's own synchronization when multiple threads
 * are accessing the jitter buffer at the same time.
 *
 * @param jb		The jitter buffer.
 * @param frame		Pointer to frame buffer to be stored in the jitter
 *			buffer.
 * @param size		The frame size.
 * @param frame_seq	The frame sequence number.
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_put_frame(pjmedia_jbuf *jb, 
					    const void *frame, 
					    pj_size_t size, 
					    int frame_seq);

/**
 * Get a frame from the jitter buffer. The jitter buffer will return the
 * oldest frame from it's buffer, when it is available.
 *
 * Application MUST manage it's own synchronization when multiple threads
 * are accessing the jitter buffer at the same time.
 *
 * @param jb		The jitter buffer.
 * @param frame		Buffer to receive the payload from the jitter buffer.
 *			Application MUST make sure that the buffer has
 *			appropriate size (i.e. not less than the frame size,
 *			as specified when the jitter buffer was created).
 *			The jitter buffer only copied a frame to this 
 *			buffer when the frame type returned by this function
 *			is PJMEDIA_JB_NORMAL_FRAME.
 * @param p_frm_type	Pointer to receive frame type. If jitter buffer is
 *			currently empty or bufferring, the frame type will
 *			be set to PJMEDIA_JB_ZERO_FRAME, and no frame will
 *			be copied. If the jitter buffer detects that frame is
 *			missing with current sequence number, the frame type
 *			will be set to PJMEDIA_JB_MISSING_FRAME, and no
 *			frame will be copied. If there is a frame, the jitter
 *			buffer will copy the frame to the buffer, and frame
 *			type will be set to PJMEDIA_JB_NORMAL_FRAME.
 *
 * @return		Always returns PJ_SUCCESS.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_get_frame( pjmedia_jbuf *jb, 
					     void *frame, 
					     char *p_frm_type);

/**
 * Retrieve the current value of jitter buffer minimum delay, in number
 * of frames.
 *
 * @param jb		The jitter buffer.
 *
 * @return		Number of frames, indicating the minimum delay that 
 *			will be applied by the jitter buffer between frame
 *			arrival and frame retrieval.
 */
PJ_DECL(unsigned)    pjmedia_jbuf_get_min_delay_size(pjmedia_jbuf *jb);


/**
 * Retrieve the current delay value, in number of frames.
 *
 * @param jb		The jitter buffer.
 *
 * @return		Number of frames, indicating the delay between frame
 *			arrival and frame retrieval.
 */
PJ_DECL(unsigned)    pjmedia_jbuf_get_delay(pjmedia_jbuf *jb);



PJ_END_DECL

/**
 * @}
 */

#endif	/* __PJMEDIA_JBUF_H__ */
