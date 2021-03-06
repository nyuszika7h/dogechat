/*
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of DogeChat, the extensible chat client.
 *
 * DogeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * DogeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DogeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOGECHAT_XFER_CHAT_H
#define DOGECHAT_XFER_CHAT_H 1

extern void xfer_chat_sendf (struct t_xfer *xfer, const char *format, ...);
extern int xfer_chat_recv_cb (void *arg_xfer, int fd);
extern void xfer_chat_open_buffer (struct t_xfer *xfer);

#endif /* DOGECHAT_XFER_CHAT_H */
