#! /usr/bin/python
# -*- python -*-
# -*- coding: utf-8 -*-
#   Copyright (C) 2008 Red Hat Inc.
#
#   This application is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   as published by the Free Software Foundation; version 2.
#
#   This application is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.

import getopt, ethtool, sys

def usage():
	print '''Usage: ethtool [OPTIONS] [<interface>]
	-h|--help               Give this help list
	-i|--driver             Show driver information
	-k|--show-offload       Get protocol offload information
	-K|--offload            Set protocol offload
		[ tso on|off ]'''

tab = ""

def printtab(msg):
	print tab + msg

def show_offload(interface, args = None):
	try:
		sg = ethtool.get_sg(interface) and "on" or "off"
	except IOError:
		sg = "not supported"

	try:
		tso = ethtool.get_tso(interface) and "on" or "off"
	except IOError:
		tso = "not supported"

	try:
		ufo = ethtool.get_ufo(interface) and "on" or "off"
	except IOError:
		ufo = "not supported"

	try:
		gso = ethtool.get_gso(interface) and "on" or "off"
	except IOError:
		gso = "not supported"

	printtab("scatter-gather: %s" % sg)
	printtab("tcp segmentation offload: %s" % tso)
	printtab("udp fragmentation offload: %s" % ufo)
	printtab("generic segmentation offload: %s" % gso)

def set_offload(interface, args):
	cmd, value = [a.lower() for a in args]

	if cmd == "tso":
		value = value == "on" and 1 or 0
		try:
			ethtool.set_tso(interface, value)
		except:
			pass

def show_driver(interface, args = None):
	try:
		driver = ethtool.get_module(interface)
	except IOError:
		driver = "not implemented"

	try:
		bus = ethtool.get_businfo(interface)
	except IOError:
		bus = "not available"

	printtab("driver: %s" % driver)
	printtab("bus-info: %s" % bus)

def run_cmd(cmd, interface, args):
	global tab

	active_devices = ethtool.get_active_devices()
	if not interface:
		tab = "  "
		for interface in ethtool.get_devices():
			inactive = " (not active)"
			if interface in active_devices:
				inactive = ""
			print "%s%s:" % (interface, inactive)
			cmd(interface, args)
	else:
		cmd(interface, args)

def run_cmd_noargs(cmd, args):
	if args:
		run_cmd(cmd, args[0], None)
	else:
		run_cmd(cmd, None, None)

def main():
	try:
		opts, args = getopt.getopt(sys.argv[1:],
					   "hikK",
					   ("help",
					    "driver",
					    "show-offload",
					    "offload"))
	except getopt.GetoptError, err:
		usage()
		print str(err)
		sys.exit(2)

	if not opts:
		usage()
		sys.exit(0)

	for o, a in opts:
		if o in ("-h", "--help"):
			usage()
			return
		elif o in ("-i", "--driver"):
			run_cmd_noargs(show_driver, args)
			break
		elif o in ("-k", "--show-offload"):
			run_cmd_noargs(show_offload, args)
			break
		elif o in ("-K", "--offload"):
			if len(args) == 2:
				interface = None
			elif len(args) == 3:
				interface = args[0]
				args = args[1:]
			else:
				usage()
				sys.exit(1)
			run_cmd(set_offload, interface, args)
			break

if __name__ == '__main__':
    main()
