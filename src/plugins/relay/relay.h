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

#ifndef DOGECHAT_RELAY_H
#define DOGECHAT_RELAY_H 1

#define dogechat_plugin dogechat_relay_plugin
#define RELAY_PLUGIN_NAME "relay"

extern struct t_dogechat_plugin *dogechat_relay_plugin;

extern int relay_signal_upgrade_received;

/* relay protocol */

enum t_relay_protocol
{
    RELAY_PROTOCOL_DOGECHAT = 0,        /* DogeChat protocol                  */
    RELAY_PROTOCOL_IRC,                /* IRC protocol (IRC proxy)          */
    /* number of relay protocols */
    RELAY_NUM_PROTOCOLS,
};

#define RELAY_COLOR_CHAT dogechat_color("chat")
#define RELAY_COLOR_CHAT_HOST dogechat_color("chat_host")
#define RELAY_COLOR_CHAT_BUFFER dogechat_color("chat_buffer")
#define RELAY_COLOR_CHAT_CLIENT dogechat_color(dogechat_config_string(relay_config_color_client))

extern char *relay_protocol_string[];

extern int relay_protocol_search (const char *name);

#endif /* DOGECHAT_RELAY_H */
