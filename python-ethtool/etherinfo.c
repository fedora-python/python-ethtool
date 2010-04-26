/* etherinfo.c - Retrieve ethernet interface info via NETLINK
 *
 * Copyright (C) 2009 Red Hat Inc.
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
#include <stdlib.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <netlink/addr.h>
#include <netlink/netlink.h>
#include <netlink/handlers.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include "etherinfo.h"

/*
 *
 *   Internal functions for working with struct etherinfo
 *
 */

inline struct etherinfo *new_etherinfo_record()
{
	struct etherinfo *ptr;

	ptr = malloc(sizeof(struct etherinfo)+1);
	if( ptr ) {
		memset(ptr, 0, sizeof(struct etherinfo)+1);
	}

	return ptr;
}


void free_etherinfo(struct etherinfo *ptr)
{
	if( ptr == NULL ) { // Just for safety
		return;
	}

	free(ptr->device);

	if( ptr->hwaddress ) {
		free(ptr->hwaddress);
	}
	if( ptr->ipv4_address ) {
		free(ptr->ipv4_address);
	}
	if( ptr->ipv4_broadcast ) {
		free(ptr->ipv4_broadcast);
	}
	if( ptr->ipv6_address ) {
		free(ptr->ipv6_address);
	}
	free(ptr);
}


/*
 *  libnl callback functions
 *
 */

static void callback_nl_link(struct nl_object *obj, void *arg)
{
	struct etherinfo *ethi = (struct etherinfo *) arg;
	struct rtnl_link *link = (struct rtnl_link *) obj;
	struct nl_addr *addr = rtnl_link_get_addr(link);
	unsigned int i, len;
	unsigned char *binaddr;
	char hwaddr[130], *ptr;

	if( (ethi == NULL) || (ethi->hwaddress != NULL) ) {
		return;
	}

	binaddr = nl_addr_get_binary_addr(addr);
	memset(&hwaddr, 0, 130);
	len = 20;
	ptr = (char *)&hwaddr;
	for( i = 0; i < 6; i++ ) {
		if( i == 0 ) {
			snprintf(ptr, len, "%02X", *(binaddr+i));
			len -= 2;
			ptr += 2;
		} else {
			snprintf(ptr, len, ":%02X", *(binaddr+i));
			len -= 3;
			ptr += 3;
		}
	}
	ethi->hwaddress = strdup(hwaddr);
}


static void callback_nl_address(struct nl_object *obj, void *arg)
{
	struct etherinfo *ethi = (struct etherinfo *) arg;
	struct nl_addr *addr;
	char ip_str[66];
	int family;

	if( ethi == NULL ) {
		return;
	}

	addr = rtnl_addr_get_local((struct rtnl_addr *)obj);
	family = nl_addr_get_family(addr);
	switch( family ) {
	case AF_INET:
	case AF_INET6:
		memset(&ip_str, 0, 66);
		inet_ntop(family, nl_addr_get_binary_addr(addr), (char *)&ip_str, 64);

		if( family == AF_INET ) {
			struct nl_addr *brdcst = rtnl_addr_get_broadcast((struct rtnl_addr *)obj);
			char brdcst_str[66];

			ethi->ipv4_address = strdup(ip_str);
			ethi->ipv4_netmask = rtnl_addr_get_prefixlen((struct rtnl_addr*) obj);

			if( brdcst ) {
				memset(&brdcst_str, 0, 66);
				inet_ntop(family, nl_addr_get_binary_addr(brdcst), (char *)&brdcst_str, 64);
				ethi->ipv4_broadcast = strdup(brdcst_str);
			}
		} else {
			ethi->ipv6_address = strdup(ip_str);
			ethi->ipv6_netmask = rtnl_addr_get_prefixlen((struct rtnl_addr*) obj);
		}
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

void dump_etherinfo(FILE *fp, struct etherinfo *ptr)
{

	fprintf(fp, "*** Interface [%i] %s  ", ptr->index, ptr->device);
	if( ptr->hwaddress ) {
		fprintf(fp, "MAC address: %s", ptr->hwaddress);
	}
	fprintf(fp, "\n");
	if( ptr->ipv4_address ) {
		fprintf(fp, "\tIPv4 Address: %s/%i",
			ptr->ipv4_address, ptr->ipv4_netmask);
		if( ptr->ipv4_broadcast ) {
			fprintf(fp, "  -  Broadcast: %s", ptr->ipv4_broadcast);
		}
		fprintf(fp, "\n");
	}
	if( ptr->ipv6_address ) {
		fprintf(fp, "\tIPv6 address: %s/%i\n",
			ptr->ipv6_address, ptr->ipv6_netmask);
	}
	fprintf(fp, "\n");
}

struct etherinfo *get_etherinfo(const char *ifdevname)
{
	struct etherinfo *ethinf = NULL;
	struct nl_handle *handle;
	struct nl_cache *link_cache;
	struct nl_cache *addr_cache;
	struct rtnl_addr *addr;
	struct rtnl_link *link;
	int ifindex;

	/* Establish connection to NETLINK */
	handle = nl_handle_alloc();
	nl_connect(handle, NETLINK_ROUTE);

	/* Find the interface index we're looking up */
	link_cache = rtnl_link_alloc_cache(handle);
	ifindex = rtnl_link_name2i(link_cache, ifdevname);
	if( ifindex < 0 ) {
		return NULL;
	}

	/* Create an empty record, where ethernet information will be saved */
	ethinf = new_etherinfo_record();
	if( !ethinf ) {
		return NULL;
	}
	ethinf->index = ifindex;
	ethinf->device = strdup(ifdevname); /* Should extract via libnl - nl_link callback? */

	/* Extract MAC/hardware address of the interface */
	link = rtnl_link_alloc();
	rtnl_link_set_ifindex(link, ifindex);
	nl_cache_foreach_filter(link_cache, (struct nl_object *)link, callback_nl_link, ethinf);
	rtnl_link_put(link);
	nl_cache_free(link_cache);

	/* Extract IP address information */
	addr_cache = rtnl_addr_alloc_cache(handle);
	addr = rtnl_addr_alloc();
	rtnl_addr_set_ifindex(addr, ifindex);
	nl_cache_foreach_filter(addr_cache, (struct nl_object *)addr, callback_nl_address, ethinf);
	rtnl_addr_put(addr);
	nl_cache_free(addr_cache);

	/* Close NETLINK connection */
	nl_close(handle);
	nl_handle_destroy(handle);

	return ethinf;
}

#ifdef TESTPROG
// Simple standalone test program
int main(int argc, char **argv) {
	struct etherinfo *inf = NULL;

	inf = get_etherinfo(argv[1]);
	if( inf == NULL ) {
		fprintf(stderr, "Operation failed.  Could not retrieve ethernet information\n");
		exit(2);
	}
	dump_etherinfo(stdout, inf);
	free_etherinfo(inf);

	return 0;
}
#endif
