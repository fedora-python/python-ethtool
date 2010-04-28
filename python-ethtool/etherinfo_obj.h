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
#include "etherinfo_struct.h"

void _ethtool_etherinfo_dealloc(etherinfo_py *);
PyObject *_ethtool_etherinfo_new(PyTypeObject *, PyObject *, PyObject *);
int _ethtool_etherinfo_init(etherinfo_py *, PyObject *, PyObject *);
PyObject *_ethtool_etherinfo_getter(etherinfo_py *, PyObject *);
int _ethtool_etherinfo_setter(etherinfo_py *, PyObject *, PyObject *);
PyObject *_ethtool_etherinfo_str(etherinfo_py *self);

#endif
