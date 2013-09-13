/* etherinfo.c - Retrieve ethernet interface info via NETLINK
 *
 * Copyright (C) 2009-2013 Red Hat Inc.
 *
 * David Sommerseth <davids@redhat.com>
 * Parts of this code is based on ideas and solutions in iproute2
 *
 * This application is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2.
 *
 * This application is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <Python.h>
#include <bits/sockaddr.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/rtnl.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include "etherinfo_struct.h"
#include "etherinfo.h"

pthread_mutex_t nlc_counter_mtx = PTHREAD_MUTEX_INITIALIZER;

/*
 *
 *   Internal functions for working with struct etherinfo
 *
 */

/**
 * Simple macro which makes sure the destination string is freed if used earlier.
 *
 * @param dst Destination pointer
 * @param src Source pointer
 *
 */
#define SET_STR_VALUE(dst, src) {	 \
	if( dst ) {		 \
		free(dst);	 \
	};			 \
	dst = strdup(src);	 \
	}

/**
 * Frees the memory used by struct etherinfo
 *
 * @param ptr Pointer to a struct etherninfo element
 */
void free_etherinfo(struct etherinfo *ptr)
{
	if( ptr == NULL ) { // Just for safety
		return;
	}

	free(ptr->device);

	if( ptr->hwaddress ) {
		free(ptr->hwaddress);
	}
	Py_XDECREF(ptr->ipv4_addresses);

	free(ptr);
}


/**
 *  libnl callback function.  Does the real parsing of a record returned by NETLINK.  This function
 *  parses LINK related packets
 *
 * @param obj   Pointer to a struct nl_object response
 * @param arg   Pointer to a struct etherinfo element where the parse result will be saved
 */
static void callback_nl_link(struct nl_object *obj, void *arg)
{
	struct etherinfo *ethi = (struct etherinfo *) arg;
	struct rtnl_link *link = (struct rtnl_link *) obj;
	char hwaddr[130];

	if( (ethi == NULL) || (ethi->hwaddress != NULL) ) {
		return;
	}

	memset(&hwaddr, 0, 130);
	nl_addr2str(rtnl_link_get_addr(link), hwaddr, sizeof(hwaddr));
	SET_STR_VALUE(ethi->hwaddress, hwaddr);
}

/**
 * For use by callback_nl_address
 * Returns 0 for success; -1 for error (though this is currently ignored)
 */
static int
append_object_for_netlink_address(struct etherinfo *ethi,
                                  struct rtnl_addr *addr)
{
	PyObject *addr_obj;

	assert(ethi);
	assert(ethi->ipv4_addresses);
	assert(addr);

	addr_obj = make_python_address_from_rtnl_addr(addr);
	if (!addr_obj) {
	  return -1;
	}

	if (-1 == PyList_Append(ethi->ipv4_addresses, addr_obj)) {
	  Py_DECREF(addr_obj);
	  return -1;
	}

	Py_DECREF(addr_obj);

	/* Success */
	return 0;
}

/**
 *  libnl callback function.  Does the real parsing of a record returned by NETLINK.  This function
 *  parses ADDRESS related packets
 *
 * @param obj   Pointer to a struct nl_object response
 * @param arg   Pointer to a struct etherinfo element where the parse result will be saved
 */
static void callback_nl_address(struct nl_object *obj, void *arg)
{
	struct etherinfo *ethi = (struct etherinfo *) arg;
	struct rtnl_addr *rtaddr = (struct rtnl_addr *) obj;

	if( ethi == NULL ) {
		return;
	}

	switch( rtnl_addr_get_family(rtaddr) ) {
	case AF_INET:
                append_object_for_netlink_address(ethi, rtaddr);
                return;
	default:
		return;
	}
}



/*
 *
 *   Exported functions - API frontend
 *
 */


/**
 * Query NETLINK for ethernet configuration
 *
 * @param ethinf Pointer to an available struct etherinfo element.  The 'device' member
 *               must contain a valid string to the device to query for information
 * @param nlc    Pointer to the libnl handle, which is used for the query against NETLINK
 * @param query  What to query for.  Must be NLQRY_LINK or NLQRY_ADDR.
 *
 * @return Returns 1 on success, otherwise 0.
 */
