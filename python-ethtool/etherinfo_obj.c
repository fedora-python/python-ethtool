/*
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


/**
 * @file   etherinfo_obj.c
 * @author David Sommerseth <davids@redhat.com>
 * @date   Fri Sep  4 18:41:28 2009
 *
 * @brief  Python ethtool.etherinfo class functions.
 *
 */

#include <Python.h>
#include "include/py3c/compat.h"
#include <bytesobject.h>
#include "structmember.h"

#include <netlink/route/rtnl.h>
#include <netlink/route/addr.h>
#include "etherinfo_struct.h"
#include "etherinfo.h"

/**
 * ethtool.etherinfo deallocator - cleans up when a object is deleted
 *
 * @param self PyEtherInfo Python object to deallocate
 */
static void _ethtool_etherinfo_dealloc(PyEtherInfo *self)
{
    close_netlink(self);
    Py_XDECREF(self->device);
    self->device = NULL;
    Py_XDECREF(self->hwaddress);
    self->hwaddress = NULL;
    Py_TYPE(self)->tp_free((PyObject*)self);
}


/*
  The old approach of having a single IPv4 address per device meant each result
  that came in from netlink overwrote the old result.

  Mimic it by returning the last entry in the list (if any).

  The return value is a *borrowed reference* (or NULL)
*/
static PyNetlinkIPaddress * get_last_ipv4_address(PyObject *addrlist)
{
    Py_ssize_t size;

    if (!addrlist) {
        return NULL;
    }

    if (!PyList_Check(addrlist)) {
        return NULL;
    }

    size = PyList_Size(addrlist);
    if (size > 0) {
        PyNetlinkIPaddress *item = (PyNetlinkIPaddress *)
             PyList_GetItem(addrlist, size - 1);
        if (Py_TYPE(item) == &ethtool_netlink_ip_address_Type) {
            return item;
        }
    }

    return NULL;
}

/**
 * Creates a human readable format of the information when object is being
 * treated as a string
 *
 * @param self  Pointer to the current PyEtherInfo device object
 *
 * @return Returns a PyObject with a string with all of the information
 */
PyObject *_ethtool_etherinfo_str(PyEtherInfo *self)
{
    PyObject *ret = NULL;
    PyObject *ipv4addrs = NULL, *ipv6addrs = NULL;

    if (!self) {
        PyErr_SetString(PyExc_AttributeError, "No data available");
        return NULL;
    }

    get_etherinfo_link(self);

    ret = PyStr_FromFormat("Device %s:\n", PyStr_AsString(self->device));

    if (self->hwaddress) {
        ret = PyStr_Concat(ret,
                           PyStr_FromFormat("\tMAC address: %s\n",
                                            PyStr_AsString(self->hwaddress)));
    }

    ipv4addrs = get_etherinfo_address(self, NLQRY_ADDR4);
    if (ipv4addrs) {
        Py_ssize_t i;
        for (i = 0; i < PyList_Size(ipv4addrs); i++) {
            PyNetlinkIPaddress *py_addr = (PyNetlinkIPaddress *)
                PyList_GetItem(ipv4addrs, i);
            PyObject *tmp = PyStr_FromFormat("\tIPv4 address: ");
            tmp = PyStr_Concat(tmp, py_addr->local);
            tmp = PyStr_Concat(tmp,
                               PyStr_FromFormat("/%d", py_addr->prefixlen));
            if (py_addr->ipv4_broadcast) {
                tmp = PyStr_Concat(
                    tmp, PyStr_FromFormat("\tBroadcast: %s\n",
                                          PyStr_AsString(
                                            py_addr->ipv4_broadcast)));
            } else {
                tmp = PyStr_Concat(tmp, PyStr_FromFormat("\n"));
            }

            ret = PyStr_Concat(ret, tmp);
        }
    }

    ipv6addrs = get_etherinfo_address(self, NLQRY_ADDR6);
    if (ipv6addrs) {
        Py_ssize_t i;
        for (i = 0; i < PyList_Size(ipv6addrs); i++) {
            PyNetlinkIPaddress *py_addr = (PyNetlinkIPaddress *)
                PyList_GetItem(ipv6addrs, i);
            PyObject *tmp = PyStr_FromFormat("\tIPv6 address: [%s] %s/%d\n",
                                             PyStr_AsString(py_addr->scope),
                                             PyStr_AsString(py_addr->local),
                                             py_addr->prefixlen);
            ret = PyStr_Concat(ret, tmp);
        }
    }

    return ret;
}

/**
 * Returns a tuple list of configured IPv4 addresses
 *
 * @param self     Pointer to the current PyEtherInfo device object to extract
 *                 IPv4 info from
 * @param notused
 *
 * @return Returns a Python tuple list of NetlinkIP4Address objects
 */
