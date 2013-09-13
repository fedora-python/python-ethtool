/*
 * Copyright (C) 2011 - 2013 Red Hat Inc.
 *
 * David Malcolm <dmalcolm@redhat.com>
 *
 * This application is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2.
 *
 * This application is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 */

/* Python object corresponding to a (struct rtnl_addr) */
#include <Python.h>
#include "structmember.h"

#include <arpa/inet.h>
#include <netlink/addr.h>
#include <netlink/route/addr.h>
#include <netlink/route/rtnl.h>
#include "etherinfo_struct.h"
#include "etherinfo.h"

/* IPv6 Addresses: */
static PyObject *
PyNetlinkIPv6Address_from_rtnl_addr(struct rtnl_addr *addr)
{
	PyNetlinkIPv6Address *py_obj;
	char buf[INET6_ADDRSTRLEN+1];

	py_obj = PyObject_New(PyNetlinkIPv6Address,
			      &ethtool_netlink_ipv6_address_Type);
	if (!py_obj) {
		return NULL;
	}

	/* Set ipv6_address: */
	memset(&buf, 0, sizeof(buf));
	if (!inet_ntop(AF_INET6, nl_addr_get_binary_addr(rtnl_addr_get_local(addr)),
		       buf, sizeof(buf))) {
		PyErr_SetFromErrno(PyExc_RuntimeError);
		goto error;
	}
	py_obj->ipv6_address = PyString_FromString(buf);
	if (!py_obj->ipv6_address) {
		goto error;
	}

	/* Set ipv6_netmask: */
	py_obj->ipv6_netmask = rtnl_addr_get_prefixlen(addr);


	/* Set ipv6_scope: */
	memset(&buf, 0, sizeof(buf));
	rtnl_scope2str(rtnl_addr_get_scope(addr), buf, sizeof(buf));
	py_obj->ipv6_scope = PyString_FromString(buf);

	return (PyObject*)py_obj;

 error:
	Py_DECREF(py_obj);
	return NULL;
}

static void
netlink_ipv6_address_dealloc(PyNetlinkIPv6Address *obj)
{
	Py_DECREF(obj->ipv6_address);
	Py_DECREF(obj->ipv6_scope);

	/* We can call PyObject_Del directly rather than calling through
	   tp_free since the type is not subtypable (Py_TPFLAGS_BASETYPE is
	   not set): */
	PyObject_Del(obj);
}

static PyObject*
netlink_ipv6_address_repr(PyNetlinkIPv6Address *obj)
{
	PyObject *result = PyString_FromString("ethtool.NetlinkIPv6Address(address='");
	PyString_Concat(&result, obj->ipv6_address);
	PyString_ConcatAndDel(&result,
			      PyString_FromFormat("/%d', scope=",
						  obj->ipv6_netmask));
	PyString_Concat(&result, obj->ipv6_scope);
	PyString_ConcatAndDel(&result, PyString_FromString(")"));
	return result;
}

static PyMemberDef _ethtool_netlink_ipv6_address_members[] = {
	{"address",
	 T_OBJECT_EX,
	 offsetof(PyNetlinkIPv6Address, ipv6_address),
	 0,
	 NULL},
	{"netmask",
	 T_INT,
	 offsetof(PyNetlinkIPv6Address, ipv6_netmask),
	 0,
	 NULL},
	{"scope",
	 T_OBJECT_EX,
	 offsetof(PyNetlinkIPv6Address, ipv6_scope),
	 0,
	 NULL},
	{NULL}  /* End of member list */
};

PyTypeObject ethtool_netlink_ipv6_address_Type = {
	PyVarObject_HEAD_INIT(0, 0)
	.tp_name = "ethtool.NetlinkIPv6Address",
	.tp_basicsize = sizeof(PyNetlinkIPv6Address),
	.tp_dealloc = (destructor)netlink_ipv6_address_dealloc,
	.tp_repr = (reprfunc)netlink_ipv6_address_repr,
	.tp_members = _ethtool_netlink_ipv6_address_members,
};



