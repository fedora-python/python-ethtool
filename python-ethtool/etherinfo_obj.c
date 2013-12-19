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
#include "structmember.h"

#include <netlink/route/rtnl.h>
#include <netlink/route/addr.h>
#include "etherinfo_struct.h"
#include "etherinfo.h"

extern PyTypeObject ethtool_etherinfoIPv6Type;

/**
 * ethtool.etherinfo deallocator - cleans up when a object is deleted
 *
 * @param self etherinfo_py object structure
 */
void _ethtool_etherinfo_dealloc(etherinfo_py *self)
{
	close_netlink(self);
	if( self->ethinfo ) {
                free_etherinfo(self->ethinfo);
	}
	self->ob_type->tp_free((PyObject*)self);
}


/**
 * ethtool.etherinfo function, creating a new etherinfo object
 *
 * @param type
 * @param args
 * @param kwds
 *
 * @return Returns in PyObject with the new object on success, otherwise NULL
 */
PyObject *_ethtool_etherinfo_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	etherinfo_py *self;

	self = (etherinfo_py *)type->tp_alloc(type, 0);
	return (PyObject *)self;
}


/**
 * ethtool.etherinfo init (constructor) method.  Makes sure the object is initialised correctly.
 *
 * @param self
 * @param args
 * @param kwds
 *
 * @return Returns 0 on success.
 */
int _ethtool_etherinfo_init(etherinfo_py *self, PyObject *args, PyObject *kwds)
{
	static char *etherinfo_kwlist[] = {"etherinfo_ptr", NULL};
	PyObject *ethinf_ptr = NULL;

	if( !PyArg_ParseTupleAndKeywords(args, kwds, "O", etherinfo_kwlist, &ethinf_ptr)) {
		PyErr_SetString(PyExc_AttributeError, "Invalid data pointer to constructor");
		return -1;
	}
	self->ethinfo = (struct etherinfo *) PyCObject_AsVoidPtr(ethinf_ptr);
	return 0;
}

/*
  The old approach of having a single IPv4 address per device meant each result
  that came in from netlink overwrote the old result.

  Mimic it by returning the last entry in the list (if any).

  The return value is a *borrowed reference* (or NULL)
*/
static PyNetlinkIPaddress * get_last_ipv4_address(etherinfo_py *self)
{
	Py_ssize_t size;
	PyObject *list;

	assert(self);
	list = self->ethinfo->ipv4_addresses;
	if (!list) {
		return NULL;
	}

	if (!PyList_Check(list)) {
		return NULL;
	}

	size = PyList_Size(list);
	if (size > 0) {
		PyObject *item = PyList_GetItem(list, size - 1);
		if (Py_TYPE(item) == &ethtool_netlink_ip_address_Type) {
			return (PyNetlinkIPaddress*)item;
		}
	}

	return NULL;
}

/**
 * ethtool.etherinfo function for retrieving data from a Python object.
 *
 * @param self
 * @param attr_o  contains the object member request (which element to return)
 *
 * @return Returns a PyObject with the value requested on success, otherwise NULL
 */
PyObject *_ethtool_etherinfo_getter(etherinfo_py *self, PyObject *attr_o)
{
	char *attr = PyString_AsString(attr_o);
	PyNetlinkIPaddress *py_addr;

	if( !self || !self->ethinfo ) {
		PyErr_SetString(PyExc_AttributeError, "No data available");
		return NULL;
	}

	if( strcmp(attr, "device") == 0 ) {
                if( self->ethinfo->device ) {
                        return PyString_FromString(self->ethinfo->device);
                } else {
                        return Py_INCREF(Py_None), Py_None;
                }
	} else if( strcmp(attr, "mac_address") == 0 ) {
		get_etherinfo_link(self);
		Py_INCREF(self->ethinfo->hwaddress);
		return self->ethinfo->hwaddress;
	} else if( strcmp(attr, "ipv4_address") == 0 ) {
		get_etherinfo(self, NLQRY_ADDR4);
		/* For compatiblity with old approach, return last IPv4 address: */
		py_addr = get_last_ipv4_address(self);
		if (py_addr) {
		  if (py_addr->local) {
		      Py_INCREF(py_addr->local);
		      return py_addr->local;
		  }
		}
		Py_RETURN_NONE;
	} else if( strcmp(attr, "ipv4_netmask") == 0 ) {
		get_etherinfo(self, NLQRY_ADDR4);
		py_addr = get_last_ipv4_address(self);
		if (py_addr) {
		  return PyInt_FromLong(py_addr->prefixlen);
		}
		return PyInt_FromLong(0);
	} else if( strcmp(attr, "ipv4_broadcast") == 0 ) {
		get_etherinfo(self, NLQRY_ADDR4);
		py_addr = get_last_ipv4_address(self);
		if (py_addr) {
		  if (py_addr->ipv4_broadcast) {
		      Py_INCREF(py_addr->ipv4_broadcast);
		      return py_addr->ipv4_broadcast;
		  }
		}
		Py_RETURN_NONE;
	} else {
		return PyObject_GenericGetAttr((PyObject *)self, attr_o);
	}
}

/**
 * ethtool.etherinfo function for setting a value to a object member.  This feature is
 * disabled by always returning -1, as the values are read-only by the user.
 *
 * @param self
 * @param attr_o
 * @param val_o
 *
 * @return Returns always -1 (failure).
 */
int _ethtool_etherinfo_setter(etherinfo_py *self, PyObject *attr_o, PyObject *val_o)
{
	PyErr_SetString(PyExc_AttributeError, "etherinfo member values are read-only.");
	return -1;
}


