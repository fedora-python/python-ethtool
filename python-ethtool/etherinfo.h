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

/** Supported query types in the etherinfo code */
typedef enum {NLQRY_ADDR4, NLQRY_ADDR6} nlQuery;

int get_etherinfo_link(PyEtherInfo *data);
PyObject * get_etherinfo_address(PyEtherInfo *self, nlQuery query);

int open_netlink(PyEtherInfo *);
struct nl_sock * get_nlc();
void close_netlink(PyEtherInfo *);

#endif
