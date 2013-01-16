/*
 * Copyright (C) 2009-2011 Red Hat Inc.
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

#include <netlink/route/addr.h>

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

/**
 * Contains IP address information about a particular ethernet device
 *
 */
struct etherinfo {
	char *device;                       /**< Device name */
	int index;                          /**< NETLINK index reference */
	char *hwaddress;                    /**< HW address / MAC address of device */
	PyObject *ipv4_addresses;        /**< list of PyNetlinkIPv4Address instances */
	struct ipv6address *ipv6_addresses; /**< Configured IPv6 addresses (as a pointer chain) */
};

/* Python object containing data baked from a (struct rtnl_addr) */
typedef struct PyNetlinkIPv4Address {
	PyObject_HEAD
	PyObject *ipv4_address;		/**< string: Configured IPv4 address */
	int ipv4_netmask;		/**< int: Configured IPv4 netmask */
	PyObject *ipv4_broadcast;	/**< string: Configured IPv4 broadcast address */
} PyNetlinkIPv4Address;
extern PyTypeObject ethtool_netlink_ipv4_address_Type;

/**
 * Pointer chain with IPv6 addresses associated with a ethernet interface.  Used
 * by struct etherinfo
 */
struct ipv6address {
	char *address;               /**<  Configured IPv6 address */
	int netmask;                 /**<  Configured IPv6 netmask */
	int scope;                   /**<  Scope for the IPv6 address */
	struct ipv6address *next;    /**<  Pointer to next configured IPv6 address */
};


/**
 * Contains the internal data structure of the
 * ethtool.etherinfo object.
 *
 */
struct etherinfo_obj_data {
	struct nl_handle **nlc;         /**< Contains NETLINK connection info (global) */
	unsigned int *nlc_users;	/**< Resource counter for the NETLINK connection (global) */
	unsigned short nlc_active;	/**< Is this instance using NETLINK? */
	struct etherinfo *ethinfo;      /**< Contains info about our current interface */
};

/**
 * A Python object of struct etherinfo_obj_data
 *
 */
typedef struct {
	PyObject_HEAD
	struct etherinfo_obj_data *data; /* IPv4 and IPv6 address information, only one element used */
} etherinfo_py;

/**
 * A Python object of struct ipv6address
 *
 */
typedef struct {
	PyObject_HEAD
	struct ipv6address *addrdata; /**< IPv6 address, only one element is used in this case */
} etherinfo_ipv6addr_py;

/**
 * NULL safe PyString_FromString() wrapper.  If input string is NULL, None will be returned
 *
 * @param str Input C string (char *)
 *
 * @return Returns a PyObject with either the input string wrapped up, or a Python None value.
 */
#define RETURN_STRING(str) (str ? PyString_FromString(str) : (Py_INCREF(Py_None), Py_None))

PyObject *
make_python_address_from_rtnl_addr(struct nl_object *obj,
                                   struct rtnl_addr *addr);


#endif
