#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2006 Julien Louis <ptitlouis@sysif.net>
#
# This file is part of DogeChat, the extensible chat client.
#
# DogeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# DogeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with DogeChat.  If not, see <http://www.gnu.org/licenses/>.
#

docdir = $(datadir)/doc/$(PACKAGE)

EXTRA_DIST = CMakeLists.txt \
             dogechat.1.de.asciidoc \
             cmdline_options.de.asciidoc \
             dogechat_user.de.asciidoc \
             dogechat_scripting.de.asciidoc \
             dogechat_faq.de.asciidoc \
             dogechat_quickstart.de.asciidoc \
             dogechat_tester.de.asciidoc \
             $(wildcard autogen/user/*.asciidoc)

if MAN
  man_targets = dogechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = dogechat_user.de.html \
		dogechat_scripting.de.html \
		dogechat_faq.de.html \
		dogechat_quickstart.de.html \
		dogechat_tester.de.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
dogechat.1: dogechat.1.de.asciidoc cmdline_options.de.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/de/dogechat.1.de.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/de/cmdline_options.de.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="DogeChat $(VERSION)" -d manpage -f manpage -L dogechat.1.de.asciidoc)
	mv -f tmp_man/dogechat.1 .
	rm -rf tmp_man

# user's guide
dogechat_user.de.html: dogechat_user.de.asciidoc cmdline_options.de.asciidoc $(wildcard autogen/user/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_user.de.html $(abs_top_srcdir)/doc/de/dogechat_user.de.asciidoc

# scripting guide
dogechat_scripting.de.html: dogechat_scripting.de.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_scripting.de.html $(abs_top_srcdir)/doc/de/dogechat_scripting.de.asciidoc

# FAQ
dogechat_faq.de.html: dogechat_faq.de.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_faq.de.html $(abs_top_srcdir)/doc/de/dogechat_faq.de.asciidoc

# quickstart
dogechat_quickstart.de.html: dogechat_quickstart.de.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_quickstart.de.html $(abs_top_srcdir)/doc/de/dogechat_quickstart.de.asciidoc

# tester's guide
dogechat_tester.de.html: dogechat_tester.de.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_tester.de.html $(abs_top_srcdir)/doc/de/dogechat_tester.de.asciidoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/de/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/de/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/de/man1/dogechat.1
	-rmdir $(DESTDIR)$(mandir)/de/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.de.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f dogechat.1 dogechat.1.de.asciidoc cmdline_options.de.asciidoc
	-rm -f *.html
