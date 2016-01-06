#
# Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
# Copyright (C) 2006-2016 Sébastien Helleu <flashcode@flashtux.org>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(ASPELL_CFLAGS) $(ENCHANT_CFLAGS)

libdir = ${dogechat_libdir}/plugins

lib_LTLIBRARIES = aspell.la

aspell_la_SOURCES = dogechat-aspell.c \
                    dogechat-aspell.h \
                    dogechat-aspell-bar-item.c \
                    dogechat-aspell-bar-item.h \
                    dogechat-aspell-command.c \
                    dogechat-aspell-command.h \
                    dogechat-aspell-completion.c \
                    dogechat-aspell-completion.h \
                    dogechat-aspell-config.c \
                    dogechat-aspell-config.h \
                    dogechat-aspell-info.c \
                    dogechat-aspell-info.h \
                    dogechat-aspell-speller.c \
                    dogechat-aspell-speller.h
aspell_la_LDFLAGS = -module -no-undefined
aspell_la_LIBADD  = $(ASPELL_LFLAGS) $(ENCHANT_LIBS)

EXTRA_DIST = CMakeLists.txt
