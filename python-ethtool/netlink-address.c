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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/* Python object corresponding to a (struct rtnl_addr) */
#include <Python.h>
#include "include/py3c/compat.h"
#include <bytesobject.h>
#include "structmember.h"

#include <arpa/inet.h>
#include <netlink/addr.h>
#include <netlink/route/addr.h>
#include <netlink/route/rtnl.h>
#include "etherinfo_struct.h"
#include "etherinfo.h"


/* IP Address parsing: */
static PyObject *
PyNetlinkIPaddress_from_rtnl_addr(struct rtnl_addr *addr)
{
    PyNetlinkIPaddress *py_obj;
    char buf[INET6_ADDRSTRLEN+1];
    struct nl_addr *peer_addr = NULL, *brdcst = NULL;

    py_obj = PyObject_New(PyNetlinkIPaddress,
                          &ethtool_netlink_ip_address_Type);
    if (!py_obj) {
        return NULL;
    }

    /* Set IP address family.  Only AF_INET and AF_INET6 is supported */
    py_obj->family = rtnl_addr_get_family(addr);
    if (py_obj->family != AF_INET && py_obj->family != AF_INET6) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Only IPv4 (AF_INET) and IPv6 (AF_INET6) "
                        "address types are supported");
        goto error;
    }

    /* Set local IP address: */
    memset(&buf, 0, sizeof(buf));
    if (!inet_ntop(py_obj->family,
                   nl_addr_get_binary_addr(rtnl_addr_get_local(addr)),
                   buf, sizeof(buf))) {
        PyErr_SetFromErrno(PyExc_RuntimeError);
        goto error;
    }
    py_obj->local = PyStr_FromString(buf);
    if (!py_obj->local) {
        goto error;
    }

    /* Set peer IP address: */
    memset(&buf, 0, sizeof(buf));
    if ((peer_addr = rtnl_addr_get_peer(addr))) {
        nl_addr2str(peer_addr, buf, sizeof(buf));
        py_obj->peer = PyStr_FromString(buf);
        if (!py_obj->local) {
            goto error;
        }
    } else {
        py_obj->peer = NULL;
    }

    /* Set IP address prefix length (netmask): */
    py_obj->prefixlen = rtnl_addr_get_prefixlen(addr);

    /* Set ipv4_broadcast: */
    py_obj->ipv4_broadcast = NULL;
    brdcst = rtnl_addr_get_broadcast(addr);
    if (py_obj->family == AF_INET && brdcst) {
        memset(&buf, 0, sizeof(buf));
        if (!inet_ntop(AF_INET, nl_addr_get_binary_addr(brdcst),
                       buf, sizeof(buf))) {
            PyErr_SetFromErrno(PyExc_RuntimeError);
            goto error;
        }
        py_obj->ipv4_broadcast = PyStr_FromString(buf);
        if (!py_obj->ipv4_broadcast) {
            goto error;
        }
    }

    /* Set IP address scope: */
    memset(&buf, 0, sizeof(buf));
    rtnl_scope2str(rtnl_addr_get_scope(addr), buf, sizeof(buf));
    py_obj->scope = PyStr_FromString(buf);

    return (PyObject*)py_obj;

error:
    Py_DECREF(py_obj);
    return NULL;
}

static void
netlink_ip_address_dealloc(PyNetlinkIPaddress *obj)
{
    Py_DECREF(obj->local);
    Py_XDECREF(obj->peer);
    Py_XDECREF(obj->ipv4_broadcast);
    Py_XDECREF(obj->scope);

    /* We can call PyObject_Del directly rather than calling through
       tp_free since the type is not subtypable (Py_TPFLAGS_BASETYPE is
       not set): */
    PyObject_Del(obj);
}

static PyObject*
netlink_ip_address_repr(PyNetlinkIPaddress *obj)
{
    PyObject *result = PyStr_FromString("ethtool.NetlinkIPaddress(family=");
    char buf[256];

    memset(&buf, 0, sizeof(buf));
    nl_af2str(obj->family, buf, sizeof(buf));
    result = PyStr_Concat(result,
                          PyStr_FromFormat("%s, address='%s",
                                           buf,
                                           PyStr_AsString(obj->local)));

    if (obj->family == AF_INET) {
        result = PyStr_Concat(result,
                              PyStr_FromFormat("', netmask=%d",
                                               obj->prefixlen));
    } else if (obj->family == AF_INET6) {
        result = PyStr_Concat(result,
                              PyStr_FromFormat("/%d'", obj->prefixlen));
    }

    if (obj->peer) {
        result = PyStr_Concat(result,
                              PyStr_FromFormat(", peer_address='%s'",
                                               PyStr_AsString(obj->peer)));
    }

    if (obj->family == AF_INET && obj->ipv4_broadcast) {
        result = PyStr_Concat(result,
                              PyStr_FromFormat(", broadcast='%s'",
                                               PyStr_AsString(
                                                   obj->ipv4_broadcast)));
    }

    result = PyStr_Concat(result,
                          PyStr_FromFormat(", scope=%s)",
                                           PyStr_AsString(obj->scope)));

    return result;
}


static PyMemberDef _ethtool_netlink_ip_address_members[] = {
    {   "address",
        T_OBJECT_EX,
        offsetof(PyNetlinkIPaddress, local),
        0,
        NULL
    },
    {   "peer_address",
        T_OBJECT_EX,
        offsetof(PyNetlinkIPaddress, peer),
        0,
        NULL
    },
    {   "netmask",
        T_INT,
        offsetof(PyNetlinkIPaddress, prefixlen),
        0,
        NULL
    },
    {   "broadcast",
        T_OBJECT, /* can be NULL */
        offsetof(PyNetlinkIPaddress, ipv4_broadcast),
        0,
        NULL
    },
    {   "scope",
        T_OBJECT_EX,
        offsetof(PyNetlinkIPaddress, scope),
        0,
        NULL
    },
    {NULL}  /* End of member list */
};

PyTypeObject ethtool_netlink_ip_address_Type = {
    PyVarObject_HEAD_INIT(0, 0)
    .tp_name = "ethtool.NetlinkIPaddress",
    .tp_basicsize = sizeof(PyNetlinkIPaddress),
    .tp_dealloc = (destructor)netlink_ip_address_dealloc,
    .tp_repr = (reprfunc)netlink_ip_address_repr,
    .tp_members = _ethtool_netlink_ip_address_members,
};


PyObject *
make_python_address_from_rtnl_addr(struct rtnl_addr *addr)
{
    assert(addr);

    switch( rtnl_addr_get_family(addr)) {

    case AF_INET:
    case AF_INET6:
        return PyNetlinkIPaddress_from_rtnl_addr(addr);

    default:
        return PyErr_SetFromErrno(PyExc_RuntimeError);
    }
}
