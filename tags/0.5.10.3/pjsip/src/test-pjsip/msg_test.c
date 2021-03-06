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
#include "test.h"
#include <pjsip.h>
#include <pjlib.h>

#define POOL_SIZE	8000
#if defined(PJ_DEBUG) && PJ_DEBUG!=0
#   define LOOP		10000
#else
#   define LOOP		100000
#endif
#define AVERAGE_MSG_LEN	800
#define THIS_FILE	"msg_test.c"

static pjsip_msg *create_msg0(pj_pool_t *pool);
static pjsip_msg *create_msg1(pj_pool_t *pool);

#define STATUS_PARTIAL		1
#define STATUS_SYNTAX_ERROR	2

#define FLAG_DETECT_ONLY	1
#define FLAG_PARSE_ONLY		4
#define FLAG_PRINT_ONLY		8

struct test_msg
{
    char	 msg[1024];
    pjsip_msg *(*creator)(pj_pool_t *pool);
    pj_size_t	 len;
    int		 expected_status;
} test_array[] = 
{
{
    /* 'Normal' message with all headers. */
    "INVITE sip:user@foo SIP/2.0\n"
    "from: Hi I'm Joe <sip:joe.user@bar.otherdomain.com>;tag=123457890123456\r"
    "To: Fellow User <sip:user@foo.bar.domain.com>\r\n"
    "Call-ID: 12345678901234567890@bar\r\n"
    "Content-Length: 0\r\n"
    "CSeq: 123456 INVITE\n"
    "Contact: <sip:joe@bar> ; q=0.5;expires=3600,sip:user@host;q=0.500\r"
    "  ,sip:user2@host2\n"
    "Content-Type: text/html ; charset=ISO-8859-4\r"
    "Route: <sip:bigbox3.site3.atlanta.com;lr>,\r\n"
    "  <sip:server10.biloxi.com;lr>\r"
    "Record-Route: <sip:server10.biloxi.com>,\r\n" /* multiple routes+folding*/
    "  <sip:bigbox3.site3.atlanta.com;lr>\n"
    "v: SIP/2.0/SCTP bigbox3.site3.atlanta.com;branch=z9hG4bK77ef4c230\n"
    "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\n" /* folding. */
    " ;received=192.0.2.1\r\n"
    "Via: SIP/2.0/UDP 10.2.1.1, SIP/2.0/TCP 192.168.1.1\n"
    "Organization: \r"
    "Max-Forwards: 70\n"
    "X-Header: \r\n"	    /* empty header */
    "P-Associated-URI:\r\n" /* empty header without space */
    "\r\n",
    &create_msg0,
    PJ_SUCCESS
},
{
    /* Typical response message. */
    "SIP/2.0 200 OK\r\n"
    "Via: SIP/2.0/SCTP server10.biloxi.com;branch=z9hG4bKnashds8;rport;received=192.0.2.1\r\n"
    "Via: SIP/2.0/UDP bigbox3.site3.atlanta.com;branch=z9hG4bK77ef4c2312983.1;received=192.0.2.2\r\n"
    "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bK776asdhds ;received=192.0.2.3\r\n"
    "Route: <sip:proxy.sipprovider.com>\r\n"
    "Route: <sip:proxy.supersip.com:5060>\r\n"
    "Max-Forwards: 70\r\n"
    "To: Bob <sip:bob@biloxi.com>;tag=a6c85cf\r\n"
    "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
    "Call-ID: a84b4c76e66710@pc33.atlanta.com\r\n"
    "CSeq: 314159 INVITE\r\n"
    "Contact: <sips:bob@192.0.2.4>\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 150\r\n"
    "\r\n"
    "v=0\r\n"
    "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "c=IN IP4 pc33.atlanta.com\r\n"
    "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
    "a=rtpmap:0 PCMU/8000\r\n",
    &create_msg1,
    PJ_SUCCESS
}
};

static struct
{
    int flag;
    pj_highprec_t detect_len, parse_len, print_len;
    pj_timestamp  detect_time, parse_time, print_time;
} var;

