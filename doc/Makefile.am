docdir = $(prefix)/share/doc/@PACKAGE@
htmldir = $(docdir)

man_MANS = Amy.6

EXTRA_DIST = $(man_MANS) Handbook.xml amy.css *.html

index.html: Handbook.xml
	test -x ./style.sh && ./style.sh

.PHONY: all-local
all-local: html

.PHONY: html
html: index.html

install-data-local: install-html

install-html:
	$(mkinstalldirs) $(DESTDIR)$(htmldir)
	$(INSTALL_DATA) *.html $(DESTDIR)$(htmldir)
	$(INSTALL_DATA) amy.css $(DESTDIR)$(htmldir)

clean:
	-rm *.html
