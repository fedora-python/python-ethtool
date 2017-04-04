/*
 * Copyright (C) 2009-2013 Red Hat Inc.
 *
 * David Sommerseth <davids@redhat.com>
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


/**
 * @file   etherinfo_struct.h
 * @author David Sommerseth <dsommers@wsdsommers.usersys.redhat.com>
 * @date   Fri Sep  4 19:06:06 2009
 *
 * @brief  Contains the internal ethtool.etherinfo data structure
 *
 */

#ifndef _ETHERINFO_STRUCT_H
#define _ETHERINFO_STRUCT_H

#include <netlink/route/addr.h>

/* Python object containing data baked from a (struct rtnl_addr) */
typedef struct PyNetlinkIPaddress {
    PyObject_HEAD
    int family;  /**< int: must be AF_INET or AF_INET6 */
    PyObject *local;  /**< string: Configured local IP address */
    PyObject *peer;  /**< string: Configured peer IP address */
    PyObject *ipv4_broadcast;  /**< string: Configured IPv4 broadcast add. */
    int prefixlen;  /**< int: Configured network prefix (netmask) */
    PyObject *scope;  /**< string: IP address scope */
} PyNetlinkIPaddress;
extern PyTypeObject ethtool_netlink_ip_address_Type;

/**
 * The Python object containing information about a single interface
 *
 */
typedef struct {
    PyObject_HEAD
    PyObject *device;  /**< Device name */
    int index;  /**< NETLINK index reference */
    PyObject *hwaddress;  /**< string: HW address / MAC address of device */
    unsigned short nlc_active;  /**< Is this instance using NETLINK? */
} PyEtherInfo;



PyObject * make_python_address_from_rtnl_addr(struct rtnl_addr *addr);


#endif
