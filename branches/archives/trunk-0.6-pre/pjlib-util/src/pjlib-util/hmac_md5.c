/* $Id$ */
/* 
 * Copyright (C) 2003-2007 Benny Prijono <benny@prijono.org>
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
#include <pjlib-util/md5.h>
#include <pj/string.h>


/* This code is taken from RFC 2104 */


PJ_DEF(void) pj_hmac_md5( const pj_uint8_t *input, unsigned input_len, 
			  const pj_uint8_t *key, unsigned key_len, 
			  pj_uint8_t digest[16] )
{
    pj_md5_context context;
    pj_uint8_t k_ipad[65];
    pj_uint8_t k_opad[65];
    pj_uint8_t tk[16];
    int i;

    /* if key is longer than 64 bytes reset it to key=MD5(key) */
    if (key_len > 64) {
        pj_md5_context      tctx;

        pj_md5_init(&tctx);
        pj_md5_update(&tctx, key, key_len);
        pj_md5_final(&tctx, tk);

        key = tk;
        key_len = 16;
    }

    /* start out by storing key in pads */
    pj_bzero( k_ipad, sizeof(k_ipad));
    pj_bzero( k_opad, sizeof(k_opad));
    pj_memcpy( k_ipad, key, key_len);
    pj_memcpy( k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i=0; i<64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }
    /*
     * perform inner MD5
     */
    pj_md5_init(&context);
    pj_md5_update(&context, k_ipad, 64);
    pj_md5_update(&context, input, input_len);
    pj_md5_final(&context, digest);

    /*
     * perform outer MD5
     */
    pj_md5_init(&context);
    pj_md5_update(&context, k_opad, 64);
    pj_md5_update(&context, digest, 16);
    pj_md5_final(&context, digest);
}

