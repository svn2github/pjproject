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
#include <pjsip/sip_resolve.h>
#include <pjsip/sip_transport.h>
#include <pjsip/sip_errno.h>
#include <pjlib-util/errno.h>
#include <pjlib-util/srv_resolver.h>
#include <pj/array.h>
#include <pj/assert.h>
#include <pj/ctype.h>
#include <pj/log.h>
#include <pj/pool.h>
#include <pj/rand.h>
#include <pj/string.h>


#define THIS_FILE   "sip_resolve.c"

#define ADDR_MAX_COUNT	    8

struct naptr_target
{
    pj_str_t		    res_type;	    /**< e.g. "_sip._udp"   */
    pj_str_t		    name;	    /**< Domain name.	    */
    pjsip_transport_type_e  type;	    /**< Transport type.    */
    unsigned		    order;	    /**< Order		    */
    unsigned		    pref;	    /**< Preference.	    */
};

struct query
{
    char		    *objname;

    pj_dns_type		     query_type;
    void		    *token;
    pjsip_resolver_callback *cb;
    pj_dns_async_query	    *object;
    pj_status_t		     last_error;

    /* Original request: */
    struct {
	pjsip_host_info	     target;
	unsigned	     def_port;
    } req;

    /* NAPTR records: */
    unsigned		     naptr_cnt;
    struct naptr_target	     naptr[8];
};


struct pjsip_resolver_t
{
    pj_dns_resolver *res;
};


static void srv_resolver_cb(void *user_data,
			    pj_status_t status,
			    const pj_dns_srv_record *rec);
static void dns_a_callback(void *user_data,
			   pj_status_t status,
			   pj_dns_parsed_packet *response);


/*
 * Public API to create the resolver.
 */
PJ_DEF(pj_status_t) pjsip_resolver_create( pj_pool_t *pool,
					   pjsip_resolver_t **p_res)
{
    pjsip_resolver_t *resolver;

    PJ_ASSERT_RETURN(pool && p_res, PJ_EINVAL);
    resolver = PJ_POOL_ZALLOC_T(pool, pjsip_resolver_t);
    *p_res = resolver;

    return PJ_SUCCESS;
}


/*
 * Public API to set the DNS resolver instance for the SIP resolver.
 */
PJ_DEF(pj_status_t) pjsip_resolver_set_resolver(pjsip_resolver_t *res,
						pj_dns_resolver *dns_res)
{
#if PJSIP_HAS_RESOLVER
    res->res = dns_res;
    return PJ_SUCCESS;
#else
    PJ_UNUSED_ARG(res);
    PJ_UNUSED_ARG(dns_res);
    pj_assert(!"Resolver is disabled (PJSIP_HAS_RESOLVER==0)");
    return PJ_EINVALIDOP;
#endif
}


/*
 * Public API to get the internal DNS resolver.
 */
PJ_DEF(pj_dns_resolver*) pjsip_resolver_get_resolver(pjsip_resolver_t *res)
{
    return res->res;
}


/*
 * Public API to create destroy the resolver
 */
PJ_DEF(void) pjsip_resolver_destroy(pjsip_resolver_t *resolver)
{
    if (resolver->res) {
#if PJSIP_HAS_RESOLVER
	pj_dns_resolver_destroy(resolver->res, PJ_FALSE);
#endif
	resolver->res = NULL;
    }
}

/*
 * Internal:
 *  determine if an address is a valid IP address.
 */
static int is_str_ip(const pj_str_t *host)
{
    const char *p = host->ptr;
    const char *end = ((const char*)host->ptr) + host->slen;

    while (p != end) {
	if (pj_isdigit(*p) || *p=='.') {
	    ++p;
	} else {
	    return 0;
	}
    }
    return 1;
}


/*
 * This is the main function for performing server resolution.
 */
