/**
 * @file   etherinfo_obj.c
 * @author David Sommerseth <davids@redhat.com>
 * @date   Fri Sep  4 18:41:28 2009
 *
 * @brief  Python ethtool.etherinfo class functions (header file).
 *
 */

#ifndef __ETHERINFO_OBJ_H
#define  __ETHERINFO_OBJ_H

#include <Python.h>
#include "structmember.h"
#include "etherinfo.h"
#include "etherinfo_struct.h"

void _ethtool_etherinfo_dealloc(etherinfo_py *);
PyObject *_ethtool_etherinfo_new(PyTypeObject *, PyObject *, PyObject *);
int _ethtool_etherinfo_init(etherinfo_py *, PyObject *, PyObject *);
PyObject *_ethtool_etherinfo_getter(etherinfo_py *, PyObject *);
int _ethtool_etherinfo_setter(etherinfo_py *, PyObject *, PyObject *);
PyObject *_ethtool_etherinfo_str(etherinfo_py *self);

/**
 * This is required by Python, which lists all accessible methods
 * in the object.  But no methods are provided.
 *
 */
static PyMethodDef _ethtool_etherinfo_methods[] = {
    {NULL}  /**< No methods defined */
};

/**
 * Defines all accessible object members
 *
 */
static PyMemberDef _ethtool_etherinfo_members[] = {
    {"device", T_OBJECT_EX, offsetof(etherinfo_py, data), 0,
     "Device name of the interface"},
    {"mac_address", T_OBJECT_EX, offsetof(etherinfo_py, data), 0,
     "MAC address / hardware address of the interface"},
    {"ipv4_address", T_OBJECT_EX, offsetof(etherinfo_py, data), 0,
     "IPv4 address"},
    {"ipv4_netmask", T_INT, offsetof(etherinfo_py, data), 0,
     "IPv4 netmask in bits"},
    {"ipv4_broadcast", T_OBJECT_EX, offsetof(etherinfo_py, data), 0,
     "IPv4 broadcast address"},
    {"ipv6_address", T_OBJECT_EX, offsetof(etherinfo_py, data), 0,
     "IPv6 address"},
    {"ipv6_netmask", T_INT, offsetof(etherinfo_py, data), 0,
     "IPv6 netmask in bits"},
    {NULL}  /* End of member list */
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
    _ethtool_etherinfo_members,            /* tp_members */
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

#endif
