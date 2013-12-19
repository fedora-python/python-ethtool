/* etherinfo.h - Retrieve ethernet interface info via NETLINK
 *
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

#ifndef _ETHERINFO_H
#define _ETHERINFO_H

typedef enum {NLQRY_ADDR4, NLQRY_ADDR6} nlQuery; /**<  Supported query types in the etherinfo code */

int get_etherinfo_link(etherinfo_py *data);
int get_etherinfo_address(etherinfo_py *data, nlQuery query);
void free_etherinfo(struct etherinfo *ptr);

int open_netlink(etherinfo_py *);
struct nl_sock * get_nlc();
void close_netlink(etherinfo_py *);

#endif
