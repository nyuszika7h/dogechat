#
# Copyright (C) 2011-2016 Sébastien Helleu <flashcode@flashtux.org>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(GUILE_CFLAGS)

libdir = ${dogechat_libdir}/plugins

lib_LTLIBRARIES = guile.la

guile_la_SOURCES = dogechat-guile.c \
                   dogechat-guile.h \
                   dogechat-guile-api.c \
                   dogechat-guile-api.h
guile_la_LDFLAGS = -module -no-undefined
guile_la_LIBADD  = ../lib_dogechat_plugins_scripts.la $(GUILE_LFLAGS)

EXTRA_DIST = CMakeLists.txt
