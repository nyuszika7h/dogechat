#
# Copyright (C) 2015-2016 Sébastien Helleu <flashcode@flashtux.org>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(V8_CFLAGS)

libdir = ${dogechat_libdir}/plugins

lib_LTLIBRARIES = javascript.la

javascript_la_SOURCES = dogechat-js.cpp \
                        dogechat-js.h \
                        dogechat-js-v8.cpp \
                        dogechat-js-v8.h \
                        dogechat-js-api.cpp \
                        dogechat-js-api.h
javascript_la_LDFLAGS = -module -no-undefined
javascript_la_LIBADD  = ../lib_dogechat_plugins_scripts.la $(V8_LFLAGS)

EXTRA_DIST = CMakeLists.txt
