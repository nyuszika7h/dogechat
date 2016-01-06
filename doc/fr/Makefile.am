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
             dogechat.1.fr.asciidoc \
             cmdline_options.fr.asciidoc \
             dogechat_user.fr.asciidoc \
             dogechat_plugin_api.fr.asciidoc \
             dogechat_scripting.fr.asciidoc \
             dogechat_faq.fr.asciidoc \
             dogechat_quickstart.fr.asciidoc \
             dogechat_tester.fr.asciidoc \
             dogechat_relay_protocol.fr.asciidoc \
             dogechat_dev.fr.asciidoc \
             $(wildcard autogen/user/*.asciidoc) \
             $(wildcard autogen/plugin_api/*.asciidoc)

if MAN
  man_targets = dogechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = dogechat_user.fr.html \
		dogechat_plugin_api.fr.html \
		dogechat_scripting.fr.html \
		dogechat_faq.fr.html \
		dogechat_quickstart.fr.html \
		dogechat_tester.fr.html \
		dogechat_relay_protocol.fr.html \
		dogechat_dev.fr.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
dogechat.1: dogechat.1.fr.asciidoc cmdline_options.fr.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/fr/dogechat.1.fr.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/fr/cmdline_options.fr.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="DogeChat $(VERSION)" -d manpage -f manpage -L dogechat.1.fr.asciidoc)
	mv -f tmp_man/dogechat.1 .
	rm -rf tmp_man

# user's guide
dogechat_user.fr.html: dogechat_user.fr.asciidoc cmdline_options.fr.asciidoc $(wildcard autogen/user/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_user.fr.html $(abs_top_srcdir)/doc/fr/dogechat_user.fr.asciidoc

# plugin API reference
dogechat_plugin_api.fr.html: dogechat_plugin_api.fr.asciidoc $(wildcard autogen/plugin_api/*.asciidoc) $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_plugin_api.fr.html $(abs_top_srcdir)/doc/fr/dogechat_plugin_api.fr.asciidoc

# scripting guide
dogechat_scripting.fr.html: dogechat_scripting.fr.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_scripting.fr.html $(abs_top_srcdir)/doc/fr/dogechat_scripting.fr.asciidoc

# FAQ
dogechat_faq.fr.html: dogechat_faq.fr.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_faq.fr.html $(abs_top_srcdir)/doc/fr/dogechat_faq.fr.asciidoc

# quickstart
dogechat_quickstart.fr.html: dogechat_quickstart.fr.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o dogechat_quickstart.fr.html $(abs_top_srcdir)/doc/fr/dogechat_quickstart.fr.asciidoc

# tester's guide
dogechat_tester.fr.html: dogechat_tester.fr.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_tester.fr.html $(abs_top_srcdir)/doc/fr/dogechat_tester.fr.asciidoc

# relay protocol
dogechat_relay_protocol.fr.html: dogechat_relay_protocol.fr.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_relay_protocol.fr.html $(abs_top_srcdir)/doc/fr/dogechat_relay_protocol.fr.asciidoc

# developer's guide
dogechat_dev.fr.html: dogechat_dev.fr.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o dogechat_dev.fr.html $(abs_top_srcdir)/doc/fr/dogechat_dev.fr.asciidoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/fr/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/fr/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/fr/man1/dogechat.1
	-rmdir $(DESTDIR)$(mandir)/fr/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.fr.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f dogechat.1 dogechat.1.fr.asciidoc cmdline_options.fr.asciidoc
	-rm -f *.html
