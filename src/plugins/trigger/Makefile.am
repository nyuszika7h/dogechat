#
# Copyright (C) 2014-2016 Sébastien Helleu <flashcode@flashtux.org>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(TRIGGER_CFLAGS)

libdir = ${dogechat_libdir}/plugins

lib_LTLIBRARIES = trigger.la

trigger_la_SOURCES = trigger.c \
                     trigger.h \
                     trigger-buffer.c \
                     trigger-buffer.h \
                     trigger-callback.c \
                     trigger-callback.h \
                     trigger-command.c \
                     trigger-command.h \
                     trigger-completion.c \
                     trigger-completion.h \
                     trigger-config.c \
                     trigger-config.h

trigger_la_LDFLAGS = -module -no-undefined
trigger_la_LIBADD  = $(TRIGGER_LFLAGS)

EXTRA_DIST = CMakeLists.txt
