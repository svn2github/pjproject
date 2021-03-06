/* $Id$ */
/* 
 * Copyright (C)2003-2006 Benny Prijono <benny@prijono.org>
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
#ifndef __PJMEDIA_CODEC_L16_H__
#define __PJMEDIA_CODEC_L16_H__

#include <pjmedia-codec/types.h>


/**
 * @defgroup PJMED_L16 L16 Family
 * @ingroup PJMEDIA_CODEC
 * @brief 16bit linear codecs (useful for debugging)
 * @{
 * This section describes functions to register and register L16 codec
 * factory to the codec manager. After the codec factory has been registered,
 * application can use @ref PJMEDIA_CODEC API to manipulate the codec.
 *
 * Note that the L16 codec factory registers several (about fourteen!) 
 * L16 codec types to codec manager (different combinations of clock
 * rate and number of channels).
 */

PJ_BEGIN_DECL


/**
 * Initialize and register L16 codec factory to pjmedia endpoint.
 *
 * @param endpt	    The pjmedia endpoint.
 * @param options   Must be zero for now.
 *
 * @return	    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_codec_l16_init( pjmedia_endpt *endpt,
					     unsigned options);



/**
 * Unregister L16 codec factory from pjmedia endpoint.
 *
 * @return	    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_codec_l16_deinit(void);


PJ_END_DECL


#endif	/* __PJMEDIA_CODEC_L16_H__ */

