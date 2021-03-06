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
/*
 * Based on implementation kindly contributed by Switchlab, Ltd.
 */
#include <pjmedia/jbuf.h>
#include <pjmedia/errno.h>
#include <pj/pool.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/string.h>


#define THIS_FILE   "jbuf.c"


struct jb_framelist
{
    char	*flist_buffer;
    int		*flist_frame_type;
    unsigned	 flist_frame_size;
    unsigned	 flist_max_count;
    unsigned	 flist_empty;
    unsigned	 flist_head;
    unsigned	 flist_tail;
    unsigned	 flist_origin;
};


typedef struct jb_framelist jb_framelist;

struct pjmedia_jbuf
{
    pj_str_t	    name;		  // jitter buffer name
    jb_framelist    jb_framelist;
    pj_size_t	    jb_frame_size;	  // frame size	
    unsigned	    jb_frame_ptime;	  // frame duration.
    pj_size_t	    jb_max_count;	  // max frames in the jitter framelist->flist_buffer

    int		    jb_level;		  // delay between source & destination
					  // (calculated according of the number of get/put operations)
    int		    jb_last_level;	  // last delay	
    int		    jb_last_jitter;	  // last jitter calculated
    int		    jb_max_hist_jitter;   // max jitter	during the last	jitter calculations
    int		    jb_stable_hist;	  // num of times the delay has	been lower then	the prefetch num
    unsigned	    jb_op_count;	  // of of operations.
    int		    jb_last_op;		  // last operation executed on	the framelist->flist_buffer (put/get)
    int		    jb_last_seq_no;	  // seq no. of	the last frame inserted	to the framelist->flist_buffer
    int		    jb_prefetch;	  // no. of frame to insert before removing some
					  // (at the beginning of the framelist->flist_buffer operation)
    int		    jb_prefetch_cnt;	  // prefetch counter
    int		    jb_min_prefetch;	  // Minimum allowable prefetch
    int		    jb_max_prefetch;	  // Maximum allowable prefetch
    int		    jb_status;		  // status is 'init' until the	first 'put' operation


};


#define JB_STATUS_INITIALIZING	0
#define JB_STATUS_PROCESSING	1

#define	PJ_ABS(x)	((x > 0) ? (x) : -(x))
#define	PJ_MAX(x, y)	((x > y) ? (x) : (y))
#define	PJ_MIN(x, y)	((x < y) ? (x) : (y))


/* Enabling this would log the jitter buffer state about once per 
 * second.
 */
#if 0
#  define TRACE__(args)	    PJ_LOG(4,args)
#else
#  define TRACE__(args)
#endif


static pj_status_t jb_framelist_init( pj_pool_t *pool,
				      jb_framelist *framelist,
				      unsigned frame_size,
				      unsigned max_count) 
{
    PJ_ASSERT_RETURN(pool && framelist, PJ_EINVAL);

    pj_bzero(framelist, sizeof(jb_framelist));

    framelist->flist_frame_size = frame_size;
    framelist->flist_max_count = max_count;
    framelist->flist_buffer = pj_pool_zalloc(pool,
					     framelist->flist_frame_size * 
					     framelist->flist_max_count);

    framelist->flist_frame_type = 
	pj_pool_zalloc(pool, sizeof(framelist->flist_frame_type[0]) * 
				framelist->flist_max_count);

    framelist->flist_empty = 1;

    return PJ_SUCCESS;

}

static pj_status_t jb_framelist_destroy(jb_framelist *framelist) 
{
    PJ_UNUSED_ARG(framelist);
    return PJ_SUCCESS;
}


static unsigned jb_framelist_size(jb_framelist *framelist) 
{
    if (framelist->flist_tail == framelist->flist_head) {
	return framelist->flist_empty ? 0 : framelist->flist_max_count;
    } else {
	return (framelist->flist_tail - framelist->flist_head + 
		framelist->flist_max_count) % framelist->flist_max_count;
    }
}