PJ_DEF(void) pjsip_resolve( pjsip_resolver_t *resolver,
			    pj_pool_t *pool,
			    const pjsip_host_info *target,
			    void *token,
			    pjsip_resolver_callback *cb)
{
    pjsip_server_addresses svr_addr;
    pj_status_t status = PJ_SUCCESS;
    int is_ip_addr;
    struct query *query;
    pjsip_transport_type_e type = target->type;

    /* Is it IP address or hostname?. */
    is_ip_addr = is_str_ip(&target->addr.host);

    /* Set the transport type if not explicitly specified. 
     * RFC 3263 section 4.1 specify rules to set up this.
     */
    if (type == PJSIP_TRANSPORT_UNSPECIFIED) {
	if (is_ip_addr || (target->addr.port != 0)) {
#if PJ_HAS_TCP
	    if (target->flag & PJSIP_TRANSPORT_SECURE) 
	    {
		type = PJSIP_TRANSPORT_TLS;
	    } else if (target->flag & PJSIP_TRANSPORT_RELIABLE) 
	    {
		type = PJSIP_TRANSPORT_TCP;
	    } else 
#endif
	    {
		type = PJSIP_TRANSPORT_UDP;
	    }
	} else {
	    /* No type or explicit port is specified, and the address is
	     * not IP address.
	     * In this case, full resolution must be performed.
	     * But we don't support it (yet).
	     */
#if PJ_HAS_TCP
	    if (target->flag & PJSIP_TRANSPORT_SECURE) 
	    {
		type = PJSIP_TRANSPORT_TLS;
	    } else if (target->flag & PJSIP_TRANSPORT_RELIABLE) 
	    {
		type = PJSIP_TRANSPORT_TCP;
	    } else 
#endif
	    {
		type = PJSIP_TRANSPORT_UDP;
	    }
	}
    }


    /* If target is an IP address, or if resolver is not configured, 
     * we can just finish the resolution now using pj_gethostbyname()
     */
    if (is_ip_addr || resolver->res == NULL) {

	pj_in_addr ip_addr;
	pj_uint16_t srv_port;

	if (!is_ip_addr) {
	    PJ_LOG(5,(THIS_FILE, 
		      "DNS resolver not available, target '%.*s:%d' type=%s "
		      "will be resolved with gethostbyname()",
		      target->addr.host.slen,
		      target->addr.host.ptr,
		      target->addr.port,
		      pjsip_transport_get_type_name(target->type)));
	}

	/* Set the port number if not specified. */
	if (target->addr.port == 0) {
	   srv_port = (pj_uint16_t)
		      pjsip_transport_get_default_port_for_type(type);
	} else {
	   srv_port = (pj_uint16_t)target->addr.port;
	}

	/* This will eventually call pj_gethostbyname() if the host
	 * is not an IP address.
	 */
	status = pj_sockaddr_in_init((pj_sockaddr_in*)&svr_addr.entry[0].addr,
				      &target->addr.host, srv_port);
	if (status != PJ_SUCCESS)
	    goto on_error;

	/* Call the callback. */
	ip_addr = ((pj_sockaddr_in*)&svr_addr.entry[0].addr)->sin_addr;
	PJ_LOG(5,(THIS_FILE, 
		  "Target '%.*s:%d' type=%s resolved to "
		  "'%s:%d' type=%s",
		  (int)target->addr.host.slen,
		  target->addr.host.ptr,
		  target->addr.port,
		  pjsip_transport_get_type_name(target->type),
		  pj_inet_ntoa(ip_addr),
		  srv_port,
		  pjsip_transport_get_type_name(type)));
	svr_addr.count = 1;
	svr_addr.entry[0].priority = 0;
	svr_addr.entry[0].weight = 0;
	svr_addr.entry[0].type = type;
	svr_addr.entry[0].addr_len = sizeof(pj_sockaddr_in);
	(*cb)(status, token, &svr_addr);

	/* Done. */
	return;
    }

    /* Target is not an IP address so we need to resolve it. */
#if PJSIP_HAS_RESOLVER

    /* Build the query state */
    query = PJ_POOL_ZALLOC_T(pool, struct query);
    query->objname = THIS_FILE;
    query->token = token;
    query->cb = cb;
    query->req.target = *target;
    pj_strdup(pool, &query->req.target.addr.host, &target->addr.host);

    /* If port is not specified, start with SRV resolution
     * (should be with NAPTR, but we'll do that later)
     */
    PJ_TODO(SUPPORT_DNS_NAPTR);

    /* Build dummy NAPTR entry */
    query->naptr_cnt = 1;
    pj_bzero(&query->naptr[0], sizeof(query->naptr[0]));
    query->naptr[0].order = 0;
    query->naptr[0].pref = 0;
    query->naptr[0].type = type;
    pj_strdup(pool, &query->naptr[0].name, &target->addr.host);


    /* Start DNS SRV or A resolution, depending on whether port is specified */
    if (target->addr.port == 0) {
	query->query_type = PJ_DNS_TYPE_SRV;

	query->req.def_port = 5060;

	if (type == PJSIP_TRANSPORT_TLS) {
	    query->naptr[0].res_type = pj_str("_sips._tcp.");
	    query->req.def_port = 5061;
	} else if (type == PJSIP_TRANSPORT_TCP)
	    query->naptr[0].res_type = pj_str("_sip._tcp.");
	else if (type == PJSIP_TRANSPORT_UDP)
	    query->naptr[0].res_type = pj_str("_sip._udp.");
	else {
	    pj_assert(!"Unknown transport type");
	    query->naptr[0].res_type = pj_str("_sip._udp.");
	    
	}

    } else {
	/* Otherwise if port is specified, start with A (or AAAA) host 
	 * resolution 
	 */
	query->query_type = PJ_DNS_TYPE_A;
	query->naptr[0].res_type.slen = 0;
	query->req.def_port = target->addr.port;
    }

    /* Start the asynchronous query */
    PJ_LOG(5, (query->objname, 
	       "Starting async DNS %s query: target=%.*s%.*s, transport=%s, "
	       "port=%d",
	       pj_dns_get_type_name(query->query_type),
	       (int)query->naptr[0].res_type.slen,
	       query->naptr[0].res_type.ptr,
	       (int)query->naptr[0].name.slen, query->naptr[0].name.ptr,
	       pjsip_transport_get_type_name(target->type),
	       target->addr.port));

    if (query->query_type == PJ_DNS_TYPE_SRV) {

	status = pj_dns_srv_resolve(&query->naptr[0].name,
				    &query->naptr[0].res_type,
				    query->req.def_port, pool, resolver->res,
				    PJ_TRUE, query, &srv_resolver_cb, NULL);

    } else if (query->query_type == PJ_DNS_TYPE_A) {

	status = pj_dns_resolver_start_query(resolver->res, 
					     &query->naptr[0].name,
					     PJ_DNS_TYPE_A, 0, 
					     &dns_a_callback,
    					     query, &query->object);

    } else {
	pj_assert(!"Unexpected");
	status = PJ_EBUG;
    }

    if (status != PJ_SUCCESS)
	goto on_error;

    return;

#else /* PJSIP_HAS_RESOLVER */
    PJ_UNUSED_ARG(pool);
    PJ_UNUSED_ARG(query);
    PJ_UNUSED_ARG(srv_name);
#endif /* PJSIP_HAS_RESOLVER */

on_error:
    if (status != PJ_SUCCESS) {
	char errmsg[PJ_ERR_MSG_SIZE];
	PJ_LOG(4,(THIS_FILE, "Failed to resolve '%.*s'. Err=%d (%s)",
			     (int)target->addr.host.slen,
			     target->addr.host.ptr,
			     status,
			     pj_strerror(status,errmsg,sizeof(errmsg)).ptr));
	(*cb)(status, token, NULL);
	return;
    }
}

