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


struct etherinfo {
	char *device;
	int index;
	char *hwaddress;
	char *ipv4_address;
	int ipv4_netmask;
	char *ipv4_broadcast;
	char *ipv6_address;
	int ipv6_netmask;
};

/*
 *  NETLINK connection handle and related information to be shared
 *  among all the instantiated etherinfo objects.
 */
struct _nlconnection {
	struct nl_handle *nlrt_handle;
};

/**
 * Contains the internal data structure of the
 * ethtool.etherinfo object.
 *
 */
struct etherinfo_obj_data {
	struct _nlconnection *nlc;	/**< Contains NETLINK connection info */
	struct etherinfo *ethinfo;      /**< Contains info about our current interface */
};

typedef struct {
	PyObject_HEAD
	struct etherinfo_obj_data *data;
} etherinfo_py;

#endif