static pj_bool_t jb_framelist_get(jb_framelist *framelist,
				  void *frame,
				  pjmedia_jb_frame_type *p_type) 
{
    if (!framelist->flist_empty) {
	pj_memcpy(frame, 
		  framelist->flist_buffer + 
		    framelist->flist_head * framelist->flist_frame_size,
		  framelist->flist_frame_size);
	*p_type = (pjmedia_jb_frame_type) 
		  framelist->flist_frame_type[framelist->flist_head];

	pj_bzero(framelist->flist_buffer + 
		    framelist->flist_head * framelist->flist_frame_size,
		  framelist->flist_frame_size);
	framelist->flist_frame_type[framelist->flist_head] = 
	    PJMEDIA_JB_MISSING_FRAME;

	framelist->flist_origin++;
	framelist->flist_head = (framelist->flist_head + 1 ) % 
				framelist->flist_max_count;
	if (framelist->flist_head == framelist->flist_tail) 
	    framelist->flist_empty = PJ_TRUE;
	
	return PJ_TRUE;

    } else {
	pj_bzero(frame, framelist->flist_frame_size);
	return PJ_FALSE;
    }
}


static void jb_framelist_remove_head( jb_framelist *framelist,
				      unsigned count) 
{
    unsigned cur_size;

    cur_size = jb_framelist_size(framelist);
    if (count > cur_size) 
	count = cur_size;

    if (count) {
	// may be done in two steps if overlapping
	unsigned step1,step2;
	unsigned tmp = framelist->flist_head+count;

	if (tmp > framelist->flist_max_count) {
	    step1 = framelist->flist_max_count - framelist->flist_head;
	    step2 = count-step1;
	} else {
	    step1 = count;
	    step2 = 0;
	}

	pj_bzero(framelist->flist_buffer + 
		    framelist->flist_head * framelist->flist_frame_size,
	          step1*framelist->flist_frame_size);
	pj_memset(framelist->flist_frame_type+framelist->flist_head,
		  PJMEDIA_JB_MISSING_FRAME,
		  step1*sizeof(framelist->flist_frame_type[0]));

	if (step2) {
	    pj_bzero( framelist->flist_buffer,
		      step2*framelist->flist_frame_size);
	    pj_memset(framelist->flist_frame_type,
		      PJMEDIA_JB_MISSING_FRAME,
		      step2*sizeof(framelist->flist_frame_type[0]));
	}

	// update pointers
	framelist->flist_origin += count;
	framelist->flist_head = (framelist->flist_head + count) % 
			        framelist->flist_max_count;
	if (framelist->flist_head == framelist->flist_tail) 
	    framelist->flist_empty = PJ_TRUE;
    }
}


static pj_bool_t jb_framelist_put_at(jb_framelist *framelist,
				     unsigned index,
				     const void *frame,
				     unsigned frame_size) 
{
    unsigned where;

    assert(frame_size <= framelist->flist_frame_size);

    if (!framelist->flist_empty) {
	unsigned max_index;
	unsigned cur_size;

	// too late
	if (index < framelist->flist_origin) 
	    return PJ_FALSE;

	// too soon
	max_index = framelist->flist_origin + framelist->flist_max_count - 1;
	if (index > max_index)
	    return PJ_FALSE;

	where = (index - framelist->flist_origin + framelist->flist_head) % 
	        framelist->flist_max_count;

	// update framelist->flist_tail pointer
	cur_size = jb_framelist_size(framelist);
	if (index >= framelist->flist_origin + cur_size) {
	    unsigned diff = (index - (framelist->flist_origin + cur_size));
	    framelist->flist_tail = (framelist->flist_tail + diff + 1) % 
				    framelist->flist_max_count;
	}
    } else {
	where = framelist->flist_tail;
	framelist->flist_origin = index;
	framelist->flist_tail = (framelist->flist_tail + 1) % 
				framelist->flist_max_count;
	framelist->flist_empty = PJ_FALSE;
    }

    pj_memcpy(framelist->flist_buffer + where * framelist->flist_frame_size, 
	      frame, frame_size);

    framelist->flist_frame_type[where] = PJMEDIA_JB_NORMAL_FRAME;

    return PJ_TRUE;
}