static PyObject *_ethtool_etherinfo_get_ipv4_addresses(PyEtherInfo *self,
                                                       PyObject *notused) {
    if (!self) {
        PyErr_SetString(PyExc_AttributeError, "No data available");
        return NULL;
    }

    return get_etherinfo_address(self, NLQRY_ADDR4);
}


/**
 * Returns a tuple list of configured IPv6 addresses
 *
 * @param self     Pointer to the current PyEtherInfo device object to extract
 *                 IPv6 info from
 * @param notused
 *
 * @return Returns a Python tuple list of NetlinkIP6Address objects
 */
static PyObject *_ethtool_etherinfo_get_ipv6_addresses(PyEtherInfo *self,
                                                       PyObject *notused) {
    if (!self) {
        PyErr_SetString(PyExc_AttributeError, "No data available");
        return NULL;
    }

    return get_etherinfo_address(self, NLQRY_ADDR6);
}


/**
 * Defines all available methods in the ethtool.etherinfo class
 *
 */
static PyMethodDef _ethtool_etherinfo_methods[] = {
    {   "get_ipv4_addresses",
        (PyCFunction)_ethtool_etherinfo_get_ipv4_addresses, METH_NOARGS,
        "Retrieve configured IPv4 addresses.  "
        "Returns a list of NetlinkIPaddress objects"
    },
    {   "get_ipv6_addresses",
        (PyCFunction)_ethtool_etherinfo_get_ipv6_addresses, METH_NOARGS,
        "Retrieve configured IPv6 addresses.  "
        "Returns a list of NetlinkIPaddress objects"
    },
    {NULL}  /**< No methods defined */
};

static PyObject *get_device(PyObject *obj, void *info)
{
    PyEtherInfo *self = (PyEtherInfo *) obj;

    if (self->device) {
        Py_INCREF(self->device);
        return self->device;
    }
    Py_RETURN_NONE;
}

static PyObject *get_mac_addr(PyObject *obj, void *info)
{
    PyEtherInfo *self = (PyEtherInfo *) obj;

    get_etherinfo_link(self);
    if (self->hwaddress) {
        Py_INCREF(self->hwaddress);
    }
    return self->hwaddress;
}

static PyObject *get_ipv4_addr(PyObject *obj, void *info)
{
    PyEtherInfo *self = (PyEtherInfo *) obj;
    PyObject *addrlist;
    PyNetlinkIPaddress *py_addr;

    addrlist = get_etherinfo_address(self, NLQRY_ADDR4);
    /* For compatiblity with old approach, return last IPv4 address: */
    py_addr = get_last_ipv4_address(addrlist);
    if (py_addr) {
        if (py_addr->local) {
            Py_INCREF(py_addr->local);
            return py_addr->local;
        }
    }

    if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject *get_ipv4_mask(PyObject *obj, void *info)
{
    PyEtherInfo *self = (PyEtherInfo *) obj;
    PyObject *addrlist;
    PyNetlinkIPaddress *py_addr;

    addrlist = get_etherinfo_address(self, NLQRY_ADDR4);
    py_addr = get_last_ipv4_address(addrlist);
    if (py_addr) {
        return PyInt_FromLong(py_addr->prefixlen);
    }

    if (PyErr_Occurred()) {
        return NULL;
    } else {
        return PyInt_FromLong(0);
    }
}

static PyObject *get_ipv4_bcast(PyObject *obj, void *info)
{
    PyEtherInfo *self = (PyEtherInfo *) obj;
    PyObject *addrlist;
    PyNetlinkIPaddress *py_addr;

    addrlist = get_etherinfo_address(self, NLQRY_ADDR4);
    py_addr = get_last_ipv4_address(addrlist);
    if (py_addr) {
        if (py_addr->ipv4_broadcast) {
            Py_INCREF(py_addr->ipv4_broadcast);
            return py_addr->ipv4_broadcast;
        }
    }

    if (PyErr_Occurred()) {
        return NULL;
    } else {
        return PyStr_FromString("0.0.0.0");
    }
}


static PyGetSetDef _ethtool_etherinfo_attributes[] = {
    {"device", get_device, NULL, "device", NULL},
    {"mac_address", get_mac_addr, NULL, "MAC address", NULL},
    {"ipv4_address", get_ipv4_addr, NULL, "IPv4 address", NULL},
    {"ipv4_netmask", get_ipv4_mask, NULL, "IPv4 netmask", NULL},
    {"ipv4_broadcast", get_ipv4_bcast, NULL, "IPv4 broadcast", NULL},
    {NULL},
};


/**
 * Definition of the functions a Python class/object requires.
 *
 */
PyTypeObject PyEtherInfo_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ethtool.etherinfo",
    .tp_basicsize = sizeof(PyEtherInfo),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)_ethtool_etherinfo_dealloc,
    .tp_str = (reprfunc)_ethtool_etherinfo_str,
    .tp_getset = _ethtool_etherinfo_attributes,
    .tp_methods = _ethtool_etherinfo_methods,
    .tp_doc = "Contains information about a specific ethernet device"
};
