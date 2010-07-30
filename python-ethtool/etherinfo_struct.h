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
	struct ipv6address *ipv6_addresses;
};

struct ipv6address {
	char *address;
	int netmask;
	int scope;
	struct ipv6address *next;
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


typedef struct {
	PyObject_HEAD
	struct ipv6address *addrdata;
} etherinfo_ipv6_py;

/**
 * NULL safe PyString_FromString() wrapper.  If input string is NULL, None will be returned
 *
 * @param str Input C string (char *)
 *
 * @return Returns a PyObject with either the input string wrapped up, or a Python None value.
 */
#define RETURN_STRING(str) (str ? PyString_FromString(str) : Py_None)

#endif
