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

#include <bits/sockaddr.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include "etherinfo.h"

#define GET_LINK 1
#define GET_IPV4 2
#define GET_IPV6 4

struct nl_request {
	struct nlmsghdr		nlmsg_info;
	struct ifaddrmsg	ifaddrmsg_info;
};

/*
 *
 *   Internal functions for working with struct etherinfo
 *
 */

inline struct etherinfo *new_etherinfo_record()
{
	struct etherinfo *ptr;

	ptr = (struct etherinfo *) malloc(sizeof(struct etherinfo)+1);
	if( ptr ) {
		memset(ptr, 0, sizeof(struct etherinfo)+1);
	}

	return ptr;
}


#define SET_STRVALUE(ptr, val) if ( (ptr == NULL) && (val != NULL) ) ptr = val;

int update_etherinfo(struct etherinfo *ipadrchain, int index, int af_type,
                        char *ipadr, int ipmask, char *ipv4brd)
{
        struct etherinfo *ptr = NULL;

        // Look up the record we will update
        for( ptr = ipadrchain; ptr != NULL; ptr = ptr->next) {
                if( ptr->index == index ) {
                        break;
                }
        }
        if( ptr == NULL ) {
                return 0;
        }

        switch( af_type ) {
        case AF_INET:
                SET_STRVALUE(ptr->ipv4_address, ipadr);
                ptr->ipv4_netmask = ipmask;
                SET_STRVALUE(ptr->ipv4_broadcast, ipv4brd);
                break;
        case AF_INET6:
                SET_STRVALUE(ptr->ipv6_address, ipadr);
                ptr->ipv6_netmask = ipmask;
                break;
        }
        return 1;
}


void free_etherinfo(struct etherinfo *ptr)
{
	if( ptr == NULL ) { // Just for safety
		return;
	}
	if( ptr->next != NULL ) {
		free_etherinfo(ptr->next);
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
 *
 *   NETLINK specific functions
 *
 */

int open_netlink_socket(struct sockaddr_nl *local)
{
	int fd;

	assert( local != NULL && local->nl_family == AF_NETLINK );

	fd = socket(local->nl_family, SOCK_RAW, NETLINK_ROUTE);
	if(fd < 0) {
		PyErr_SetString(PuExc_OSError, strerror(errno));
		return -1;
	}

	if(bind(fd, (struct sockaddr*) local, sizeof(*local)) < 0) {
		PyErr_SetString(PuExc_OSError, strerror(errno));
		return -1;
	}

	return fd;
}


int send_netlink_query(int fd, int get_type)
{
	struct sockaddr_nl peer;
	struct msghdr msg_info;
	struct nl_request netlink_req;
	struct iovec iov_info;

	memset(&peer, 0, sizeof(peer));
	peer.nl_family = AF_NETLINK;
	peer.nl_pad = 0;
	peer.nl_pid = 0;
	peer.nl_groups = 0;

	memset(&msg_info, 0, sizeof(msg_info));
	msg_info.msg_name = (void *) &peer;
	msg_info.msg_namelen = sizeof(peer);

	memset(&netlink_req, 0, sizeof(netlink_req));
	netlink_req.nlmsg_info.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	netlink_req.nlmsg_info.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	netlink_req.nlmsg_info.nlmsg_pid = getpid();

        // Set the information we want to query for
	netlink_req.nlmsg_info.nlmsg_type = (get_type == GET_LINK ? RTM_GETLINK : RTM_GETADDR);
	netlink_req.ifaddrmsg_info.ifa_family = (get_type == GET_IPV6 ? AF_INET6 : AF_INET);

	iov_info.iov_base = (void *) &netlink_req.nlmsg_info;
	iov_info.iov_len = netlink_req.nlmsg_info.nlmsg_len;
	msg_info.msg_iov = &iov_info;
	msg_info.msg_iovlen = 1;

	if( sendmsg(fd, &msg_info, 0) < 0 ) {
		PyErr_SetString(PuExc_OSError, strerror(errno));
		return 0;
	}
	return 1;
}


int read_netlink_results(int fd, struct sockaddr_nl *local,
			 int (*callback)(struct nlmsghdr *, struct etherinfo *, struct etherinfo **idxptr),
			 struct etherinfo *ethinf)
{
        struct etherinfo *process_ethinfo_idxptr = NULL;  // Int. index ptr for callback function
	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr msg;
	char buf[16384];

	memset(&nladdr, 0, sizeof(nladdr));
	memset(&iov, 0, sizeof(iov));
	memset(&msg, 0, sizeof(msg));
	memset(&buf, 0, sizeof(buf));

	msg.msg_name = &nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	iov.iov_base = buf;
	while (1) {
		int status;
		struct nlmsghdr *h;

		iov.iov_len = sizeof(buf);
		status = recvmsg(fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			PyErr_SetString(PuExc_OSError, strerror(errno));
			return 0;
		}

		if (status == 0) {
			fprintf(stderr, "** ERROR ** EOF on netlink\n");
			return 0;
		}

		h = (struct nlmsghdr *)buf;
		while (NLMSG_OK(h, status)) {
			if (nladdr.nl_pid != 0 ||
			    h->nlmsg_pid != local->nl_pid ) {
				goto skip_data;
			}

			if (h->nlmsg_type == NLMSG_DONE) {
				return 1;
			}

			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);

				if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
					fprintf(stderr, "** ERROR ** Error message truncated\n");
				} else {
					errno = -err->error;
					PyErr_SetString(PuExc_OSError, strerror(errno));
				}
				return 0;
			}
			// Process/decode data
			if( !callback(h, ethinf, &process_ethinfo_idxptr) ) {
				fprintf(stderr, "** ERROR ** callback failed\n");
				return 0;
			}
		skip_data:
			h = NLMSG_NEXT(h, status);
		}

		if (msg.msg_flags & MSG_TRUNC) {
			fprintf(stderr, "** INFO ** Message truncated\n");
			continue;
		}
		if (status) {
			fprintf(stderr, "** WARNING ** Remnant of size %d\n", status);
			return 0;
		}
	}
}


