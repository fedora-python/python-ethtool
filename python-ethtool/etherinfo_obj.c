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

#include "etherinfo_struct.h"
#include "etherinfo.h"


/**
 * ethtool.etherinfo deallocator - cleans up when a object is deleted
 *
 * @param self etherinfo_py object structure
 */
void _ethtool_etherinfo_dealloc(etherinfo_py *self)
{
	if( self->data ) {
		if( self->data->ethinfo ) {
			free_etherinfo(self->data->ethinfo);
		}
		free(self->data);
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
	self->data = (struct etherinfo_obj_data *) PyCObject_AsVoidPtr(ethinf_ptr);
	return 0;
}

/**
 * NULL safe PyString_FromString() wrapper.  If input string is NULL, None will be returned
 *
 * @param str Input C string (char *)
 *
 * @return Returns a PyObject with either the input string wrapped up, or a Python None value.
 */
#define RETURN_STRING(str) (str ? PyString_FromString(str) : Py_None);

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
	PyObject *ret;
	char *attr = PyString_AsString(attr_o);

	if( !self || !self->data ) {
		PyErr_SetString(PyExc_AttributeError, "No data available");
		return NULL;
	}

	if( strcmp(attr, "device") == 0 ) {
		ret = RETURN_STRING(self->data->ethinfo->device);
	} else if( strcmp(attr, "mac_address") == 0 ) {
		get_etherinfo(self->data->ethinfo, self->data->nlc, NLQRY_LINK);
		ret = RETURN_STRING(self->data->ethinfo->hwaddress);
	} else if( strcmp(attr, "ipv4_address") == 0 ) {
		get_etherinfo(self->data->ethinfo, self->data->nlc, NLQRY_LINK);
		ret = RETURN_STRING(self->data->ethinfo->ipv4_address);
	} else if( strcmp(attr, "ipv4_netmask") == 0 ) {
		get_etherinfo(self->data->ethinfo, self->data->nlc, NLQRY_ADDR);
		ret = PyInt_FromLong(self->data->ethinfo->ipv4_netmask);
	} else if( strcmp(attr, "ipv4_broadcast") == 0 ) {
		get_etherinfo(self->data->ethinfo, self->data->nlc, NLQRY_ADDR);
		ret = RETURN_STRING(self->data->ethinfo->ipv4_broadcast);
	} else if( strcmp(attr, "ipv6_address") == 0 ) {
		get_etherinfo(self->data->ethinfo, self->data->nlc, NLQRY_ADDR);
		ret = RETURN_STRING(self->data->ethinfo->ipv6_address);
	} else if( strcmp(attr, "ipv6_netmask") == 0 ) {
		get_etherinfo(self->data->ethinfo, self->data->nlc, NLQRY_ADDR);
		ret = PyInt_FromLong(self->data->ethinfo->ipv6_netmask);
	} else {
		ret = PyObject_GenericGetAttr((PyObject *)self, attr_o);
	}
	return ret;
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

	if( !self || !self->data || !self->data->nlc || !self->data->ethinfo ) {
		PyErr_SetString(PyExc_AttributeError, "No data available");
		return NULL;
	}

	get_etherinfo(self->data->ethinfo, self->data->nlc, NLQRY_LINK);
	get_etherinfo(self->data->ethinfo, self->data->nlc, NLQRY_ADDR);

	ret = PyString_FromFormat("Device %s:\n", self->data->ethinfo->device);
	if( self->data->ethinfo->hwaddress ) {
		PyObject *tmp = PyString_FromFormat("\tMAC address: %s\n", self->data->ethinfo->hwaddress);
		PyString_Concat(&ret, tmp);
	}

	if( self->data->ethinfo->ipv4_address ) {
		PyObject *tmp = PyString_FromFormat("\tIPv4 address: %s/%i",
						   self->data->ethinfo->ipv4_address,
						   self->data->ethinfo->ipv4_netmask);
		if( self->data->ethinfo->ipv4_broadcast ) {
			PyObject *tmp2 = PyString_FromFormat("    Broadcast: %s",
							     self->data->ethinfo->ipv4_broadcast);
			PyString_Concat(&tmp, tmp2);
		}
		PyString_Concat(&tmp, PyString_FromString("\n"));
		PyString_Concat(&ret, tmp);
	}

	if( self->data->ethinfo->ipv6_address ) {
		PyObject *tmp = PyString_FromFormat("\tIPv6 address: %s/%i\n",
						   self->data->ethinfo->ipv6_address,
						   self->data->ethinfo->ipv6_netmask);
		PyString_Concat(&ret, tmp);
	}
	return ret;
}