static pj_status_t test_entry( pj_pool_t *pool, struct test_msg *entry )
{
    pjsip_msg *parsed_msg, *ref_msg = NULL;
    static pjsip_msg *print_msg;
    pj_status_t status = PJ_SUCCESS;
    int len;
    pj_str_t str1, str2;
    pjsip_hdr *hdr1, *hdr2;
    pj_timestamp t1, t2;
    pjsip_parser_err_report err_list;
    pj_size_t msg_size;
    char msgbuf1[PJSIP_MAX_PKT_LEN];
    char msgbuf2[PJSIP_MAX_PKT_LEN];
    enum { BUFLEN = 512 };

    entry->len = pj_native_strlen(entry->msg);

    if (var.flag & FLAG_PARSE_ONLY)
	goto parse_msg;

    if (var.flag & FLAG_PRINT_ONLY) {
	if (print_msg == NULL)
	    print_msg = entry->creator(pool);
	goto print_msg;
    }

    /* Detect message. */
    var.detect_len = var.detect_len + entry->len;
    pj_get_timestamp(&t1);
    status = pjsip_find_msg(entry->msg, entry->len, PJ_FALSE, &msg_size);
    if (status != PJ_SUCCESS) {
	if (status!=PJSIP_EPARTIALMSG || 
	    entry->expected_status!=STATUS_PARTIAL)
	{
	    app_perror("   error: unable to detect message", status);
	    return -5;
	}
    }
    if (msg_size != entry->len) {
	PJ_LOG(3,(THIS_FILE, "   error: size mismatch"));
	return -6;
    }
    pj_get_timestamp(&t2);
    pj_sub_timestamp(&t2, &t1);
    pj_add_timestamp(&var.detect_time, &t2);

    if (var.flag & FLAG_DETECT_ONLY)
	return PJ_SUCCESS;
    
    /* Parse message. */
parse_msg:
    var.parse_len = var.parse_len + entry->len;
    pj_get_timestamp(&t1);
    pj_list_init(&err_list);
    parsed_msg = pjsip_parse_msg(pool, entry->msg, entry->len, &err_list);
    if (parsed_msg == NULL) {
	if (entry->expected_status != STATUS_SYNTAX_ERROR) {
	    status = -10;
	    if (err_list.next != &err_list) {
		PJ_LOG(3,(THIS_FILE, "   Syntax error in line %d col %d",
			      err_list.next->line, err_list.next->col));
	    }
	    goto on_return;
	}
    }
    pj_get_timestamp(&t2);
    pj_sub_timestamp(&t2, &t1);
    pj_add_timestamp(&var.parse_time, &t2);

    if (var.flag & FLAG_PARSE_ONLY)
	return PJ_SUCCESS;

    /* Create reference message. */
    ref_msg = entry->creator(pool);

    /* Create buffer for comparison. */
    str1.ptr = pj_pool_alloc(pool, BUFLEN);
    str2.ptr = pj_pool_alloc(pool, BUFLEN);

    /* Compare message type. */
    if (parsed_msg->type != ref_msg->type) {
	status = -20;
	goto on_return;
    }

    /* Compare request or status line. */
    if (parsed_msg->type == PJSIP_REQUEST_MSG) {
	pjsip_method *m1 = &parsed_msg->line.req.method;
	pjsip_method *m2 = &ref_msg->line.req.method;

	if (pjsip_method_cmp(m1, m2) != 0) {
	    status = -30;
	    goto on_return;
	}
	status = pjsip_uri_cmp(PJSIP_URI_IN_REQ_URI,
			       parsed_msg->line.req.uri, 
			       ref_msg->line.req.uri);
	if (status != PJ_SUCCESS) {
	    app_perror("   error: request URI mismatch", status);
	    status = -31;
	    goto on_return;
	}
    } else {
	if (parsed_msg->line.status.code != ref_msg->line.status.code) {
	    PJ_LOG(3,(THIS_FILE, "   error: status code mismatch"));
	    status = -32;
	    goto on_return;
	}
	if (pj_strcmp(&parsed_msg->line.status.reason, 
		      &ref_msg->line.status.reason) != 0) 
	{
	    PJ_LOG(3,(THIS_FILE, "   error: status text mismatch"));
	    status = -33;
	    goto on_return;
	}
    }

    /* Compare headers. */
    hdr1 = parsed_msg->hdr.next;
    hdr2 = ref_msg->hdr.next;

    while (hdr1 != &parsed_msg->hdr && hdr2 != &ref_msg->hdr) {
	len = hdr1->vptr->print_on(hdr1, str1.ptr, BUFLEN);
	if (len < 1) {
	    status = -40;
	    goto on_return;
	}
	str1.ptr[len] = '\0';
	str1.slen = len;

	len = hdr2->vptr->print_on(hdr2, str2.ptr, BUFLEN);
	if (len < 1) {
	    status = -50;
	    goto on_return;
	}
	str2.ptr[len] = '\0';
	str2.slen = len;

	if (pj_strcmp(&str1, &str2) != 0) {
	    status = -60;
	    PJ_LOG(3,(THIS_FILE, "   error: header string mismatch:\n"
		          "   h1='%s'\n"
			  "   h2='%s'\n",
			  str1.ptr, str2.ptr));
	    goto on_return;
	}

	hdr1 = hdr1->next;
	hdr2 = hdr2->next;
    }

    if (hdr1 != &parsed_msg->hdr || hdr2 != &ref_msg->hdr) {
	status = -70;
	goto on_return;
    }

    /* Compare body? */
    if (parsed_msg->body==NULL && ref_msg->body==NULL)
	goto print_msg;

    /* Compare msg body length. */
    if (parsed_msg->body->len != ref_msg->body->len) {
	status = -80;
	goto on_return;
    }

    /* Compare msg body content type. */
    if (pj_strcmp(&parsed_msg->body->content_type.type,
	          &ref_msg->body->content_type.type) != 0) {
	status = -90;
	goto on_return;
    }
    if (pj_strcmp(&parsed_msg->body->content_type.subtype,
	          &ref_msg->body->content_type.subtype) != 0) {
	status = -100;
	goto on_return;
    }

    /* Compare body content. */
    str1.slen = parsed_msg->body->print_body(parsed_msg->body,
					     msgbuf1, sizeof(msgbuf1));
    if (str1.slen < 1) {
	status = -110;
	goto on_return;
    }
    str1.ptr = msgbuf1;

    str2.slen = ref_msg->body->print_body(ref_msg->body,
					  msgbuf2, sizeof(msgbuf2));
    if (str2.slen < 1) {
	status = -120;
	goto on_return;
    }
    str2.ptr = msgbuf2;

    if (pj_strcmp(&str1, &str2) != 0) {
	status = -140;
	goto on_return;
    }
    
    /* Print message. */
print_msg:
    var.print_len = var.print_len + entry->len;
    pj_get_timestamp(&t1);
    if (var.flag && FLAG_PRINT_ONLY)
	ref_msg = print_msg;
    len = pjsip_msg_print(ref_msg, msgbuf1, PJSIP_MAX_PKT_LEN);
    if (len < 1) {
	status = -150;
	goto on_return;
    }
    pj_get_timestamp(&t2);
    pj_sub_timestamp(&t2, &t1);
    pj_add_timestamp(&var.print_time, &t2);


    status = PJ_SUCCESS;

on_return:
    return status;
}


