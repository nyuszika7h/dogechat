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

SUBDIRS = . en fr it de pl es ru ja cs

EXTRA_DIST = docgen.py \
             CMakeLists.txt \
             asciidoc.conf \
             asciidoc_toc.css \
             asciidoc_toc2.css

uninstall-hook:
	-rmdir $(DESTDIR)$(datadir)/doc/$(PACKAGE)

clean-local:
	-rm -f asciidoc_toc.css asciidoc_toc2.css
