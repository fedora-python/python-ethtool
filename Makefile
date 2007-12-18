PKGNAME = python-ethtool
VERSION = $(shell grep version setup.py | cut -f 2 -d \' )
TAG = $(PKGNAME)-$(VERSION)
GITURL = git://git.kernel.org/pub/scm/linux/kernel/git/acme/$(PKGNAME).git

all:

clean:
	-rm *.tar.gz python-ethtool/*.pyc
	-rm -r dist MANIFEST
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