/*
 *
 *    Internal functions for processing NETLINK_ROUTE results
 *
 */

/* ll_addr_n2a - stolen from iproute2 source */
const char *ll_addr_n2a(unsigned char *addr, int alen, int type, char *buf, int blen)
{
	int i;
	int l;

	if (alen == 4 &&
	    (type == ARPHRD_TUNNEL || type == ARPHRD_SIT || type == ARPHRD_IPGRE)) {
		return inet_ntop(AF_INET, addr, buf, blen);
	}
	if (alen == 16 && type == ARPHRD_TUNNEL6) {
		return inet_ntop(AF_INET6, addr, buf, blen);
	}
	l = 0;
	for (i=0; i<alen; i++) {
		if (i==0) {
			snprintf(buf+l, blen, "%02x", addr[i]);
			blen -= 2;
			l += 2;
		} else {
			snprintf(buf+l, blen, ":%02x", addr[i]);
			blen -= 3;
			l += 3;
		}
	}
	return buf;

}


// Callback function for processing RTM_GETLINK results
int etherinfo_proc_getlink(struct nlmsghdr *msg, struct etherinfo *ethinfchain, struct etherinfo **idxptr)
{
	int len = 0;
	struct ifinfomsg *ifinfo = NLMSG_DATA(msg);
	struct rtattr *rta = NULL;

        assert( ethinfchain != NULL );

	rta = IFLA_RTA(ifinfo);
	len = IFLA_PAYLOAD(msg);

        // Set the index pointer to the record we will update/register
        *idxptr = ((*idxptr) == NULL ? ethinfchain : (*idxptr)->next);
        if( (*idxptr)->next == NULL ) {
                // Append new record if we hit the end of the chain
                (*idxptr)->next = new_etherinfo_record();
		if( *idxptr == NULL ) {
			return 0;
		}
        }

        // Store information
        (*idxptr)->index = ifinfo->ifi_index;
        (*idxptr)->type = ifinfo->ifi_type;
	while( RTA_OK(rta, len) ) {
		switch( rta->rta_type ) {
		case IFLA_IFNAME:
                        (*idxptr)->device = strdup((char *)RTA_DATA(rta));
			break;

		case IFLA_ADDRESS:
                        (*idxptr)->hwaddress = (char *)malloc(258);
                        memset((*idxptr)->hwaddress, 0, 258);
                        ll_addr_n2a(RTA_DATA(rta), RTA_PAYLOAD(rta), ifinfo->ifi_type,
				    (*idxptr)->hwaddress, 256);
			break;
                default:
                        break;
		}

		rta = RTA_NEXT(rta, len);
	}
	return 1;
}

