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
             dogechat.1.ja.asciidoc \
             cmdline_options.ja.asciidoc \
             dogechat_user.ja.asciidoc \
             dogechat_plugin_api.ja.asciidoc \
             dogechat_scripting.ja.asciidoc \
             dogechat_faq.ja.asciidoc \
             dogechat_quickstart.ja.asciidoc \
             dogechat_tester.ja.asciidoc \
             dogechat_relay_protocol.ja.asciidoc \
             dogechat_dev.ja.asciidoc \
             $(wildcard autogen/user/*.asciidoc) \
             $(wildcard autogen/plugin_api/*.asciidoc)

if MAN
  man_targets = dogechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = dogechat_user.ja.html \
		dogechat_plugin_api.ja.html \
		dogechat_scripting.ja.html \
		dogechat_faq.ja.html \
		dogechat_quickstart.ja.html \
		dogechat_tester.ja.html \
		dogechat_relay_protocol.ja.html \
		dogechat_dev.ja.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
dogechat.1: dogechat.1.ja.asciidoc cmdline_options.ja.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/ja/dogechat.1.ja.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/ja/cmdline_options.ja.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="DogeChat $(VERSION)" -d manpage -f manpage -L dogechat.1.ja.asciidoc)
	mv -f tmp_man/dogechat.1 .
	rm -rf tmp_man

# user's guide
dogechat_user.ja.html: dogechat_user.ja.asciidoc cmdline_options.ja.asciidoc $(wildcard autogen/user/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_user.ja.html $(abs_top_srcdir)/doc/ja/dogechat_user.ja.asciidoc

# plugin API reference
dogechat_plugin_api.ja.html: dogechat_plugin_api.ja.asciidoc $(wildcard autogen/plugin_api/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_plugin_api.ja.html $(abs_top_srcdir)/doc/ja/dogechat_plugin_api.ja.asciidoc

# scripting guide
dogechat_scripting.ja.html: dogechat_scripting.ja.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_scripting.ja.html $(abs_top_srcdir)/doc/ja/dogechat_scripting.ja.asciidoc

# FAQ
dogechat_faq.ja.html: dogechat_faq.ja.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_faq.ja.html $(abs_top_srcdir)/doc/ja/dogechat_faq.ja.asciidoc

# quickstart
dogechat_quickstart.ja.html: dogechat_quickstart.ja.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_quickstart.ja.html $(abs_top_srcdir)/doc/ja/dogechat_quickstart.ja.asciidoc

# tester's guide
dogechat_tester.ja.html: dogechat_tester.ja.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_tester.ja.html $(abs_top_srcdir)/doc/ja/dogechat_tester.ja.asciidoc

# relay protocol
dogechat_relay_protocol.ja.html: dogechat_relay_protocol.ja.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_relay_protocol.ja.html $(abs_top_srcdir)/doc/ja/dogechat_relay_protocol.ja.asciidoc

# developer's guide
dogechat_dev.ja.html: dogechat_dev.ja.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_dev.ja.html $(abs_top_srcdir)/doc/ja/dogechat_dev.ja.asciidoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/ja/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/ja/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/ja/man1/dogechat.1
	-rmdir $(DESTDIR)$(mandir)/ja/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.ja.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f dogechat.1 dogechat.1.ja.asciidoc cmdline_options.ja.asciidoc
	-rm -f *.html
