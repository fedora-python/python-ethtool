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
#include "include/py3c/compat.h"
#include <bytesobject.h>
#include <bits/sockaddr.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <netlink/errno.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/rtnl.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include "etherinfo_struct.h"
#include "etherinfo.h"

/*
 *
 *   Internal functions for working with struct etherinfo
 *
 */


/**
 *  libnl callback function.
 *  Does the real parsing of a record returned by NETLINK.
 *  This function parses LINK related packets
 *
 * @param obj   Pointer to a struct nl_object response
 * @param arg   Pointer to a struct etherinfo element where the parse result
 *              will be saved
 */
static void callback_nl_link(struct nl_object *obj, void *arg)
{
    PyEtherInfo *ethi = (PyEtherInfo *) arg;
    struct rtnl_link *link = (struct rtnl_link *) obj;
    char hwaddr[130];

    if ((ethi == NULL) || (ethi->hwaddress != NULL)) {
        return;
    }

    memset(&hwaddr, 0, 130);
    nl_addr2str(rtnl_link_get_addr(link), hwaddr, sizeof(hwaddr));
    if (ethi->hwaddress) {
        Py_XDECREF(ethi->hwaddress);
    }
    ethi->hwaddress = PyStr_FromFormat("%s", hwaddr);
}


/**
 *  libnl callback function.  Does the real parsing of a record returned by
 *  NETLINK.  This function parses ADDRESS related packets
 *
 * @param obj   Pointer to a struct nl_object response
 * @param arg   Pointer to a struct etherinfo element where the parse result
 *              will be saved
 */
static void callback_nl_address(struct nl_object *obj, void *arg)
{
    PyObject *py_addrlist = (PyObject *) arg;
    struct rtnl_addr *rtaddr = (struct rtnl_addr *) obj;
    PyObject *addr_obj = NULL;
    int af_family = -1;

    if (py_addrlist == NULL) {
        return;
    }

    /* Ensure that we're processing only known address types.
     * Currently only IPv4 and IPv6 is handled
     */
    af_family = rtnl_addr_get_family(rtaddr);
    if (af_family != AF_INET && af_family != AF_INET6) {
        return;
    }

    /* Prepare a new Python object with the IP address */
    addr_obj = make_python_address_from_rtnl_addr(rtaddr);
    if (!addr_obj) {
        return;
    }
    /* Append the IP address object to the address list */
    PyList_Append(py_addrlist, addr_obj);
    Py_DECREF(addr_obj);
}


/**
 * Sets the etherinfo.index member to the corresponding device set in
 * etherinfo.device
 *
 * @param self A pointer the current PyEtherInfo Python object which contains
 *             the device name and the place where to save the corresponding
 *             index value.
 *
 * @return Returns 1 on success, otherwise 0.  On error, a Python error
 *         exception is set.
 */
static int _set_device_index(PyEtherInfo *self)
{
    struct nl_cache *link_cache;
    struct rtnl_link *link;

    /* Find the interface index we're looking up.
     * As we don't expect it to change, we're reusing a "cached"
     * interface index if we have that
     */
    if (self->index < 0) {
        if ((errno = rtnl_link_alloc_cache(get_nlc(),
                                           AF_UNSPEC, &link_cache)) < 0) {
            PyErr_SetString(PyExc_OSError, nl_geterror(errno));
            return 0;
        }
        link = rtnl_link_get_by_name(link_cache, PyStr_AsString(self->device));
        if (!link) {
            errno = ENODEV;
            PyErr_SetFromErrno(PyExc_IOError);
            nl_cache_free(link_cache);
            return 0;
        }
        self->index = rtnl_link_get_ifindex(link);
        if (self->index <= 0) {
            errno = ENODEV;
            PyErr_SetFromErrno(PyExc_IOError);
            rtnl_link_put(link);
            nl_cache_free(link_cache);
            return 0;
        }

        rtnl_link_put(link);
        nl_cache_free(link_cache);
    }
    return 1;
}


/*
 *
 *   Exported functions - API frontend
 *
 */

/**
 * Populate the PyEtherInfo Python object with link information for the current
 * device
 *
 * @param self  Pointer to the device object, a PyEtherInfo Python object
 *
 * @return Returns 1 on success, otherwise 0
 */
int get_etherinfo_link(PyEtherInfo *self)
{
    struct nl_cache *link_cache;
    struct rtnl_link *link;
    int err = 0;

    if (!self) {
        return 0;
    }

    /* Open a NETLINK connection on-the-fly */
    if (!open_netlink(self)) {
        PyErr_Format(PyExc_RuntimeError,
                     "Could not open a NETLINK connection for %s",
                     PyStr_AsString(self->device));
        return 0;
    }

    if (_set_device_index(self) != 1) {
        return 0;
    }

    /* Extract MAC/hardware address of the interface */
    if ((err = rtnl_link_alloc_cache(get_nlc(), AF_UNSPEC, &link_cache)) < 0) {
        PyErr_SetString(PyExc_OSError, nl_geterror(err));
        return 0;
    }
    link = rtnl_link_alloc();
    if (!link) {
        errno = ENOMEM;
        PyErr_SetFromErrno(PyExc_OSError);
        return 0;
    }
    rtnl_link_set_ifindex(link, self->index);
    nl_cache_foreach_filter(link_cache, OBJ_CAST(link),
                            callback_nl_link, self);
    rtnl_link_put(link);
    nl_cache_free(link_cache);

    return 1;
}



/**
 * Query NETLINK for device IP address configuration
 *
 * @param self   A PyEtherInfo Python object for the current device to retrieve
 *               IP address configuration data from
 * @param query  What to query for.  Must be NLQRY_ADDR4 for IPv4 addresses or
 *               NLQRY_ADDR6 for IPv6 addresses.
 *
 * @return Returns a Python list containing PyNetlinkIPaddress objects on
 *         success, otherwise NULL
 */
PyObject * get_etherinfo_address(PyEtherInfo *self, nlQuery query)
{
    struct nl_cache *addr_cache;
    struct rtnl_addr *addr;
    PyObject *addrlist = NULL;
    int err = 0;

    if (!self) {
        return NULL;
    }

    /* Open a NETLINK connection on-the-fly */
    if (!open_netlink(self)) {
        PyErr_Format(PyExc_RuntimeError,
                     "Could not open a NETLINK connection for %s",
                     PyStr_AsString(self->device));
        return NULL;
    }

    if(!_set_device_index(self)) {
        return NULL;
    }

    /* Query the for requested info via NETLINK */
    /* Extract IP address information */
    if ((err = rtnl_addr_alloc_cache(get_nlc(), &addr_cache)) < 0) {
        PyErr_SetString(PyExc_OSError, nl_geterror(err));
        nl_cache_free(addr_cache);
        return NULL;
    }

    addr = rtnl_addr_alloc();

    if (!addr) {
        errno = ENOMEM;
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    rtnl_addr_set_ifindex(addr, self->index);

    switch( query) {
    case NLQRY_ADDR4:
        rtnl_addr_set_family(addr, AF_INET);
        break;

    case NLQRY_ADDR6:
        rtnl_addr_set_family(addr, AF_INET6);
        break;

    default:
        return NULL;
    }

    /* Retrieve all address information */
    addrlist = PyList_New(0);  /* The list where to put the address object */
    assert(addrlist);

    /* Loop through all configured addresses */
    nl_cache_foreach_filter(addr_cache, OBJ_CAST(addr),
                            callback_nl_address, addrlist);
    rtnl_addr_put(addr);
    nl_cache_free(addr_cache);

    return addrlist;
}
