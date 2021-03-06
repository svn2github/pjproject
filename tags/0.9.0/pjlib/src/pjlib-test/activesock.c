/* $Id$ */
/* 
 * Copyright (C)2003-2008 Benny Prijono <benny@prijono.org>
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
#include <pjlib.h>

/**
 * \page page_pjlib_activesock_test Test: Active Socket
 *
 * This file is <b>pjlib-test/activesock.c</b>
 *
 * \include pjlib-test/activesock.c
 */

#if INCLUDE_ACTIVESOCK_TEST

#define THIS_FILE   "activesock.c"


/*******************************************************************
 * Simple UDP echo server.
 */
struct udp_echo_srv
{
    pj_activesock_t	*asock;
    pj_bool_t		 echo_enabled;
    pj_uint16_t		 port;
    pj_ioqueue_op_key_t	 send_key;
    pj_status_t		 status;
    unsigned		 rx_cnt;
    unsigned		 rx_err_cnt, tx_err_cnt;
};

static void udp_echo_err(const char *title, pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));
    PJ_LOG(3,(THIS_FILE, "   error: %s: %s", title, errmsg));
}

static pj_bool_t udp_echo_srv_on_data_recvfrom(pj_activesock_t *asock,
					       void *data,
					       pj_size_t size,
					       const pj_sockaddr_t *src_addr,
					       int addr_len,
					       pj_status_t status)
{
    struct udp_echo_srv *srv;
    pj_ssize_t sent;


    srv = (struct udp_echo_srv*) pj_activesock_get_user_data(asock);

    if (status != PJ_SUCCESS) {
	srv->status = status;
	srv->rx_err_cnt++;
	udp_echo_err("recvfrom() callback", status);
	return PJ_TRUE;
    }

    srv->rx_cnt++;

    /* Send back if echo is enabled */
    if (srv->echo_enabled) {
	sent = size;
	srv->status = pj_activesock_sendto(asock, &srv->send_key, data, 
					   &sent, 0,
					   src_addr, addr_len);
	if (srv->status != PJ_SUCCESS) {
	    srv->tx_err_cnt++;
	    udp_echo_err("sendto()", status);
	}
    }

    return PJ_TRUE;
}


static pj_status_t udp_echo_srv_create(pj_pool_t *pool,
				       pj_ioqueue_t *ioqueue,
				       pj_bool_t enable_echo,
				       struct udp_echo_srv **p_srv)
{
    struct udp_echo_srv *srv;
    pj_sock_t sock_fd = PJ_INVALID_SOCKET;
    pj_sockaddr addr;
    int addr_len;
    pj_activesock_cb activesock_cb;
    pj_status_t status;

    srv = PJ_POOL_ZALLOC_T(pool, struct udp_echo_srv);
    srv->echo_enabled = enable_echo;

    pj_sockaddr_in_init(&addr.ipv4, NULL, 0);
    addr_len = sizeof(addr);

    pj_bzero(&activesock_cb, sizeof(activesock_cb));
    activesock_cb.on_data_recvfrom = &udp_echo_srv_on_data_recvfrom;

    status = pj_activesock_create_udp(pool, &addr, NULL, ioqueue, &activesock_cb, 
				      srv, &srv->asock, &addr);
    if (status != PJ_SUCCESS) {
	pj_sock_close(sock_fd);
	udp_echo_err("pj_activesock_create()", status);
	return status;
    }

    srv->port = pj_ntohs(addr.ipv4.sin_port);

    pj_ioqueue_op_key_init(&srv->send_key, sizeof(srv->send_key));

    status = pj_activesock_start_recvfrom(srv->asock, pool, 32, 0);
    if (status != PJ_SUCCESS) {
	pj_activesock_close(srv->asock);
	udp_echo_err("pj_activesock_start_recvfrom()", status);
	return status;
    }


    *p_srv = srv;
    return PJ_SUCCESS;
}

static void udp_echo_srv_destroy(struct udp_echo_srv *srv)
{
    pj_activesock_close(srv->asock);
}

/*******************************************************************
 * UDP ping pong test (send packet back and forth between two UDP echo
 * servers.
 */
static int udp_ping_pong_test(void)
{
    pj_ioqueue_t *ioqueue = NULL;
    pj_pool_t *pool = NULL;
    struct udp_echo_srv *srv1=NULL, *srv2=NULL;
    pj_bool_t need_send = PJ_TRUE;
    unsigned data = 0;
    int count, ret;
    pj_status_t status;

    pool = pj_pool_create(mem, "pingpong", 512, 512, NULL);
    if (!pool)
	return -10;

    status = pj_ioqueue_create(pool, 4, &ioqueue);
    if (status != PJ_SUCCESS) {
	ret = -20;
	udp_echo_err("pj_ioqueue_create()", status);
	goto on_return;
    }

    status = udp_echo_srv_create(pool, ioqueue, PJ_TRUE, &srv1);
    if (status != PJ_SUCCESS) {
	ret = -30;
	goto on_return;
    }

    status = udp_echo_srv_create(pool, ioqueue, PJ_TRUE, &srv2);
    if (status != PJ_SUCCESS) {
	ret = -40;
	goto on_return;
    }

    /* initiate the first send */
    for (count=0; count<1000; ++count) {
	unsigned last_rx1, last_rx2;
	unsigned i;

	if (need_send) {
	    pj_str_t loopback;
	    pj_sockaddr_in addr;
	    pj_ssize_t sent;

	    ++data;

	    sent = sizeof(data);
	    loopback = pj_str("127.0.0.1");
	    pj_sockaddr_in_init(&addr, &loopback, srv2->port);
	    status = pj_activesock_sendto(srv1->asock, &srv1->send_key,
					  &data, &sent, 0,
					  &addr, sizeof(addr));
	    if (status != PJ_SUCCESS && status != PJ_EPENDING) {
		ret = -50;
		udp_echo_err("sendto()", status);
		goto on_return;
	    }

	    need_send = PJ_FALSE;
	}

	last_rx1 = srv1->rx_cnt;
	last_rx2 = srv2->rx_cnt;

	for (i=0; i<10 && last_rx1 == srv1->rx_cnt && last_rx2 == srv2->rx_cnt; ++i) {
	    pj_time_val delay = {0, 10};
	    pj_ioqueue_poll(ioqueue, &delay);
	}

	if (srv1->rx_err_cnt+srv1->tx_err_cnt != 0 ||
	    srv2->rx_err_cnt+srv2->tx_err_cnt != 0)
	{
	    /* Got error */
	    ret = -60;
	    goto on_return;
	}

	if (last_rx1 == srv1->rx_cnt && last_rx2 == srv2->rx_cnt) {
	    /* Packet lost */
	    ret = -70;
	    udp_echo_err("packets have been lost", PJ_ETIMEDOUT);
	    goto on_return;
	}
    }

    ret = 0;

on_return:
    if (srv2)
	udp_echo_srv_destroy(srv2);
    if (srv1)
	udp_echo_srv_destroy(srv1);
    if (ioqueue)
	pj_ioqueue_destroy(ioqueue);
    if (pool)
	pj_pool_release(pool);
    
    return ret;
}


int activesock_test(void)
{
    int ret;

    PJ_LOG(3,("", "..udp ping/pong test"));
    ret = udp_ping_pong_test();
    if (ret != 0)
	return ret;

    return 0;
}

#else	/* INCLUDE_ACTIVESOCK_TEST */
/* To prevent warning about "translation unit is empty"
 * when this test is disabled. 
 */
int dummy_active_sock_test;
#endif	/* INCLUDE_ACTIVESOCK_TEST */