#if PJSIP_HAS_RESOLVER

/* 
 * This callback is called when target is resolved with DNS A query.
 */
static void dns_a_callback(void *user_data,
			   pj_status_t status,
			   pj_dns_parsed_packet *pkt)
{
    struct query *query = (struct query*) user_data;
    pjsip_server_addresses srv;
    pj_dns_a_record rec;
    unsigned i;

    rec.addr_count = 0;

    /* Parse the response */
    if (status == PJ_SUCCESS) {
	status = pj_dns_parse_a_response(pkt, &rec);
    }

    if (status != PJ_SUCCESS) {
	char errmsg[PJ_ERR_MSG_SIZE];

	/* Log error */
	pj_strerror(status, errmsg, sizeof(errmsg));
	PJ_LOG(4,(query->objname, "DNS A record resolution failed: %s", 
		  errmsg));

	/* Call the callback */
	(*query->cb)(status, query->token, NULL);
	return;
    }

    /* Build server addresses and call callback */
    srv.count = 0;
    for (i=0; i<rec.addr_count; ++i) {
	srv.entry[srv.count].type = query->naptr[0].type;
	srv.entry[srv.count].priority = 0;
	srv.entry[srv.count].weight = 0;
	srv.entry[srv.count].addr_len = sizeof(pj_sockaddr_in);
	pj_sockaddr_in_init(&srv.entry[srv.count].addr.ipv4,
			    0, (pj_uint16_t)query->req.def_port);
	srv.entry[srv.count].addr.ipv4.sin_addr.s_addr =
	    rec.addr[i].s_addr;

	++srv.count;
    }

    /* Call the callback */
    (*query->cb)(PJ_SUCCESS, query->token, &srv);
}


/* Callback to be called by DNS SRV resolution */
static void srv_resolver_cb(void *user_data,
			    pj_status_t status,
			    const pj_dns_srv_record *rec)
{
    struct query *query = (struct query*) user_data;
    pjsip_server_addresses srv;
    unsigned i;

    if (status != PJ_SUCCESS) {
	char errmsg[PJ_ERR_MSG_SIZE];

	/* Log error */
	pj_strerror(status, errmsg, sizeof(errmsg));
	PJ_LOG(4,(query->objname, "DNS A record resolution failed: %s", 
		  errmsg));

	/* Call the callback */
	(*query->cb)(status, query->token, NULL);
	return;
    }

    /* Build server addresses and call callback */
    srv.count = 0;
    for (i=0; i<rec->count; ++i) {
	unsigned j;

	for (j=0; j<rec->entry[i].server.addr_count; ++j) {
	    srv.entry[srv.count].type = query->naptr[0].type;
	    srv.entry[srv.count].priority = rec->entry[i].priority;
	    srv.entry[srv.count].weight = rec->entry[i].weight;
	    srv.entry[srv.count].addr_len = sizeof(pj_sockaddr_in);
	    pj_sockaddr_in_init(&srv.entry[srv.count].addr.ipv4,
				0, (pj_uint16_t)rec->entry[i].port);
	    srv.entry[srv.count].addr.ipv4.sin_addr.s_addr =
		rec->entry[i].server.addr[j].s_addr;

	    ++srv.count;
	}
    }

    /* Call the callback */
    (*query->cb)(PJ_SUCCESS, query->token, &srv);
}

#endif	/* PJSIP_HAS_RESOLVER */

