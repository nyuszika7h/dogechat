#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
             dogechat.1.it.asciidoc \
             cmdline_options.it.asciidoc \
             dogechat_user.it.asciidoc \
             dogechat_plugin_api.it.asciidoc \
             dogechat_scripting.it.asciidoc \
             dogechat_faq.it.asciidoc \
             dogechat_quickstart.it.asciidoc \
             dogechat_tester.it.asciidoc \
             $(wildcard autogen/user/*.asciidoc) \
             $(wildcard autogen/plugin_api/*.asciidoc)

if MAN
  man_targets = dogechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = dogechat_user.it.html \
		dogechat_plugin_api.it.html \
		dogechat_scripting.it.html \
		dogechat_faq.it.html \
		dogechat_quickstart.it.html \
		dogechat_tester.it.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
dogechat.1: dogechat.1.it.asciidoc cmdline_options.it.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/it/dogechat.1.it.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/it/cmdline_options.it.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="DogeChat $(VERSION)" -d manpage -f manpage -L dogechat.1.it.asciidoc)
	mv -f tmp_man/dogechat.1 .
	rm -rf tmp_man

# user's guide
dogechat_user.it.html: dogechat_user.it.asciidoc cmdline_options.it.asciidoc $(wildcard autogen/user/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_user.it.html $(abs_top_srcdir)/doc/it/dogechat_user.it.asciidoc

# plugin API reference
dogechat_plugin_api.it.html: dogechat_plugin_api.it.asciidoc $(wildcard autogen/plugin_api/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_plugin_api.it.html $(abs_top_srcdir)/doc/it/dogechat_plugin_api.it.asciidoc

# scripting guide
dogechat_scripting.it.html: dogechat_scripting.it.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_scripting.it.html $(abs_top_srcdir)/doc/it/dogechat_scripting.it.asciidoc

# FAQ
dogechat_faq.it.html: dogechat_faq.it.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_faq.it.html $(abs_top_srcdir)/doc/it/dogechat_faq.it.asciidoc

# quickstart
dogechat_quickstart.it.html: dogechat_quickstart.it.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_quickstart.it.html $(abs_top_srcdir)/doc/it/dogechat_quickstart.it.asciidoc

# tester's guide
dogechat_tester.it.html: dogechat_tester.it.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_tester.it.html $(abs_top_srcdir)/doc/it/dogechat_tester.it.asciidoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/it/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/it/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/it/man1/dogechat.1
	-rmdir $(DESTDIR)$(mandir)/it/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.it.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f dogechat.1 dogechat.1.it.asciidoc cmdline_options.it.asciidoc
	-rm -f *.html
