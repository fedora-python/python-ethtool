#!/usr/bin/python
# -*- coding: utf-8 -*-

from __future__ import print_function

from setuptools import setup, Extension
import sys

try:
    import commands
except ImportError:
    import subprocess as commands

version = '0.13'


class PkgConfigExtension(Extension):
    '''Extension with lazy properties taken from pkg-config'''

    def __init__(self, *args, **kwargs):
        '''Behaves like Extension's __init__ but you should not use
        include_dirs, library_dirs and libraries arguments.

        Extra arguments:
          pkg : string
            The name of the pkg-config package to use for querying
          extra_libraris : [string]
            This will be added to the libraries attribute. Optional.
        '''
        self._pkg = kwargs['pkg']
        del kwargs['pkg']
        if 'extra_libraries' in kwargs:
            self._extra_libraries = kwargs['extra_libraries']
            del kwargs['extra_libraries']
        else:
            self._extra_libraries = []

        Extension.__init__(self, *args, **kwargs)

        try:
            # on Python 2 we need to delete those now
            del self.include_dirs
            del self.library_dirs
            del self.libraries
        except AttributeError:
            # on Python 3, that's not needed or possible
            pass

    @classmethod
    def _str2list(cls, pkgstr, onlystr):
        res = []
        for l in pkgstr.split(" "):
            if l.find(onlystr) == 0:
                res.append(l.replace(onlystr, "", 1))
        return res

    @classmethod
    def _run(cls, command_string):
        res, output = commands.getstatusoutput(command_string)
        if res != 0:
            print('Failed to query %s' % command_string)
            sys.exit(1)
        return output

    @property
    def include_dirs(self):
        includes = self._run('pkg-config --cflags-only-I %s' % self._pkg)
        return self._str2list(includes, '-I')

    @property
    def library_dirs(self):
        libdirs = self._run('pkg-config --libs-only-L %s' % self._pkg)
        return self._str2list(libdirs, '-L')

    @property
    def libraries(self):
        libs = self._run('pkg-config --libs-only-l %s' % self._pkg)
        return self._str2list(libs, '-l') + self._extra_libraries

    @include_dirs.setter
    def include_dirs(self, value):
        pass

    @library_dirs.setter
    def library_dirs(self, value):
        pass

    @libraries.setter
    def libraries(self, value):
        pass


with open('README.rst') as f:
    long_description = f.read()

with open('CHANGES.rst') as f:
    long_description += '\n\n'
    long_description += f.read()

setup(name='ethtool',
      version=version,
      description='Python module to interface with ethtool',
      long_description=long_description,

      author='Harald Hoyer, Arnaldo Carvalho de Melo, David Sommerseth',
      author_email='davids@redhat.com',

      maintainer='Lumír Balhar, Miro Hrončok, Charalampos Stratakis, Sanqui',
      maintainer_email='python-maint@redhat.com',

      url='https://github.com/fedora-python/python-ethtool',
      license='GPL-2.0',
      keywords='network networking ethernet tool ethtool',

      classifiers=[
          'Intended Audience :: Developers',
          'Intended Audience :: System Administrators',
          'Operating System :: POSIX :: Linux',
          'License :: OSI Approved :: GNU General Public License v2 (GPLv2)',
          'Programming Language :: Python',
          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 3',
          'Programming Language :: Python :: 3.5',
          'Programming Language :: Python :: 3.6',
          'Programming Language :: Python :: Implementation :: CPython',
          'Topic :: Software Development :: Libraries',
          'Topic :: System :: Networking',
      ],
      
      scripts=[
        'scripts/pethtool', 'scripts/pifconfig'
      ],
      
      ext_modules=[
          PkgConfigExtension(
              'ethtool',
              sources=[
                  'python-ethtool/ethtool.c',
                  'python-ethtool/etherinfo.c',
                  'python-ethtool/etherinfo_obj.c',
                  'python-ethtool/netlink.c',
                  'python-ethtool/netlink-address.c'],
              extra_compile_args=[
                  '-fno-strict-aliasing', '-Wno-unused-function'],
              define_macros=[('VERSION', '"%s"' % version)],
              pkg='libnl-3.0',
              extra_libraries=['nl-route-3'],
          )
      ]
      )