static pjsip_msg *create_msg0(pj_pool_t *pool)
{

    pjsip_msg *msg;
    pjsip_name_addr *name_addr;
    pjsip_sip_uri *url;
    pjsip_fromto_hdr *fromto;
    pjsip_cid_hdr *cid;
    pjsip_clen_hdr *clen;
    pjsip_cseq_hdr *cseq;
    pjsip_contact_hdr *contact;
    pjsip_ctype_hdr *ctype;
    pjsip_routing_hdr *routing;
    pjsip_via_hdr *via;
    pjsip_generic_string_hdr *generic;
    pj_str_t str;

    msg = pjsip_msg_create(pool, PJSIP_REQUEST_MSG);

    /* "INVITE sip:user@foo SIP/2.0\n" */
    pjsip_method_set(&msg->line.req.method, PJSIP_INVITE_METHOD);
    url = pjsip_sip_uri_create(pool, 0);
    msg->line.req.uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->user, "user");
    pj_strdup2(pool, &url->host, "foo");

    /* "From: Hi I'm Joe <sip:joe.user@bar.otherdomain.com>;tag=123457890123456\r" */
    fromto = pjsip_from_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)fromto);
    pj_strdup2(pool, &fromto->tag, "123457890123456");
    name_addr = pjsip_name_addr_create(pool);
    fromto->uri = (pjsip_uri*)name_addr;
    pj_strdup2(pool, &name_addr->display, "Hi I'm Joe");
    url = pjsip_sip_uri_create(pool, 0);
    name_addr->uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->user, "joe.user");
    pj_strdup2(pool, &url->host, "bar.otherdomain.com");

    /* "To: Fellow User <sip:user@foo.bar.domain.com>\r\n" */
    fromto = pjsip_to_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)fromto);
    name_addr = pjsip_name_addr_create(pool);
    fromto->uri = (pjsip_uri*)name_addr;
    pj_strdup2(pool, &name_addr->display, "Fellow User");
    url = pjsip_sip_uri_create(pool, 0);
    name_addr->uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->user, "user");
    pj_strdup2(pool, &url->host, "foo.bar.domain.com");

    /* "Call-ID: 12345678901234567890@bar\r\n" */
    cid = pjsip_cid_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)cid);
    pj_strdup2(pool, &cid->id, "12345678901234567890@bar");

    /* "Content-Length: 0\r\n" */
    clen = pjsip_clen_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)clen);
    clen->len = 0;

    /* "CSeq: 123456 INVITE\n" */
    cseq = pjsip_cseq_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)cseq);
    cseq->cseq = 123456;
    pjsip_method_set(&cseq->method, PJSIP_INVITE_METHOD);

    /* "Contact: <sip:joe@bar>;q=0.5;expires=3600*/
    contact = pjsip_contact_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)contact);
    contact->q1000 = 500;
    contact->expires = 3600;
    name_addr = pjsip_name_addr_create(pool);
    contact->uri = (pjsip_uri*)name_addr;
    url = pjsip_sip_uri_create(pool, 0);
    name_addr->uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->user, "joe");
    pj_strdup2(pool, &url->host, "bar");

    /*, sip:user@host;q=0.500\r" */
    contact = pjsip_contact_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)contact);
    contact->q1000 = 500;
    name_addr = pjsip_name_addr_create(pool);
    contact->uri = (pjsip_uri*)name_addr;
    url = pjsip_sip_uri_create(pool, 0);
    name_addr->uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->user, "user");
    pj_strdup2(pool, &url->host, "host");

    /* "  ,sip:user2@host2\n" */
    contact = pjsip_contact_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)contact);
    name_addr = pjsip_name_addr_create(pool);
    contact->uri = (pjsip_uri*)name_addr;
    url = pjsip_sip_uri_create(pool, 0);
    name_addr->uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->user, "user2");
    pj_strdup2(pool, &url->host, "host2");

    /* "Content-Type: text/html; charset=ISO-8859-4\r" */
    ctype = pjsip_ctype_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)ctype);
    pj_strdup2(pool, &ctype->media.type, "text");
    pj_strdup2(pool, &ctype->media.subtype, "html");
    pj_strdup2(pool, &ctype->media.param, ";charset=ISO-8859-4");

    /* "Route: <sip:bigbox3.site3.atlanta.com;lr>,\r\n" */
    routing = pjsip_route_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)routing);
    url = pjsip_sip_uri_create(pool, 0);
    routing->name_addr.uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->host, "bigbox3.site3.atlanta.com");
    url->lr_param = 1;

    /* "  <sip:server10.biloxi.com;lr>\r" */
    routing = pjsip_route_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)routing);
    url = pjsip_sip_uri_create(pool, 0);
    routing->name_addr.uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->host, "server10.biloxi.com");
    url->lr_param = 1;

    /* "Record-Route: <sip:server10.biloxi.com>,\r\n" */
    routing = pjsip_rr_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)routing);
    url = pjsip_sip_uri_create(pool, 0);
    routing->name_addr.uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->host, "server10.biloxi.com");
    url->lr_param = 0;

    /* "  <sip:bigbox3.site3.atlanta.com;lr>\n" */
    routing = pjsip_rr_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)routing);
    url = pjsip_sip_uri_create(pool, 0);
    routing->name_addr.uri = (pjsip_uri*)url;
    pj_strdup2(pool, &url->host, "bigbox3.site3.atlanta.com");
    url->lr_param = 1;

    /* "Via: SIP/2.0/SCTP bigbox3.site3.atlanta.com;branch=z9hG4bK77ef4c230\n" */
    via = pjsip_via_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)via);
    pj_strdup2(pool, &via->transport, "SCTP");
    pj_strdup2(pool, &via->sent_by.host, "bigbox3.site3.atlanta.com");
    pj_strdup2(pool, &via->branch_param, "z9hG4bK77ef4c230");

    /* "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\n"
	" ;received=192.0.2.1\r\n" */
    via = pjsip_via_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)via);
    pj_strdup2(pool, &via->transport, "UDP");
    pj_strdup2(pool, &via->sent_by.host, "pc33.atlanta.com");
    pj_strdup2(pool, &via->branch_param, "z9hG4bKnashds8");
    pj_strdup2(pool, &via->recvd_param, "192.0.2.1");


    /* "Via: SIP/2.0/UDP 10.2.1.1, */ 
    via = pjsip_via_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)via);
    pj_strdup2(pool, &via->transport, "UDP");
    pj_strdup2(pool, &via->sent_by.host, "10.2.1.1");
    
    
    /*SIP/2.0/TCP 192.168.1.1\n" */
    via = pjsip_via_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)via);
    pj_strdup2(pool, &via->transport, "TCP");
    pj_strdup2(pool, &via->sent_by.host, "192.168.1.1");

    /* "Organization: \r" */
    str.ptr = "Organization";
    str.slen = 12;
    generic = pjsip_generic_string_hdr_create(pool, &str, NULL);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)generic);
    generic->hvalue.ptr = NULL;
    generic->hvalue.slen = 0;

    /* "Max-Forwards: 70\n" */
    str.ptr = "Max-Forwards";
    str.slen = 12;
    generic = pjsip_generic_string_hdr_create(pool, &str, NULL);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)generic);
    str.ptr = "70";
    str.slen = 2;
    generic->hvalue = str;

    /* "X-Header: \r\n" */
    str.ptr = "X-Header";
    str.slen = 8;
    generic = pjsip_generic_string_hdr_create(pool, &str, NULL);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)generic);
    str.ptr = NULL;
    str.slen = 0;
    generic->hvalue = str;

    /* P-Associated-URI:\r\n */
    str.ptr = "P-Associated-URI";
    str.slen = 16;
    generic = pjsip_generic_string_hdr_create(pool, &str, NULL);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)generic);
    str.ptr = NULL;
    str.slen = 0;
    generic->hvalue = str;

    return msg;
}

