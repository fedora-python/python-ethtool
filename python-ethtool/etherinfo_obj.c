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
#include "etherinfo.h"
#include "etherinfo_struct.h"


/**
 * ethtool.etherinfo deallocator - cleans up when a object is deleted
 *
 * @param self etherinfo_py object structure
 */
void _ethtool_etherinfo_dealloc(etherinfo_py *self)
{
	if( self->info ) {
		free_etherinfo(self->info);
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
	if (self != NULL) {
	}
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
	self->info = (struct etherinfo *) malloc(sizeof(struct etherinfo)+2);
	memset(self->info, 0, sizeof(struct etherinfo)+2);
	self->info->device = strdup("test");
	return 0;
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

	if( !self || !self->info ) {
		PyErr_SetString(PyExc_AttributeError, "No data available");
		return NULL;
	}

	if( strcmp(attr, "device") == 0 ) {
		return PyString_FromString(self->info->device);
	} else if( strcmp(attr, "mac_address") == 0 ) {
		return PyString_FromString(self->info->hwaddress);
	} else if( strcmp(attr, "ipv4_address") == 0 ) {
		return PyString_FromString(self->info->ipv4_address);
	} else if( strcmp(attr, "ipv4_netmask") == 0 ) {
		return PyInt_FromLong(self->info->ipv4_netmask);
	} else if( strcmp(attr, "ipv4_broadcast") == 0 ) {
		return PyString_FromString(self->info->ipv4_broadcast);
	} else if( strcmp(attr, "ipv6_address") == 0 ) {
		return PyString_FromString(self->info->ipv6_address);
	} else if( strcmp(attr, "ipv6_netmask") == 0 ) {
		return PyInt_FromLong(self->info->ipv6_netmask);
	}
	PyErr_SetString(PyExc_AttributeError, "Unknown attribute name");
	return NULL;
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


