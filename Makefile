PACKAGE := python-ethtool
VERSION := $(shell rpm -q --qf '%{VERSION} ' --specfile rpm/SPECS/$(PACKAGE).spec | cut -d' ' -f1)

rpmdirs:
	@[ -d rpm/BUILD ]   || mkdir rpm/BUILD
	@[ -d rpm/RPMS ]    || mkdir rpm/RPMS
	@[ -d rpm/SRPMS ]   || mkdir rpm/SRPMS
	@[ -d rpm/SOURCES ] || mkdir rpm/SOURCES

bz2: rpmdirs
	git archive --format=tar --prefix=$(PACKAGE)-$(VERSION)/ HEAD | \
	bzip2 -9 > rpm/SOURCES/$(PACKAGE)-$(VERSION).tar.bz2

install:
	python setup.py install --root=$(DESTDIR)

rpm: bz2 rpmdirs
	rpmbuild -ba --define "_topdir $(PWD)/rpm" rpm/SPECS/$(PACKAGE).spec

bz2dev: rpmdirs
	@mkdir -p /tmp/$(PACKAGE)-$(VERSION)
	@tar cf - `cat MANIFEST` | (cd /tmp/$(PACKAGE)-$(VERSION) ; tar xf -)
	@(cd /tmp; tar cf - $(PACKAGE)-$(VERSION)) | bzip2 -9 > rpm/SOURCES/$(PACKAGE)-$(VERSION).tar.bz2

rpmdev: bz2dev rpmdirs
	rpmbuild -ba --define "_topdir $(PWD)/rpm" rpm/SPECS/$(PACKAGE).spec


build:
	python setup.py build

# Need to set PYTHONPATH appropriately when invoking this
test:
	python tests/parse_ifconfig.py -v
	python -m unittest discover -v
	valgrind --leak-check=full python tests/test_ethtool.py

# As of 5f6339c432dc227e456b70c459cf6f57c6cfe7c2 I (dmalcolm) hope to have
# fixed the memory leaks within python-ethtool itself, but I expect
# to see a few blocks lost within libnl which appear to be caches,
# and thus (presumably) false positives:
# ==9163== 4,600 (80 direct, 4,520 indirect) bytes in 2 blocks are definitely lost in loss record 2,559 of 2,612
# ==9163==    at 0x4C26F18: calloc (vg_replace_malloc.c:566)
# ==9163==    by 0x1152C307: nl_cache_alloc (in /usr/lib64/libnl.so.1.1)
# ==9163==    by 0x115309FC: rtnl_link_alloc_cache (in /usr/lib64/libnl.so.1.1)
# ==9163==    by 0x1130C843: get_etherinfo (etherinfo.c:315)
# ==9163==    by 0x1130CFD0: _ethtool_etherinfo_getter (etherinfo_obj.c:152)
# ==9163==    by 0x4F07E90: PyEval_EvalFrameEx (ceval.c:2330)
# ...etc...
# ==9163== LEAK SUMMARY:
# ==9163==    definitely lost: 160 bytes in 4 blocks
# ==9163==    indirectly lost: 9,040 bytes in 60 blocks
# ==9163==      possibly lost: 489,676 bytes in 3,299 blocks
# ==9163==    still reachable: 1,277,848 bytes in 11,100 blocks
# ==9163==         suppressed: 0 bytes in 0 blocks
