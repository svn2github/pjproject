/* $Id$ */
/* 
 * Copyright (C)2003-2007 Benny Prijono <benny@prijono.org>
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

#if INCLUDE_MUTEX_TEST

#undef TRACE_
//#define TRACE_(x)   PJ_LOG(3,x)
#define TRACE_(x)

/* Test witn non-recursive mutex. */
static int simple_mutex_test(pj_pool_t *pool)
{
    pj_status_t rc;
    pj_mutex_t *mutex;

    PJ_LOG(3,("", "...testing simple mutex"));
    
    /* Create mutex. */
    TRACE_(("", "....create mutex"));
    rc = pj_mutex_create( pool, "", PJ_MUTEX_SIMPLE, &mutex);
    if (rc != PJ_SUCCESS) {
	app_perror("...error: pj_mutex_create", rc);
	return -10;
    }

    /* Normal lock/unlock cycle. */
    TRACE_(("", "....lock mutex"));
    rc = pj_mutex_lock(mutex);
    if (rc != PJ_SUCCESS) {
	app_perror("...error: pj_mutex_lock", rc);
	return -20;
    }
    TRACE_(("", "....unlock mutex"));
    rc = pj_mutex_unlock(mutex);
    if (rc != PJ_SUCCESS) {
	app_perror("...error: pj_mutex_unlock", rc);
	return -30;
    }
    
    /* Lock again. */
    TRACE_(("", "....lock mutex"));
    rc = pj_mutex_lock(mutex);
    if (rc != PJ_SUCCESS) return -40;

    /* Try-lock should fail. It should not deadlocked. */
    TRACE_(("", "....trylock mutex"));
    rc = pj_mutex_trylock(mutex);
    if (rc == PJ_SUCCESS)
	PJ_LOG(3,("", "...info: looks like simple mutex is recursive"));

    /* Unlock and done. */
    TRACE_(("", "....unlock mutex"));
    rc = pj_mutex_unlock(mutex);
    if (rc != PJ_SUCCESS) return -50;

    TRACE_(("", "....destroy mutex"));
    rc = pj_mutex_destroy(mutex);
    if (rc != PJ_SUCCESS) return -60;

    TRACE_(("", "....done"));
    return PJ_SUCCESS;
}


/* Test with recursive mutex. */
static int recursive_mutex_test(pj_pool_t *pool)
{
    pj_status_t rc;
    pj_mutex_t *mutex;

    PJ_LOG(3,("", "...testing recursive mutex"));

    /* Create mutex. */
    TRACE_(("", "....create mutex"));
    rc = pj_mutex_create( pool, "", PJ_MUTEX_RECURSE, &mutex);
    if (rc != PJ_SUCCESS) {
	app_perror("...error: pj_mutex_create", rc);
	return -10;
    }

    /* Normal lock/unlock cycle. */
    TRACE_(("", "....lock mutex"));
    rc = pj_mutex_lock(mutex);
    if (rc != PJ_SUCCESS) {
	app_perror("...error: pj_mutex_lock", rc);
	return -20;
    }
    TRACE_(("", "....unlock mutex"));
    rc = pj_mutex_unlock(mutex);
    if (rc != PJ_SUCCESS) {
	app_perror("...error: pj_mutex_unlock", rc);
	return -30;
    }
    
    /* Lock again. */
    TRACE_(("", "....lock mutex"));
    rc = pj_mutex_lock(mutex);
    if (rc != PJ_SUCCESS) return -40;

    /* Try-lock should NOT fail. . */
    TRACE_(("", "....trylock mutex"));
    rc = pj_mutex_trylock(mutex);
    if (rc != PJ_SUCCESS) {
	app_perror("...error: recursive mutex is not recursive!", rc);
	return -40;
    }

    /* Locking again should not fail. */
    TRACE_(("", "....lock mutex"));
    rc = pj_mutex_lock(mutex);
    if (rc != PJ_SUCCESS) {
	app_perror("...error: recursive mutex is not recursive!", rc);
	return -45;
    }

    /* Unlock several times and done. */
    TRACE_(("", "....unlock mutex 3x"));
    rc = pj_mutex_unlock(mutex);
    if (rc != PJ_SUCCESS) return -50;
    rc = pj_mutex_unlock(mutex);
    if (rc != PJ_SUCCESS) return -51;
    rc = pj_mutex_unlock(mutex);
    if (rc != PJ_SUCCESS) return -52;

    TRACE_(("", "....destroy mutex"));
    rc = pj_mutex_destroy(mutex);
    if (rc != PJ_SUCCESS) return -60;

    TRACE_(("", "....done"));
    return PJ_SUCCESS;
}

int mutex_test(void)
{
    pj_pool_t *pool;
    int rc;

    pool = pj_pool_create(mem, "", 4000, 4000, NULL);

    rc = simple_mutex_test(pool);
    if (rc != 0)
	return rc;

    rc = recursive_mutex_test(pool);
    if (rc != 0)
	return rc;

    pj_pool_release(pool);

    return 0;
}

#else
int dummy_mutex_test;
#endif