enum pjmedia_jb_op
{
    JB_OP_INIT  = -1,
    JB_OP_PUT   = 1,
    JB_OP_GET   = 2
};


PJ_DEF(pj_status_t) pjmedia_jbuf_create(pj_pool_t *pool, 
					const pj_str_t *name,
					unsigned frame_size, 
					unsigned ptime,
					unsigned max_count,
					pjmedia_jbuf **p_jb)
{
    pjmedia_jbuf *jb;
    pj_status_t status;

    jb = pj_pool_zalloc(pool, sizeof(pjmedia_jbuf));

    status = jb_framelist_init(pool, &jb->jb_framelist, frame_size, max_count);
    if (status != PJ_SUCCESS)
	return status;

    pj_strdup_with_null(pool, &jb->name, name);
    jb->jb_frame_size	 = frame_size;
    jb->jb_frame_ptime   = ptime;
    jb->jb_last_seq_no	 = -1;
    jb->jb_level	 = 0;
    jb->jb_last_level	 = 0;
    jb->jb_last_jitter	 = 0;
    jb->jb_last_op	 = JB_OP_INIT;
    jb->jb_prefetch	 = PJ_MIN(PJMEDIA_JB_DEFAULT_INIT_DELAY,max_count*4/5);
    jb->jb_prefetch_cnt	 = 0;
    jb->jb_min_prefetch  = 0;
    jb->jb_max_prefetch  = max_count*4/5;
    jb->jb_stable_hist	 = 0;
    jb->jb_status	 = JB_STATUS_INITIALIZING;
    jb->jb_max_hist_jitter = 0;
    jb->jb_max_count	 = max_count;

    *p_jb = jb;
    return PJ_SUCCESS;
}


/*
 * Set the jitter buffer to fixed delay mode. The default behavior
 * is to adapt the delay with actual packet delay.
 *
 */
PJ_DEF(pj_status_t) pjmedia_jbuf_set_fixed( pjmedia_jbuf *jb,
					    unsigned prefetch)
{
    PJ_ASSERT_RETURN(jb, PJ_EINVAL);
    PJ_ASSERT_RETURN(prefetch <= jb->jb_max_count, PJ_EINVAL);

    jb->jb_min_prefetch = jb->jb_max_prefetch = 
	jb->jb_prefetch = prefetch;

    return PJ_SUCCESS;
}


/*
 * Set the jitter buffer to adaptive mode.
 */
