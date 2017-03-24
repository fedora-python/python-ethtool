#!/usr/bin/python2

from __future__ import print_function

from distutils.core import setup, Extension
try:
    import commands
except ImportError:
    import subprocess as commands
import sys

version = '0.12'

def pkgconfig(pkg):
    def _str2list(pkgstr, onlystr):
        res = []
        for l in pkgstr.split(" "):
            if l.find(onlystr) == 0:
                res.append(l.replace(onlystr, "", 1))
        return res

    (res, cflags) = commands.getstatusoutput('pkg-config --cflags-only-other %s' % pkg)
    if res != 0:
        print('Failed to query pkg-config --cflags-only-other %s' % pkg)
        sys.exit(1)

    (res, includes) = commands.getstatusoutput('pkg-config --cflags-only-I %s' % pkg)
    if res != 0:
        print('Failed to query pkg-config --cflags-only-I %s' % pkg)
        sys.exit(1)

    (res, libs) = commands.getstatusoutput('pkg-config --libs-only-l %s' % pkg)
    if res != 0:
        print('Failed to query pkg-config --libs-only-l %s' % pkg)
        sys.exit(1)

    (res, libdirs) = commands.getstatusoutput('pkg-config --libs-only-L %s' % pkg)
    if res != 0:
        print('Failed to query pkg-config --libs-only-L %s' % pkg)
        sys.exit(1)


    # Clean up the results and return what we've extracted from pkg-config
    return {'cflags': cflags,
            'include': _str2list(includes, '-I'),
            'libs': _str2list(libs, '-l'),
            'libdirs': _str2list(libdirs, '-L')
            }


libnl = pkgconfig('libnl-3.0')
libnl['libs'].append('nl-route-3')

setup(name='ethtool',
      version=version,
      description='Python module to interface with ethtool',
      author='Harald Hoyer, Arnaldo Carvalho de Melo, David Sommerseth',
      author_email='davids@redhat.com',
      url='https://github.com/fedora-python/python-ethtool',
      ext_modules=[
        Extension(
            'ethtool',
            sources = [
                'python-ethtool/ethtool.c',
                'python-ethtool/etherinfo.c',
                'python-ethtool/etherinfo_obj.c',
                'python-ethtool/netlink.c',
                'python-ethtool/netlink-address.c'],
            extra_compile_args=['-fno-strict-aliasing'],
            include_dirs = libnl['include'],
            library_dirs = libnl['libdirs'],
            libraries = libnl['libs'],
            define_macros = [('VERSION', '"%s"' % version)]
            )
        ]
)
