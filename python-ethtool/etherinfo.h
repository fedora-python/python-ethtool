/* etherinfo.h - Retrieve ethernet interface info via NETLINK
 *
 * Copyright (C) 2009 Red Hat Inc.
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

#ifndef _ETHERINFO_H
#define _ETHERINFO_H

struct etherinfo {
	int index;
	char *device;
	char *hwaddress;
	char *ipv4_address;
	int ipv4_netmask;
	char *ipv4_broadcast;
	char *ipv6_address;
	int ipv6_netmask;
};

struct etherinfo *get_etherinfo();
void free_etherinfo(struct etherinfo *ptr);
void dump_etherinfo(FILE *, struct etherinfo *);

#endif
