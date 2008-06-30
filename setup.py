#!/usr/bin/python2

from distutils.core import setup, Extension

ethtool = Extension('ethtool',
		    sources = ['python-ethtool/ethtool.c'])

# don't reformat this line, Makefile parses it
setup(name='ethtool',
      version='0.2',
      description='Python module to interface with ethtool',
      author='Harald Hoyer & Arnaldo Carvalho de Melo',
      author_email='acme@redhat.com',
      url='http://fedoraproject.org/wiki/python-ethtool',
      ext_modules=[ethtool])