static pjsip_msg *create_msg1(pj_pool_t *pool)
{
    pjsip_via_hdr *via;
    pjsip_route_hdr *route;
    pjsip_name_addr *name_addr;
    pjsip_sip_uri *url;
    pjsip_max_fwd_hdr *max_fwd;
    pjsip_to_hdr *to;
    pjsip_from_hdr *from;
    pjsip_contact_hdr *contact;
    pjsip_ctype_hdr *ctype;
    pjsip_cid_hdr *cid;
    pjsip_clen_hdr *clen;
    pjsip_cseq_hdr *cseq;
    pjsip_msg *msg = pjsip_msg_create(pool, PJSIP_RESPONSE_MSG);
    pjsip_msg_body *body;

    //"SIP/2.0 200 OK\r\n"
    msg->line.status.code = 200;
    msg->line.status.reason = pj_str("OK");

    //"Via: SIP/2.0/SCTP server10.biloxi.com;branch=z9hG4bKnashds8;rport;received=192.0.2.1\r\n"
    via = pjsip_via_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)via);
    via->transport = pj_str("SCTP");
    via->sent_by.host = pj_str("server10.biloxi.com");
    via->branch_param = pj_str("z9hG4bKnashds8");
    via->rport_param = 0;
    via->recvd_param = pj_str("192.0.2.1");

    //"Via: SIP/2.0/UDP bigbox3.site3.atlanta.com;branch=z9hG4bK77ef4c2312983.1;received=192.0.2.2\r\n"
    via = pjsip_via_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)via);
    via->transport = pj_str("UDP");
    via->sent_by.host = pj_str("bigbox3.site3.atlanta.com");
    via->branch_param = pj_str("z9hG4bK77ef4c2312983.1");
    via->recvd_param = pj_str("192.0.2.2");

    //"Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bK776asdhds ;received=192.0.2.3\r\n"
    via = pjsip_via_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)via);
    via->transport = pj_str("UDP");
    via->sent_by.host = pj_str("pc33.atlanta.com");
    via->branch_param = pj_str("z9hG4bK776asdhds");
    via->recvd_param = pj_str("192.0.2.3");

    //"Route: <sip:proxy.sipprovider.com>\r\n"
    route = pjsip_route_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)route);
    url = pjsip_sip_uri_create(pool, PJ_FALSE);
    route->name_addr.uri = (pjsip_uri*)url;
    url->host = pj_str("proxy.sipprovider.com");
    
    //"Route: <sip:proxy.supersip.com:5060>\r\n"
    route = pjsip_route_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)route);
    url = pjsip_sip_uri_create(pool, PJ_FALSE);
    route->name_addr.uri = (pjsip_uri*)url;
    url->host = pj_str("proxy.supersip.com");
    url->port = 5060;

    //"Max-Forwards: 70\r\n"
    max_fwd = pjsip_max_fwd_hdr_create(pool, 70);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)max_fwd);

    //"To: Bob <sip:bob@biloxi.com>;tag=a6c85cf\r\n"
    to = pjsip_to_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)to);
    name_addr = pjsip_name_addr_create(pool);
    name_addr->display = pj_str("Bob");
    to->uri = (pjsip_uri*)name_addr;
    url = pjsip_sip_uri_create(pool, PJ_FALSE);
    name_addr->uri = (pjsip_uri*)url;
    url->user = pj_str("bob");
    url->host = pj_str("biloxi.com");
    to->tag = pj_str("a6c85cf");

    //"From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
    from = pjsip_from_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)from);
    name_addr = pjsip_name_addr_create(pool);
    name_addr->display = pj_str("Alice");
    from->uri = (pjsip_uri*)name_addr;
    url = pjsip_sip_uri_create(pool, PJ_FALSE);
    name_addr->uri = (pjsip_uri*)url;
    url->user = pj_str("alice");
    url->host = pj_str("atlanta.com");
    from->tag = pj_str("1928301774");

    //"Call-ID: a84b4c76e66710@pc33.atlanta.com\r\n"
    cid = pjsip_cid_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)cid);
    cid->id = pj_str("a84b4c76e66710@pc33.atlanta.com");

    //"CSeq: 314159 INVITE\r\n"
    cseq = pjsip_cseq_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)cseq);
    cseq->cseq = 314159;
    pjsip_method_set(&cseq->method, PJSIP_INVITE_METHOD);

    //"Contact: <sips:bob@192.0.2.4>\r\n"
    contact = pjsip_contact_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)contact);
    name_addr = pjsip_name_addr_create(pool);
    contact->uri = (pjsip_uri*)name_addr;
    url = pjsip_sip_uri_create(pool, PJ_TRUE);
    name_addr->uri = (pjsip_uri*)url;
    url->user = pj_str("bob");
    url->host = pj_str("192.0.2.4");

    //"Content-Type: application/sdp\r\n"
    ctype = pjsip_ctype_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)ctype);
    ctype->media.type = pj_str("application");
    ctype->media.subtype = pj_str("sdp");

    //"Content-Length: 150\r\n"
    clen = pjsip_clen_hdr_create(pool);
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)clen);
    clen->len = 150;

    // Body
    body = pj_pool_zalloc(pool, sizeof(*body));
    msg->body = body;
    body->content_type.type = pj_str("application");
    body->content_type.subtype = pj_str("sdp");
    body->data = 
	"v=0\r\n"
	"o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
	"s=-\r\n"
	"t=0 0\r\n"
	"c=IN IP4 pc33.atlanta.com\r\n"
	"m=audio 3456 RTP/AVP 0 1 3 99\r\n"
	"a=rtpmap:0 PCMU/8000\r\n";
    body->len = pj_native_strlen(body->data);
    body->print_body = &pjsip_print_text_body;

    return msg;
}

