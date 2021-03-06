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
#include <pjmedia/resample.h>
#include <pjmedia/errno.h>
#include <pj/assert.h>
#include <pj/pool.h>
#include <pj/string.h>


#define BYTES_PER_SAMPLE	2

struct resample_port
{
    pjmedia_port	 base;
    pjmedia_port	*dn_port;
    unsigned		 options;
    pjmedia_resample	*resample_get;
    pjmedia_resample	*resample_put;
    pj_int16_t		*get_buf;
    pj_int16_t		*put_buf;
};



static pj_status_t resample_put_frame(pjmedia_port *this_port,
				      const pjmedia_frame *frame);
static pj_status_t resample_get_frame(pjmedia_port *this_port, 
				      pjmedia_frame *frame);
static pj_status_t resample_destroy(pjmedia_port *this_port);



PJ_DEF(pj_status_t) pjmedia_resample_port_create( pj_pool_t *pool,
						  pjmedia_port *dn_port,
						  unsigned clock_rate,
						  unsigned opt,
						  pjmedia_port **p_port  )
{
    struct resample_port *rport;
    unsigned ptime;
    pj_status_t status;

    /* Validate arguments. */
    PJ_ASSERT_RETURN(pool && dn_port && clock_rate && p_port, PJ_EINVAL);

    /* Only supports 16bit samples per frame */
    PJ_ASSERT_RETURN(dn_port->info.bits_per_sample == 16, PJMEDIA_ENCBITS);

    ptime = dn_port->info.samples_per_frame * 1000 / 
	    dn_port->info.clock_rate;
    
    /* Create and initialize port. */
    rport = pj_pool_zalloc(pool, sizeof(struct resample_port));
    PJ_ASSERT_RETURN(rport != NULL, PJ_ENOMEM);

    rport->base.info.clock_rate = clock_rate;
    rport->base.info.samples_per_frame = clock_rate * ptime / 1000;
    rport->base.info.bytes_per_frame = rport->base.info.samples_per_frame *
				       BYTES_PER_SAMPLE;
    rport->base.info.bits_per_sample = BYTES_PER_SAMPLE * 8;
    rport->base.info.channel_count = dn_port->info.channel_count;
    rport->base.info.encoding_name = pj_str("pcm");
    rport->base.info.has_info = 1;
    rport->base.info.name = pj_str("resample");
    rport->base.info.need_info = 0;
    rport->base.info.pt = 0xFF;
    rport->base.info.signature = PJMEDIA_PORT_SIGNATURE('R','S','M','P');
    rport->base.info.type = PJMEDIA_TYPE_AUDIO;

    rport->dn_port = dn_port;
    rport->options = opt;


    /* Create buffers. 
     * We need separate buffer for get_frame() and put_frame() since
     * both functions may run simultaneously.
     */
    rport->get_buf = pj_pool_alloc(pool, rport->base.info.bytes_per_frame);
    PJ_ASSERT_RETURN(rport->get_buf != NULL, PJ_ENOMEM);

    rport->put_buf = pj_pool_alloc(pool, rport->base.info.bytes_per_frame);
    PJ_ASSERT_RETURN(rport->put_buf != NULL, PJ_ENOMEM);


    /* Create "get_frame" resample */
    status = pjmedia_resample_create(pool, 
				     (opt&PJMEDIA_RESAMPLE_USE_LINEAR)==0,
				     (opt&PJMEDIA_RESAMPLE_USE_SMALL_FILTER)==0,
				     dn_port->info.clock_rate, 
				     rport->base.info.clock_rate,
				     dn_port->info.samples_per_frame, 
				     &rport->resample_get);
    if (status != PJ_SUCCESS)
	return status;

    /* Create "put_frame" resample */
    status = pjmedia_resample_create(pool, 
				     (opt&PJMEDIA_RESAMPLE_USE_LINEAR)==0, 
				     (opt&PJMEDIA_RESAMPLE_USE_SMALL_FILTER)==0,
				     rport->base.info.clock_rate, 
				     dn_port->info.clock_rate,
				     rport->base.info.samples_per_frame,
				     &rport->resample_put);

    /* Media port interface */
    rport->base.get_frame = &resample_get_frame;
    rport->base.put_frame = &resample_put_frame;
    rport->base.on_destroy = &resample_destroy;


    /* Done */
    *p_port = &rport->base;

    return PJ_SUCCESS;
}



static pj_status_t resample_put_frame(pjmedia_port *this_port,
				      const pjmedia_frame *frame)
{
    struct resample_port *rport = (struct resample_port*) this_port;
    pjmedia_frame downstream_frame;

    /* Return if we don't have downstream port. */
    if (rport->dn_port == NULL) {
	return PJ_SUCCESS;
    }

    if (frame->type == PJMEDIA_FRAME_TYPE_AUDIO) {
	pjmedia_resample_run( rport->resample_put, frame->buf, 
			      rport->put_buf);

	downstream_frame.buf = rport->put_buf;
	downstream_frame.size = rport->dn_port->info.bytes_per_frame;
    } else {
	downstream_frame.buf = frame->buf;
	downstream_frame.size = frame->size;
    }

    downstream_frame.type = frame->type;
    downstream_frame.timestamp.u64 = frame->timestamp.u64;

    return pjmedia_port_put_frame( rport->dn_port, &downstream_frame );
}



static pj_status_t resample_get_frame(pjmedia_port *this_port, 
				      pjmedia_frame *frame)
{
    struct resample_port *rport = (struct resample_port*) this_port;
    pjmedia_frame tmp_frame;
    pj_status_t status;

    /* Return silence if we don't have downstream port */
    if (rport->dn_port == NULL) {
	pj_memset(frame->buf, frame->size, 0);
	return PJ_SUCCESS;
    }

    tmp_frame.buf = rport->get_buf;
    tmp_frame.size = rport->dn_port->info.bytes_per_frame;
    tmp_frame.timestamp.u64 = frame->timestamp.u64;
    tmp_frame.type = PJMEDIA_FRAME_TYPE_AUDIO;

    status = pjmedia_port_get_frame( rport->dn_port, &tmp_frame);
    if (status != PJ_SUCCESS)
	return status;

    if (tmp_frame.type != PJMEDIA_FRAME_TYPE_AUDIO) {
	frame->type = tmp_frame.type;
	frame->timestamp = tmp_frame.timestamp;
	frame->size = tmp_frame.size;
	if (tmp_frame.size)
	    pj_memcpy(frame->buf, tmp_frame.buf, tmp_frame.size);
	return PJ_SUCCESS;
    }

    pjmedia_resample_run( rport->resample_get, tmp_frame.buf, frame->buf);

    frame->size = rport->base.info.bytes_per_frame;
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

    return PJ_SUCCESS;
}


static pj_status_t resample_destroy(pjmedia_port *this_port)
{
    struct resample_port *rport = (struct resample_port*) this_port;

    if ((rport->options & PJMEDIA_RESAMPLE_DONT_DESTROY_DN)==0) {
	pjmedia_port_destroy(rport->dn_port);
	rport->dn_port = NULL;
    }

    /* Nothing else to do */

    return PJ_SUCCESS;
}