int get_etherinfo(struct etherinfo_obj_data *data, nlQuery query)
{
	struct nl_cache *link_cache;
	struct nl_cache *addr_cache;
	struct rtnl_addr *addr;
	struct rtnl_link *link;
	struct etherinfo *ethinf = NULL;
	int ret = 0;

	if( !data || !data->ethinfo ) {
		return 0;
	}
	ethinf = data->ethinfo;

	/* Open a NETLINK connection on-the-fly */
	if( !open_netlink(data) ) {
		PyErr_Format(PyExc_RuntimeError,
			     "Could not open a NETLINK connection for %s",
			     ethinf->device);
		return 0;
	}

	/* Find the interface index we're looking up.
	 * As we don't expect it to change, we're reusing a "cached"
	 * interface index if we have that
	 */
	if( ethinf->index < 0 ) {
		if( rtnl_link_alloc_cache(*data->nlc, AF_UNSPEC, &link_cache) < 0) {
                        return 0;
                }

                link = rtnl_link_get_by_name(link_cache, ethinf->device);
                if( !link ) {
                        return 0;
                }

		ethinf->index = rtnl_link_get_ifindex(link);
		if( ethinf->index < 0 ) {
			return 0;
		}
		nl_cache_free(link_cache);
	}

	/* Query the for requested info vai NETLINK */
	switch( query ) {
	case NLQRY_LINK:
		/* Extract MAC/hardware address of the interface */
		if( rtnl_link_alloc_cache(*data->nlc, AF_UNSPEC, &link_cache) < 0) {
                        return 0;
                }
		link = rtnl_link_alloc();
		rtnl_link_set_ifindex(link, ethinf->index);
		nl_cache_foreach_filter(link_cache, OBJ_CAST(link), callback_nl_link, ethinf);
		rtnl_link_put(link);
		nl_cache_free(link_cache);
		ret = 1;
		break;

	case NLQRY_ADDR:
		/* Extract IP address information */
		if( rtnl_addr_alloc_cache(*data->nlc, &addr_cache) < 0) {
                        return 0;
                }
		addr = rtnl_addr_alloc();
		rtnl_addr_set_ifindex(addr, ethinf->index);

                /* Make sure we don't have any old IPv4 addresses saved */
                Py_XDECREF(ethinf->ipv4_addresses);
                ethinf->ipv4_addresses = PyList_New(0);
                if (!ethinf->ipv4_addresses) {
                        return 0;
                }

                /* Retrieve all address information */
		nl_cache_foreach_filter(addr_cache, OBJ_CAST(addr), callback_nl_address, ethinf);
		rtnl_addr_put(addr);
		nl_cache_free(addr_cache);
		ret = 1;
		break;

	default:
		ret = 0;
	}
	return ret;
}


/**
 * Connects to the NETLINK interface.  This will be called
 * for each etherinfo object being generated, and it will
 * keep a separate file descriptor open for each object
 *
 * @param data etherinfo_obj_data structure
 *
 * @return Returns 1 on success, otherwise 0.
 */
int open_netlink(struct etherinfo_obj_data *data)
{
	if( !data ) {
		return 0;
	}

	/* Reuse already established NETLINK connection, if a connection exists */
	if( *data->nlc ) {
		/* If this object has not used NETLINK earlier, tag it as a user */
		if( !data->nlc_active ) {
			pthread_mutex_lock(&nlc_counter_mtx);
			(*data->nlc_users)++;
			pthread_mutex_unlock(&nlc_counter_mtx);
		}
		data->nlc_active = 1;
		return 1;
	}

	/* No earlier connections exists, establish a new one */
	*data->nlc = nl_socket_alloc();
	nl_connect(*data->nlc, NETLINK_ROUTE);
	if( (*data->nlc != NULL) ) {
		/* Force O_CLOEXEC flag on the NETLINK socket */
		if( fcntl(nl_socket_get_fd(*data->nlc), F_SETFD, FD_CLOEXEC) == -1 ) {
			fprintf(stderr,
				"**WARNING** Failed to set O_CLOEXEC on NETLINK socket: %s\n",
				strerror(errno));
		}

		/* Tag this object as an active user */
		pthread_mutex_lock(&nlc_counter_mtx);
		(*data->nlc_users)++;
		pthread_mutex_unlock(&nlc_counter_mtx);
		data->nlc_active = 1;
		return 1;
	} else {
		return 0;
	}
}


/**
 * Closes the NETLINK connection.  This should be called automatically whenever
 * the corresponding etherinfo object is deleted.
 *
 * @param ptr  Pointer to the pointer of struct nl_handle, which contains the NETLINK connection
 */
void close_netlink(struct etherinfo_obj_data *data)
{
	if( !data || !(*data->nlc) ) {
		return;
	}

	/* Untag this object as a NETLINK user */
	data->nlc_active = 0;
	pthread_mutex_lock(&nlc_counter_mtx);
	(*data->nlc_users)--;
	pthread_mutex_unlock(&nlc_counter_mtx);

	/* Don't close the connection if there are more users */
	if( *data->nlc_users > 0) {
		return;
	}

	/* Close NETLINK connection */
	nl_close(*data->nlc);
	nl_socket_free(*data->nlc);
	*data->nlc = NULL;
}