/*****************************************************************************/

static pj_status_t simple_test(void)
{
    unsigned i;
    pj_status_t status;

    PJ_LOG(3,(THIS_FILE, "  simple test.."));
    for (i=0; i<PJ_ARRAY_SIZE(test_array); ++i) {
	pj_pool_t *pool;
	pool = pjsip_endpt_create_pool(endpt, NULL, POOL_SIZE, POOL_SIZE);
	status = test_entry( pool, &test_array[i] );
	pjsip_endpt_release_pool(endpt, pool);

	if (status != PJ_SUCCESS)
	    return status;
    }

    return PJ_SUCCESS;
}


static int msg_benchmark(unsigned *p_detect, unsigned *p_parse, 
			 unsigned *p_print)
{
    pj_status_t status;
    pj_pool_t *pool;
    int i, loop;
    pj_timestamp zero;
    pj_time_val elapsed;
    pj_highprec_t avg_detect, avg_parse, avg_print, kbytes;


    pj_bzero(&var, sizeof(var));
    zero.u64 = 0;

    for (loop=0; loop<LOOP; ++loop) {
	for (i=0; i<PJ_ARRAY_SIZE(test_array); ++i) {
	    pool = pjsip_endpt_create_pool(endpt, NULL, POOL_SIZE, POOL_SIZE);
	    status = test_entry( pool, &test_array[i] );
	    pjsip_endpt_release_pool(endpt, pool);

	    if (status != PJ_SUCCESS)
		return status;
	}
    }

    kbytes = var.detect_len;
    pj_highprec_mod(kbytes, 1000000);
    pj_highprec_div(kbytes, 100000);
    elapsed = pj_elapsed_time(&zero, &var.detect_time);
    avg_detect = pj_elapsed_usec(&zero, &var.detect_time);
    pj_highprec_mul(avg_detect, AVERAGE_MSG_LEN);
    pj_highprec_div(avg_detect, var.detect_len);
    avg_detect = 1000000 / avg_detect;

    PJ_LOG(3,(THIS_FILE, 
	      "    %u.%u MB detected in %d.%03ds (avg=%d msg detection/sec)", 
	      (unsigned)(var.detect_len/1000000), (unsigned)kbytes,
	      elapsed.sec, elapsed.msec,
	      (unsigned)avg_detect));
    *p_detect = (unsigned)avg_detect;

    kbytes = var.parse_len;
    pj_highprec_mod(kbytes, 1000000);
    pj_highprec_div(kbytes, 100000);
    elapsed = pj_elapsed_time(&zero, &var.parse_time);
    avg_parse = pj_elapsed_usec(&zero, &var.parse_time);
    pj_highprec_mul(avg_parse, AVERAGE_MSG_LEN);
    pj_highprec_div(avg_parse, var.parse_len);
    avg_parse = 1000000 / avg_parse;

    PJ_LOG(3,(THIS_FILE, 
	      "    %u.%u MB parsed in %d.%03ds (avg=%d msg parsing/sec)", 
	      (unsigned)(var.parse_len/1000000), (unsigned)kbytes,
	      elapsed.sec, elapsed.msec,
	      (unsigned)avg_parse));
    *p_parse = (unsigned)avg_parse;

    kbytes = var.print_len;
    pj_highprec_mod(kbytes, 1000000);
    pj_highprec_div(kbytes, 100000);
    elapsed = pj_elapsed_time(&zero, &var.print_time);
    avg_print = pj_elapsed_usec(&zero, &var.print_time);
    pj_highprec_mul(avg_print, AVERAGE_MSG_LEN);
    pj_highprec_div(avg_print, var.print_len);
    avg_print = 1000000 / avg_print;

    PJ_LOG(3,(THIS_FILE, 
	      "    %u.%u MB printed in %d.%03ds (avg=%d msg print/sec)", 
	      (unsigned)(var.print_len/1000000), (unsigned)kbytes,
	      elapsed.sec, elapsed.msec,
	      (unsigned)avg_print));

    *p_print = (unsigned)avg_print;
    return status;
}

