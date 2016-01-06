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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(ZLIB_CFLAGS) $(GCRYPT_CFLAGS) $(GNUTLS_CFLAGS)

libdir = ${dogechat_libdir}/plugins

lib_LTLIBRARIES = relay.la

relay_la_SOURCES = relay.c \
                   relay.h \
                   relay-buffer.c \
                   relay-buffer.h \
                   relay-client.c \
                   relay-client.h \
                   irc/relay-irc.c \
                   irc/relay-irc.h \
                   dogechat/relay-dogechat.c \
                   dogechat/relay-dogechat.h \
                   dogechat/relay-dogechat-msg.c \
                   dogechat/relay-dogechat-msg.h \
                   dogechat/relay-dogechat-nicklist.c \
                   dogechat/relay-dogechat-nicklist.h \
                   dogechat/relay-dogechat-protocol.c \
                   dogechat/relay-dogechat-protocol.h \
                   relay-command.c \
                   relay-command.h \
                   relay-completion.c \
                   relay-completion.h \
                   relay-config.c \
                   relay-config.h \
                   relay-info.c \
                   relay-info.h \
                   relay-network.c \
                   relay-network.h \
                   relay-raw.c \
                   relay-raw.h \
                   relay-server.c \
                   relay-server.h \
                   relay-upgrade.c \
                   relay-upgrade.h \
                   relay-websocket.c \
                   relay-websocket.h

relay_la_LDFLAGS = -module -no-undefined
relay_la_LIBADD  = $(RELAY_LFLAGS) $(ZLIB_LFLAGS) $(GCRYPT_LFLAGS) $(GNUTLS_LFLAGS)

EXTRA_DIST = CMakeLists.txt
