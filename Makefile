PKGNAME = python-ethtool
VERSION = $(shell grep version setup.py | cut -f 2 -d \' )
TAG = $(PKGNAME)-$(VERSION)
GITURL = git://git.kernel.org/pub/scm/linux/kernel/git/acme/$(PKGNAME).git

all:

clean:
	-rm *.tar.gz python-ethtool/*.pyc
	-rm rpm/SOURCES/${PKGNAME}-$(VERSION).tar.bz2
	-rm -rf rpm/{BUILD,RPMS/*,SRPMS}/*
	python setup.py -q clean --all

install: all
	python setup.py install --root=$(DESTDIR)

tag:
	git tag $(TAG)

push:
	@git tag -l | grep -q $(TAG) && \
	    git push origin $(TAG) || \
	    echo Not tagged.

archive:
	@rm -rf /tmp/${PKGNAME}-$(VERSION) /tmp/${PKGNAME}
	@cd /tmp; git clone -q $(GITURL) $(PKGNAME)
	@mv /tmp/${PKGNAME} /tmp/${PKGNAME}-$(VERSION)
	@cd /tmp/${PKGNAME}-$(VERSION) ; git checkout $(TAG)
	@cd /tmp/${PKGNAME}-$(VERSION) ; python setup.py -q sdist
	@cp /tmp/${PKGNAME}-$(VERSION)/dist/${PKGNAME}-$(VERSION).tar.gz .
	@rm -rf /tmp/${PKGNAME}-$(VERSION)
	@echo "The archive is in ${PKGNAME}-$(VERSION).tar.gz"

local:
	@rm -rf ${PKGNAME}-$(VERSION).tar.gz
	@rm -rf /tmp/${PKGNAME}-$(VERSION) /tmp/${PKGNAME}
	@dir=$$PWD; cp -a $$dir /tmp/${PKGNAME}-$(VERSION)
	@cd /tmp/${PKGNAME}-$(VERSION) ; python setup.py -q sdist
	@cp /tmp/${PKGNAME}-$(VERSION)/dist/${PKGNAME}-$(VERSION).tar.gz .
	@rm -rf /tmp/${PKGNAME}-$(VERSION)	
	@echo "The archive is in ${PKGNAME}-$(VERSION).tar.gz"

bz2:
	git archive --format=tar --prefix=${PKGNAME}-$(VERSION)/ HEAD | bzip2 -9 > rpm/SOURCES/${PKGNAME}-$(VERSION).tar.bz2

rpm: bz2
	rpmbuild -ba --define "_topdir $(PWD)/rpm" rpm/SPECS/${PKGNAME}.spec

bz2dev:
	@mkdir -p /tmp/${PKGNAME}-$(VERSION)
	@tar cf - `cat MANIFEST` | (cd /tmp/${PKGNAME}-$(VERSION) ; tar xf -)
	@(cd /tmp; tar cf - ${PKGNAME}-$(VERSION)) | bzip2 -9 > rpm/SOURCES/${PKGNAME}-$(VERSION).tar.bz2

rpmdev: bz2dev
	rpmbuild -ba --define "_topdir $(PWD)/rpm" rpm/SPECS/${PKGNAME}.spec