/*****************************************************************************/

int msg_test(void)
{
    enum { COUNT = 4, DETECT=0, PARSE=1, PRINT=2 };
    struct {
	unsigned detect;
	unsigned parse;
	unsigned print;
    } run[COUNT];
    unsigned i, max, avg_len;
    char desc[250];
    pj_status_t status;

    status = simple_test();
    if (status != PJ_SUCCESS)
	return status;

    for (i=0; i<COUNT; ++i) {
	PJ_LOG(3,(THIS_FILE, "  benchmarking (%d of %d)..", i+1, COUNT));
	status = msg_benchmark(&run[i].detect, &run[i].parse, &run[i].print);
	if (status != PJ_SUCCESS)
	    return status;
    }

    /* Calculate average message length */
    for (i=0, avg_len=0; i<PJ_ARRAY_SIZE(test_array); ++i) {
	avg_len += test_array[i].len;
    }
    avg_len /= PJ_ARRAY_SIZE(test_array);


    /* Print maximum detect/sec */
    for (i=0, max=0; i<COUNT; ++i)
	if (run[i].detect > max) max = run[i].detect;

    PJ_LOG(3,("", "  Maximum message detection/sec=%u", max));

    pj_ansi_sprintf(desc, "Number of SIP messages "
			  "can be pre-parse by <tt>pjsip_find_msg()</tt> "
			  "per second (tested with %d message sets with "
			  "average message length of "
			  "%d bytes)", (int)PJ_ARRAY_SIZE(test_array), avg_len);
    report_ival("msg-detect-per-sec", max, "msg/sec", desc);

    /* Print maximum parse/sec */
    for (i=0, max=0; i<COUNT; ++i)
	if (run[i].parse > max) max = run[i].parse;

    PJ_LOG(3,("", "  Maximum message parsing/sec=%u", max));

    pj_ansi_sprintf(desc, "Number of SIP messages "
			  "can be <b>parsed</b> by <tt>pjsip_parse_msg()</tt> "
			  "per second (tested with %d message sets with "
			  "average message length of "
			  "%d bytes)", (int)PJ_ARRAY_SIZE(test_array), avg_len);
    report_ival("msg-parse-per-sec", max, "msg/sec", desc);

    /* Msg parsing bandwidth */
    report_ival("msg-parse-bandwidth-mb", avg_len*max/1000000, "MB/sec",
	        "Message parsing bandwidth in megabytes (number of megabytes"
		" worth of SIP messages that can be parsed per second). "
		"The value is derived from msg-parse-per-sec above.");


    /* Print maximum print/sec */
    for (i=0, max=0; i<COUNT; ++i)
	if (run[i].print > max) max = run[i].print;

    PJ_LOG(3,("", "  Maximum message print/sec=%u", max));

    pj_ansi_sprintf(desc, "Number of SIP messages "
			  "can be <b>printed</b> by <tt>pjsip_msg_print()</tt>"
			  " per second (tested with %d message sets with "
			  "average message length of "
			  "%d bytes)", (int)PJ_ARRAY_SIZE(test_array), avg_len);

    report_ival("msg-print-per-sec", max, "msg/sec", desc);

    /* Msg print bandwidth */
    report_ival("msg-printed-bandwidth-mb", avg_len*max/1000000, "MB/sec",
	        "Message print bandwidth in megabytes (total size of "
		"SIP messages printed per second). "
		"The value is derived from msg-print-per-sec above.");


    return PJ_SUCCESS;
}

