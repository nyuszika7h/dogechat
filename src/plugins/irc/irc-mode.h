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

#ifndef DOGECHAT_IRC_MODE_H
#define DOGECHAT_IRC_MODE_H 1

struct t_irc_server;
struct t_irc_channel;

extern int irc_mode_channel_set (struct t_irc_server *server,
                                 struct t_irc_channel *channel,
                                 const char *modes);
extern void irc_mode_user_set (struct t_irc_server *server, const char *modes,
                               int reset_modes);

#endif /* DOGECHAT_IRC_MODE_H */
