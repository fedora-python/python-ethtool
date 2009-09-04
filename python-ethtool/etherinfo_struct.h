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

/**
 * Contains the internal data structure of the
 * ethtool.etherinfo object.
 *
 */
typedef struct {
    PyObject_HEAD
    struct etherinfo *info;	/**< Contains information about one ethernet device */
} etherinfo_py;

#endif
