/*
 * Copyright (C) 2009-2010 Red Hat Inc.
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
 * @brief  Python ethtool.etherinfo class functions (header file).
 *
 */

#ifndef __ETHERINFO_OBJ_H
#define  __ETHERINFO_OBJ_H

#include <Python.h>
#include "etherinfo_struct.h"

void _ethtool_etherinfo_dealloc(PyEtherInfo *);
PyObject *_ethtool_etherinfo_new(PyTypeObject *, PyObject *, PyObject *);
int _ethtool_etherinfo_init(PyEtherInfo *, PyObject *, PyObject *);
PyObject *_ethtool_etherinfo_getter(PyEtherInfo *, PyObject *);
int _ethtool_etherinfo_setter(PyEtherInfo *, PyObject *, PyObject *);
PyObject *_ethtool_etherinfo_str(PyEtherInfo *self);

#endif
