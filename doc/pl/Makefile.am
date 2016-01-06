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
             dogechat.1.pl.asciidoc \
             cmdline_options.pl.asciidoc \
             dogechat_user.pl.asciidoc \
             dogechat_scripting.pl.asciidoc \
             dogechat_faq.pl.asciidoc \
             dogechat_quickstart.pl.asciidoc \
             dogechat_tester.pl.asciidoc \
             $(wildcard autogen/user/*.asciidoc)

if MAN
  man_targets = dogechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = dogechat_user.pl.html \
		dogechat_scripting.pl.html \
		dogechat_faq.pl.html \
		dogechat_quickstart.pl.html \
		dogechat_tester.pl.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
dogechat.1: dogechat.1.pl.asciidoc cmdline_options.pl.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/pl/dogechat.1.pl.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/pl/cmdline_options.pl.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="DogeChat $(VERSION)" -d manpage -f manpage -L dogechat.1.pl.asciidoc)
	mv -f tmp_man/dogechat.1 .
	rm -rf tmp_man

# user's guide
dogechat_user.pl.html: dogechat_user.pl.asciidoc cmdline_options.pl.asciidoc $(wildcard autogen/user/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_user.pl.html $(abs_top_srcdir)/doc/pl/dogechat_user.pl.asciidoc

# scripting guide
dogechat_scripting.pl.html: dogechat_scripting.pl.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_scripting.pl.html $(abs_top_srcdir)/doc/pl/dogechat_scripting.pl.asciidoc

# FAQ
dogechat_faq.pl.html: dogechat_faq.pl.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_faq.pl.html $(abs_top_srcdir)/doc/pl/dogechat_faq.pl.asciidoc

# quickstart
dogechat_quickstart.pl.html: dogechat_quickstart.pl.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_quickstart.pl.html $(abs_top_srcdir)/doc/pl/dogechat_quickstart.pl.asciidoc

# tester's guide
dogechat_tester.pl.html: dogechat_tester.pl.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_tester.pl.html $(abs_top_srcdir)/doc/pl/dogechat_tester.pl.asciidoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/pl/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/pl/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/pl/man1/dogechat.1
	-rmdir $(DESTDIR)$(mandir)/pl/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.pl.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f dogechat.1 dogechat.1.pl.asciidoc cmdline_options.pl.asciidoc
	-rm -f *.html
