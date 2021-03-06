/* $Header: /pjproject/pjlib/src/pj++/pool.hpp 4     8/24/05 10:29a Bennylp $ */
/* 
 * PJLIB - PJ Foundation Library
 * (C)2003-2005 Benny Prijono <bennylp@bulukucing.org>
 *
 * Author:
 *  Benny Prijono <bennylp@bulukucing.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __PJPP_POOL_H__
#define __PJPP_POOL_H__

#include <pj/pool.h>

class PJ_Pool
{
public:
    const char *getobjname() const
    {
	return pj_pool_getobjname(this->pool_());
    }

    pj_pool_t *pool_()
    {
	return (pj_pool_t*)this;
    }

    const pj_pool_t *pool_() const
    {
	return (const pj_pool_t*)this;
    }

    void release()
    {
	pj_pool_release(this->pool_());
    }

    void reset()
    {
	pj_pool_reset(this->pool_());
    }

    pj_size_t get_capacity()
    {
	pj_pool_get_capacity(this->pool_());
    }

    pj_size_t get_used_size()
    {
	pj_pool_get_used_size(this->pool_());
    }

    void *alloc(pj_size_t size)
    {
	return pj_pool_alloc(this->pool_(), size);
    }

    void *calloc(pj_size_t count, pj_size_t elem)
    {
	return pj_pool_calloc(this->pool_(), count, elem);
    }
};

class PJ_Caching_Pool
{
public:
    void init(pj_size_t max_capacity,
	      const pj_pool_factory_policy *pol=&pj_pool_factory_default_policy)
    {
	pj_caching_pool_init(&cp_, pol, max_capacity);
    }

    void destroy()
    {
	pj_caching_pool_destroy(&cp_);
    }

    PJ_Pool *create_pool(const char *name, pj_size_t initial_size, pj_size_t increment_size, pj_pool_callback *callback)
    {
	return (PJ_Pool*) (*cp_.factory.create_pool)(&cp_.factory, name, initial_size, increment_size, callback);
    }

    void release_pool( PJ_Pool *pool )
    {
	pj_pool_release(pool->pool_());
    }

private:
    pj_caching_pool cp_;
};

#endif	/* __PJPP_POOL_H__ */
