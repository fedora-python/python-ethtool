/* netlink.c - Generic NETLINK API functions
 *
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

#include <Python.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>

#include "etherinfo_struct.h"

pthread_mutex_t nlc_counter_mtx = PTHREAD_MUTEX_INITIALIZER;


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

/*
Local variables:
c-basic-offset: 8
indent-tabs-mode: y
End:
*/
