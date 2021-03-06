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

PJ_IDEF(void) pjsip_msg_add_hdr( pjsip_msg *msg, pjsip_hdr *hdr )
{
    pj_list_insert_before(&msg->hdr, hdr);
}

PJ_IDEF(void) pjsip_msg_insert_first_hdr( pjsip_msg *msg, pjsip_hdr *hdr )
{
    pj_list_insert_after(&msg->hdr, hdr);
}