/* IPv4 Addresses: */
static PyObject *
PyNetlinkIPv4Address_from_rtnl_addr(struct rtnl_addr *addr)
{
	PyNetlinkIPv4Address *py_obj;
	char buf[INET_ADDRSTRLEN+1];
	struct nl_addr *brdcst;

	py_obj = PyObject_New(PyNetlinkIPv4Address,
			      &ethtool_netlink_ipv4_address_Type);
	if (!py_obj) {
		return NULL;
	}

	/* Set ipv4_address: */
	memset(&buf, 0, sizeof(buf));
	if (!inet_ntop(AF_INET, nl_addr_get_binary_addr(rtnl_addr_get_local(addr)),
		       buf, sizeof(buf))) {
		PyErr_SetFromErrno(PyExc_RuntimeError);
		goto error;
	}
	py_obj->ipv4_address = PyString_FromString(buf);
	if (!py_obj->ipv4_address) {
		goto error;
	}

	/* Set ipv4_netmask: */
	py_obj->ipv4_netmask = rtnl_addr_get_prefixlen(addr);

	/* Set ipv4_broadcast: */
	py_obj->ipv4_broadcast = NULL;
	brdcst = rtnl_addr_get_broadcast(addr);
	if( brdcst ) {
		memset(&buf, 0, sizeof(buf));
		if (!inet_ntop(AF_INET, nl_addr_get_binary_addr(brdcst),
			       buf, sizeof(buf))) {
			PyErr_SetFromErrno(PyExc_RuntimeError);
			goto error;
		}
		py_obj->ipv4_broadcast = PyString_FromString(buf);
		if (!py_obj->ipv4_broadcast) {
			goto error;
		}
	}

	return (PyObject*)py_obj;

 error:
	Py_DECREF(py_obj);
	return NULL;
}

static void
netlink_ipv4_address_dealloc(PyNetlinkIPv4Address *obj)
{
	Py_DECREF(obj->ipv4_address);
	Py_XDECREF(obj->ipv4_broadcast);

	/* We can call PyObject_Del directly rather than calling through
	   tp_free since the type is not subtypable (Py_TPFLAGS_BASETYPE is
	   not set): */
	PyObject_Del(obj);
}

static PyObject*
netlink_ipv4_address_repr(PyNetlinkIPv4Address *obj)
{
	PyObject *result = PyString_FromString("ethtool.NetlinkIPv4Address(address='");
	PyString_Concat(&result, obj->ipv4_address);
	PyString_ConcatAndDel(&result,
			      PyString_FromFormat("', netmask=%d",
						  obj->ipv4_netmask));
	if (obj->ipv4_broadcast) {
		PyString_ConcatAndDel(&result, PyString_FromString(", broadcast='"));
		PyString_Concat(&result, obj->ipv4_broadcast);
		PyString_ConcatAndDel(&result, PyString_FromString("'"));
	}
	PyString_ConcatAndDel(&result, PyString_FromString(")"));
	return result;
}

static PyMemberDef _ethtool_netlink_ipv4_address_members[] = {
	{"address",
	 T_OBJECT_EX,
	 offsetof(PyNetlinkIPv4Address, ipv4_address),
	 0,
	 NULL},
	{"netmask",
	 T_INT,
	 offsetof(PyNetlinkIPv4Address, ipv4_netmask),
	 0,
	 NULL},
	{"broadcast",
	 T_OBJECT, /* can be NULL */
	 offsetof(PyNetlinkIPv4Address, ipv4_broadcast),
	 0,
	 NULL},
	{NULL}  /* End of member list */
};

PyTypeObject ethtool_netlink_ipv4_address_Type = {
	PyVarObject_HEAD_INIT(0, 0)
	.tp_name = "ethtool.NetlinkIPv4Address",
	.tp_basicsize = sizeof(PyNetlinkIPv4Address),
	.tp_dealloc = (destructor)netlink_ipv4_address_dealloc,
	.tp_repr = (reprfunc)netlink_ipv4_address_repr,
	.tp_members = _ethtool_netlink_ipv4_address_members,
};

/* Factory function, in case we want to generalize this to add IPv6 support */
PyObject *
make_python_address_from_rtnl_addr(struct rtnl_addr *addr)
{
	assert(addr);

	switch( rtnl_addr_get_family(addr) ) {

	case AF_INET:
		return PyNetlinkIPv4Address_from_rtnl_addr(addr);

	case AF_INET6:
		return PyNetlinkIPv6Address_from_rtnl_addr(addr);

	default:
		return PyErr_SetFromErrno(PyExc_RuntimeError);
	}
}

/*
Local variables:
c-basic-offset: 8
indent-tabs-mode: y
End:
*/
