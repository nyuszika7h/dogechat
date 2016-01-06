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
             dogechat.1.en.asciidoc \
             cmdline_options.en.asciidoc \
             dogechat_user.en.asciidoc \
             dogechat_plugin_api.en.asciidoc \
             dogechat_scripting.en.asciidoc \
             dogechat_faq.en.asciidoc \
             dogechat_quickstart.en.asciidoc \
             dogechat_tester.en.asciidoc \
             dogechat_relay_protocol.en.asciidoc \
             dogechat_dev.en.asciidoc \
             $(wildcard autogen/user/*.asciidoc) \
             $(wildcard autogen/plugin_api/*.asciidoc)

if MAN
  man_targets = dogechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = dogechat_user.en.html \
		dogechat_plugin_api.en.html \
		dogechat_scripting.en.html \
		dogechat_faq.en.html \
		dogechat_quickstart.en.html \
		dogechat_tester.en.html \
		dogechat_relay_protocol.en.html \
		dogechat_dev.en.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
dogechat.1: dogechat.1.en.asciidoc cmdline_options.en.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/en/dogechat.1.en.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/en/cmdline_options.en.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="DogeChat $(VERSION)" -d manpage -f manpage -L dogechat.1.en.asciidoc)
	mv -f tmp_man/dogechat.1 .
	rm -rf tmp_man

# user's guide
dogechat_user.en.html: dogechat_user.en.asciidoc cmdline_options.en.asciidoc $(wildcard autogen/user/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_user.en.html $(abs_top_srcdir)/doc/en/dogechat_user.en.asciidoc

# plugin API reference
dogechat_plugin_api.en.html: dogechat_plugin_api.en.asciidoc $(wildcard autogen/plugin_api/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_plugin_api.en.html $(abs_top_srcdir)/doc/en/dogechat_plugin_api.en.asciidoc

# scripting guide
dogechat_scripting.en.html: dogechat_scripting.en.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_scripting.en.html $(abs_top_srcdir)/doc/en/dogechat_scripting.en.asciidoc

# FAQ
dogechat_faq.en.html: dogechat_faq.en.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_faq.en.html $(abs_top_srcdir)/doc/en/dogechat_faq.en.asciidoc

# quickstart
dogechat_quickstart.en.html: dogechat_quickstart.en.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_quickstart.en.html $(abs_top_srcdir)/doc/en/dogechat_quickstart.en.asciidoc

# tester's guide
dogechat_tester.en.html: dogechat_tester.en.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_tester.en.html $(abs_top_srcdir)/doc/en/dogechat_tester.en.asciidoc

# relay protocol
dogechat_relay_protocol.en.html: dogechat_relay_protocol.en.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_relay_protocol.en.html $(abs_top_srcdir)/doc/en/dogechat_relay_protocol.en.asciidoc

# developer's guide
dogechat_dev.en.html: dogechat_dev.en.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_dev.en.html $(abs_top_srcdir)/doc/en/dogechat_dev.en.asciidoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/man1/dogechat.1
	-rmdir $(DESTDIR)$(mandir)/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.en.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f dogechat.1 dogechat.1.en.asciidoc cmdline_options.en.asciidoc
	-rm -f *.html
