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
#ifndef __PJSIP_AUTH_SIP_AUTH_PARSER_H__
#define __PJSIP_AUTH_SIP_AUTH_PARSER_H__

/**
 * @file pjsip_auth_parser.h
 * @brief SIP Authorization Parser Module.
 */

#include <pj/types.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJSIP_AUTH_PARSER_MODULE Authorization Parser Module
 * @ingroup PJSIP_AUTH
 * @{
 */

/**
 * Initialize and register authorization parser module.
 * This will register parser handler for various Authorization related headers
 * such as Authorization, WWW-Authenticate, Proxy-Authorizization, and 
 * Proxy-Authenticate headers.
 *
 * @return      PJ_SUCCESS or the appropriate status code.
 */
PJ_DECL(pj_status_t) pjsip_auth_init_parser(void);

/**
 * DeInitialize authorization parser module.
 */
PJ_DECL(void) pjsip_auth_deinit_parser();


extern const pj_str_t	pjsip_USERNAME_STR,
			pjsip_REALM_STR,
			pjsip_NONCE_STR,
			pjsip_URI_STR,
			pjsip_RESPONSE_STR,
			pjsip_ALGORITHM_STR,
			pjsip_DOMAIN_STR,
			pjsip_STALE_STR,
			pjsip_QOP_STR,
			pjsip_CNONCE_STR,
			pjsip_OPAQUE_STR,
			pjsip_NC_STR,
			pjsip_TRUE_STR,
			pjsip_FALSE_STR,
			pjsip_DIGEST_STR,
			pjsip_PGP_STR,
			pjsip_MD5_STR,
			pjsip_AUTH_STR;
/*
extern const pj_str_t	pjsip_QUOTED_TRUE_STR,
			pjsip_QUOTED_FALSE_STR,
			pjsip_QUOTED_DIGEST_STR,
			pjsip_QUOTED_PGP_STR,
			pjsip_QUOTED_MD5_STR,
			pjsip_QUOTED_AUTH_STR;
*/

/**
 * @}
 */

PJ_END_DECL

#endif	/* __PJSIP_AUTH_SIP_AUTH_PARSER_H__ */