// Callback function for processing RTM_GETADDR results
int etherinfo_proc_getaddr(struct nlmsghdr *msg, struct etherinfo *ethinfchain, struct etherinfo **idxptr)
{
	int len = 0;
	struct ifaddrmsg *ifaddr = NLMSG_DATA(msg);
	struct rtattr *rta = NULL;
        char *ifa_addr = NULL, *ifa_brd = NULL;
        int ifa_netmask = 0;

        assert( ethinfchain != NULL );

	rta = IFA_RTA(ifaddr);
	len = IFA_PAYLOAD(msg);

        // Copy interesting information to our buffers
	while( RTA_OK(rta, len) ) {
		switch( rta->rta_type ) {
		case IFA_ADDRESS: // IP address + netmask
                        ifa_addr = (char *) malloc(130);
                        memset(ifa_addr, 0, 130);
			inet_ntop(ifaddr->ifa_family, RTA_DATA(rta), ifa_addr, 128);
                        ifa_netmask = ifaddr->ifa_prefixlen;
			break;

		case IFA_BROADCAST:
                        ifa_brd = (char *) malloc(130);
                        memset(ifa_brd, 0, 130);
			inet_ntop(ifaddr->ifa_family, RTA_DATA(rta), ifa_brd, 128);
			break;

		default:
                        break;
		}
		rta = RTA_NEXT(rta, len);
	}

        // Update the corresponding etherinfo record
	return update_etherinfo(ethinfchain, ifaddr->ifa_index, ifaddr->ifa_family,
				ifa_addr, ifa_netmask, ifa_brd);
}


/*
 *
 *   Exported functions - API frontend
 *
 */

void dump_etherinfo(FILE *fp, struct etherinfo *ethinfo)
{
        struct etherinfo *ptr;

        for( ptr = ethinfo; ptr->next != NULL; ptr = ptr->next ) {
                if( (ptr->type != ARPHRD_ETHER) && (ptr->type != ARPHRD_LOOPBACK) ) {
                        continue;
                }
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
}

struct etherinfo *get_etherinfo()
{
	int fd, result;
	struct sockaddr_nl local;
	struct etherinfo *ethinf = NULL;

	// open NETLINK socket
	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pad = 0;
	local.nl_pid = getpid();
	local.nl_groups = 0;
	fd = open_netlink_socket(&local);
	if( fd < 0 ) {
		return NULL;
	}

	// Create an empty record, where ethernet information will be saved
	ethinf = new_etherinfo_record();
	if( !ethinf ) {
		return NULL;
	}

        // Get some hardware info - ifname, type and hwaddress.  Populates ethinf
	if( !send_netlink_query(fd, GET_LINK) ) {
		result = 1;
		goto error;
	}
	if( !read_netlink_results(fd, &local, etherinfo_proc_getlink, ethinf) ) {
		result = 1;
		goto error;
	}


        // IPv4 information - updates the interfaces found in ethinfo
	if( !send_netlink_query(fd, GET_IPV4) ) {
		result = 1;
		goto error;
	}
	if( !read_netlink_results(fd, &local, etherinfo_proc_getaddr, ethinf) ) {
		result = 1;
		goto error;
	}


        // IPv6 information - updates the interfaces found in ethinfo
	if( !send_netlink_query(fd, GET_IPV6) ) {
		result = 1;
		goto error;
	}
	if( !read_netlink_results(fd, &local, etherinfo_proc_getaddr, ethinf) ) {
		result = 1;
		goto error;
	}

	result = 0;
	goto exit;

 error:
	free_etherinfo(ethinf);
	ethinf = NULL;

 exit:
	close(fd);
	return ethinf;
}

#ifdef TESTPROG
// Simple standalone test program
int main() {
	struct etherinfo *inf = NULL;

	inf = get_etherinfo();
	if( inf == NULL ) {
		fprintf(stderr, "Operation failed.  Could not retrieve ethernet information\n");
		exit(2);
	}
	dump_etherinfo(stdout, inf);
	free_etherinfo(inf);

	return 0;
}
#endif