PJ_DEF(pj_status_t) pjmedia_jbuf_set_adaptive( pjmedia_jbuf *jb,
					       unsigned prefetch,
					       unsigned min_prefetch,
					       unsigned max_prefetch)
{
    PJ_ASSERT_RETURN(jb, PJ_EINVAL);
    PJ_ASSERT_RETURN(min_prefetch < max_prefetch &&
		     prefetch >= min_prefetch &&
		     prefetch <= max_prefetch &&
		     max_prefetch <= jb->jb_max_count,
		     PJ_EINVAL);

    jb->jb_prefetch = prefetch;
    jb->jb_min_prefetch = min_prefetch;
    jb->jb_max_prefetch = max_prefetch;

    return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pjmedia_jbuf_reset(pjmedia_jbuf *jb)
{
    jb->jb_last_seq_no	 = -1;
    jb->jb_level	 = 0;
    jb->jb_last_level	 = 0;
    jb->jb_last_jitter	 = 0;
    jb->jb_last_op	 = JB_OP_INIT;
    jb->jb_prefetch_cnt	 = 0;
    jb->jb_stable_hist	 = 0;
    jb->jb_status	 = JB_STATUS_INITIALIZING;
    jb->jb_max_hist_jitter = 0;

    jb_framelist_remove_head(&jb->jb_framelist, 
			     jb_framelist_size(&jb->jb_framelist));
    return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pjmedia_jbuf_destroy(pjmedia_jbuf *jb)
{
    return jb_framelist_destroy(&jb->jb_framelist);
}


static void jbuf_calculate_jitter(pjmedia_jbuf *jb)
{
    unsigned stable_history_limit;

    stable_history_limit = 1000 / jb->jb_frame_ptime;

    jb->jb_last_jitter = PJ_ABS(jb->jb_level-jb->jb_last_level);
    jb->jb_last_level = jb->jb_level;
    jb->jb_max_hist_jitter = PJ_MAX(jb->jb_max_hist_jitter,jb->jb_last_jitter);
    jb->jb_op_count++;

    if (jb->jb_last_jitter < jb->jb_prefetch) {
	jb->jb_stable_hist += jb->jb_last_jitter;
	if (jb->jb_stable_hist > (int)stable_history_limit) {
	    int seq_diff = (jb->jb_prefetch - jb->jb_max_hist_jitter)/3;
	    if (seq_diff<1) seq_diff = 1;

	    jb->jb_prefetch -= seq_diff;
	    if (jb->jb_prefetch < jb->jb_min_prefetch) 
		jb->jb_prefetch = jb->jb_min_prefetch;

	    jb->jb_stable_hist = 0;
	    jb->jb_max_hist_jitter = 0;

	    TRACE__((THIS_FILE,"jb updated(1), prefetch=%d, size=%d", 
		     jb->jb_prefetch, jb_framelist_size(&jb->jb_framelist)));

	    /* These code is used to shorten the delay in the jitter buffer.

	    if (jb->jb_op_count >= stable_history_limit*2 &&
		(int)jb_framelist_size(&jb->jb_framelist) > jb->jb_prefetch+2)
	    {
		jb_framelist_remove_head(&jb->jb_framelist,1);

		PJ_LOG(5,(jb->name.ptr, 
			  "jbuf optimizing, prefetch: %d, size=%d", 
			  jb->jb_prefetch,
			  jb_framelist_size(&jb->jb_framelist)));
		jb->jb_op_count = 0;
	    }
	    */

	}
    } else {
	jb->jb_prefetch = PJ_MIN(jb->jb_last_jitter,
				 (int)(jb->jb_max_count*4/5));
	if (jb->jb_prefetch > jb->jb_max_prefetch)
	    jb->jb_prefetch = jb->jb_max_prefetch;
	jb->jb_stable_hist = 0;
	jb->jb_max_hist_jitter = 0;

	TRACE__((THIS_FILE,"jb updated(2), prefetch=%d, size=%d", 
	         jb->jb_prefetch, jb_framelist_size(&jb->jb_framelist)));

	/* These code is used to shorten the delay in the jitter buffer
	   when the current size is larger than the prefetch. But it does
	   not really work when the stream supports multiple frames, since
	   the size may grow only temporarily.

	if (jb->jb_op_count >= stable_history_limit * 2) {
	    if ((int)jb_framelist_size(&jb->jb_framelist) > jb->jb_prefetch+2) 
	    {
		jb_framelist_remove_head(&jb->jb_framelist,1);

		PJ_LOG(5,(jb->name.ptr, 
			  "jbuf optimizing prefetch: %d, size=%d",
			  jb->jb_prefetch,
			  jb_framelist_size(&jb->jb_framelist)));
	    }

	    jb->jb_op_count = 0;
	}
	*/
    }
}

static void jbuf_update(pjmedia_jbuf *jb, int oper)
{
    if(jb->jb_last_op != oper) {
	jbuf_calculate_jitter(jb);
	jb->jb_last_op = oper;
    }
}

PJ_DEF(void) pjmedia_jbuf_put_frame( pjmedia_jbuf *jb, 
				     const void *frame, 
				     pj_size_t frame_size, 
				     int frame_seq)
{
    pj_size_t min_frame_size;
    int seq_diff;

    if (jb->jb_last_seq_no == -1)	{
	jb->jb_last_seq_no = frame_seq - 1;
    }

    seq_diff = frame_seq - jb->jb_last_seq_no;
    jb->jb_last_seq_no = PJ_MAX(jb->jb_last_seq_no, frame_seq);
    if (seq_diff > 0) jb->jb_level += seq_diff;

    if(jb->jb_status ==	JB_STATUS_INITIALIZING) {
	jb->jb_status = JB_STATUS_PROCESSING;
	jb->jb_level = 0;
	jb->jb_last_level = 0;
	jb->jb_last_jitter = 0;
    } else {
	jbuf_update(jb, JB_OP_PUT);
    }

    min_frame_size = PJ_MIN(frame_size, jb->jb_frame_size);
    if (seq_diff > 0) {

	while (jb_framelist_put_at(&jb->jb_framelist,
				   frame_seq,frame,min_frame_size) ==PJ_FALSE)
	{
	    jb_framelist_remove_head(&jb->jb_framelist,
				     PJ_MAX(jb->jb_max_count/4,1) );
	}

	if (jb->jb_prefetch_cnt < jb->jb_prefetch)	
	    jb->jb_prefetch_cnt += seq_diff;

    }
    else
    {
	jb_framelist_put_at(&jb->jb_framelist,frame_seq,frame,min_frame_size);
    }
}

/*
 * Get frame from jitter buffer.
 */
PJ_DEF(void) pjmedia_jbuf_get_frame( pjmedia_jbuf *jb, 
				     void *frame, 
				     char *p_frame_type)
{
    pjmedia_jb_frame_type ftype;

    jb->jb_level--;

    jbuf_update(jb, JB_OP_GET);

    if (jb_framelist_size(&jb->jb_framelist) == 0) {
	jb->jb_prefetch_cnt = 0;
    }

    if ((jb->jb_prefetch_cnt < jb->jb_prefetch)) {
	/* Can't return frame because jitter buffer is filling up
	 * minimum prefetch.
	 */
	pj_bzero(frame, jb->jb_frame_size);
	if (jb_framelist_size(&jb->jb_framelist) == 0)
	    *p_frame_type = PJMEDIA_JB_ZERO_EMPTY_FRAME;
	else
	    *p_frame_type = PJMEDIA_JB_ZERO_PREFETCH_FRAME;

	return;
    }

    /* Retrieve a frame from frame list */
    if (jb_framelist_get(&jb->jb_framelist,frame,&ftype) == PJ_FALSE) {
	/* Can't return frame because jitter buffer is empty! */
	pj_bzero(frame, jb->jb_frame_size);
	*p_frame_type = PJMEDIA_JB_ZERO_EMPTY_FRAME;

	return;
    }

    /* We've successfully retrieved a frame from the frame list, but
     * the frame could be a blank frame!
     */
    if (ftype == PJMEDIA_JB_NORMAL_FRAME) 
	*p_frame_type	= PJMEDIA_JB_NORMAL_FRAME;
    else 
	*p_frame_type	= PJMEDIA_JB_MISSING_FRAME;
}

/*
 * Get jitter buffer state.
 */
PJ_DEF(pj_status_t) pjmedia_jbuf_get_state( pjmedia_jbuf *jb,
					    pjmedia_jb_state *state )
{
    PJ_ASSERT_RETURN(jb && state, PJ_EINVAL);

    state->frame_size = jb->jb_frame_size;
    state->prefetch = jb->jb_prefetch;
    state->min_prefetch = jb->jb_min_prefetch;
    state->max_prefetch = jb->jb_max_prefetch;
    state->size = jb_framelist_size(&jb->jb_framelist);

    return PJ_SUCCESS;
}

