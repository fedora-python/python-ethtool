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

/**
 * Contains IP address information about a particular ethernet device
 *
 */
struct etherinfo {
	char *device;                       /**< Device name */
	int index;                          /**< NETLINK index reference */
	PyObject *hwaddress;             /**< string: HW address / MAC address of device */
	PyObject *ipv4_addresses;        /**< list of PyNetlinkIPv4Address instances */
	PyObject *ipv6_addresses;        /**< list of PyNetlinkIPv6Addresses instances */
};

/* Python object containing data baked from a (struct rtnl_addr) */
typedef struct PyNetlinkIPaddress {
	PyObject_HEAD
        int family;                     /**< int: must be AF_INET or AF_INET6 */
	PyObject *local;		/**< string: Configured local IP address */
	PyObject *peer;		        /**< string: Configured peer IP address */
	PyObject *ipv4_broadcast;	/**< string: Configured IPv4 broadcast address */
	int prefixlen;		        /**< int: Configured network prefix (netmask) */
        PyObject *scope;                /**< string: IP address scope */
} PyNetlinkIPaddress;
extern PyTypeObject ethtool_netlink_ip_address_Type;

/**
 * Contains the internal data structure of the
 * ethtool.etherinfo object.
 *
 */
struct etherinfo_obj_data {
	struct nl_sock **nlc;           /**< Contains NETLINK connection info (global) */
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



PyObject * make_python_address_from_rtnl_addr(struct rtnl_addr *addr);


#endif