/**
 * Creates a human readable format of the information when object is being treated as a string
 *
 * @param self
 *
 * @return Returns a PyObject with a string with all of the information
 */
PyObject *_ethtool_etherinfo_str(etherinfo_py *self)
{
	PyObject *ret = NULL;

	if( !self || !self->ethinfo ) {
		PyErr_SetString(PyExc_AttributeError, "No data available");
		return NULL;
	}

	get_etherinfo_link(self);
	get_etherinfo(self, NLQRY_ADDR4);
	get_etherinfo(self, NLQRY_ADDR6);

	ret = PyString_FromFormat("Device %s:\n", self->ethinfo->device);
	if( self->ethinfo->hwaddress ) {
		PyString_ConcatAndDel(&ret, PyString_FromString("\tMAC address: "));
		PyString_Concat(&ret, self->ethinfo->hwaddress);
		PyString_ConcatAndDel(&ret, PyString_FromString("\n"));
	}

	if( self->ethinfo->ipv4_addresses ) {
               Py_ssize_t i;
               for (i = 0; i < PyList_Size(self->ethinfo->ipv4_addresses); i++) {
                       PyNetlinkIPaddress *py_addr = (PyNetlinkIPaddress *)PyList_GetItem(self->ethinfo->ipv4_addresses, i);
                       PyObject *tmp = PyString_FromFormat("\tIPv4 address: ");
                       PyString_Concat(&tmp, py_addr->local);
                       PyString_ConcatAndDel(&tmp, PyString_FromFormat("/%d", py_addr->prefixlen));
                       if (py_addr->ipv4_broadcast ) {
                                PyString_ConcatAndDel(&tmp,
                                                      PyString_FromString("	  Broadcast: "));
                                PyString_Concat(&tmp, py_addr->ipv4_broadcast);
                       }
                       PyString_ConcatAndDel(&tmp, PyString_FromString("\n"));
                       PyString_ConcatAndDel(&ret, tmp);
               }
	}

	if( self->ethinfo->ipv6_addresses ) {
	       Py_ssize_t i;
	       for (i = 0; i < PyList_Size(self->ethinfo->ipv6_addresses); i++) {
		       PyNetlinkIPaddress *py_addr = (PyNetlinkIPaddress *)PyList_GetItem(self->ethinfo->ipv6_addresses, i);
		       PyObject *tmp = PyString_FromFormat("\tIPv6 address: [");
		       PyString_Concat(&tmp, py_addr->scope);
		       PyString_ConcatAndDel(&tmp, PyString_FromString("] "));
		       PyString_Concat(&tmp, py_addr->local);
		       PyString_ConcatAndDel(&tmp, PyString_FromFormat("/%d", py_addr->prefixlen));
		       PyString_ConcatAndDel(&tmp, PyString_FromString("\n"));
		       PyString_ConcatAndDel(&ret, tmp);
	       }
	}

	return ret;
}

/**
 * Returns a tuple list of configured IPv4 addresses
 *
 * @param self
 * @param notused
 *
 * @return Returns a Python tuple list of NetlinkIP4Address objects
 */
static PyObject *_ethtool_etherinfo_get_ipv4_addresses(etherinfo_py *self, PyObject *notused) {
	PyObject *ret;

	if( !self || !self->ethinfo ) {
		PyErr_SetString(PyExc_AttributeError, "No data available");
		return NULL;
	}

	get_etherinfo(self, NLQRY_ADDR4);

	/* Transfer ownership of reference: */
	ret = self->ethinfo->ipv4_addresses;
	self->ethinfo->ipv4_addresses = NULL;

	return ret;
}


/**
 * Returns a tuple list of configured IPv4 addresses
 *
 * @param self
 * @param notused
 *
 * @return Returns a Python tuple list of NetlinkIP6Address objects
 */
static PyObject *_ethtool_etherinfo_get_ipv6_addresses(etherinfo_py *self, PyObject *notused) {
	PyObject *ret;

	if( !self || !self->ethinfo ) {
		PyErr_SetString(PyExc_AttributeError, "No data available");
		return NULL;
	}

	get_etherinfo(self, NLQRY_ADDR6);

	/* Transfer ownership of reference: */
	ret = self->ethinfo->ipv6_addresses;
	self->ethinfo->ipv6_addresses = NULL;

	return ret;
}


/**
 * Defines all available methods in the ethtool.etherinfo class
 *
 */
static PyMethodDef _ethtool_etherinfo_methods[] = {
	{"get_ipv4_addresses", (PyCFunction)_ethtool_etherinfo_get_ipv4_addresses, METH_NOARGS,
	 "Retrieve configured IPv4 addresses.  Returns a list of NetlinkIPaddress objects"},
	{"get_ipv6_addresses", (PyCFunction)_ethtool_etherinfo_get_ipv6_addresses, METH_NOARGS,
	 "Retrieve configured IPv6 addresses.  Returns a list of NetlinkIPaddress objects"},
	{NULL}  /**< No methods defined */
};

/**
 * Definition of the functions a Python class/object requires.
 *
 */
PyTypeObject ethtool_etherinfoType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "ethtool.etherinfo",       /*tp_name*/
    sizeof(etherinfo_py),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)_ethtool_etherinfo_dealloc,/*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc)_ethtool_etherinfo_str,        /*tp_str*/
    (getattrofunc)_ethtool_etherinfo_getter, /*tp_getattro*/
    (setattrofunc)_ethtool_etherinfo_setter, /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Contains information about a specific ethernet device", /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    _ethtool_etherinfo_methods,            /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)_ethtool_etherinfo_init,     /* tp_init */
    0,                         /* tp_alloc */
    _ethtool_etherinfo_new,                /* tp_new */
};

