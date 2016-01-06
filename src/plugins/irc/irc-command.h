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

#ifndef DOGECHAT_IRC_COMMAND_H
#define DOGECHAT_IRC_COMMAND_H 1

struct t_irc_server;
struct t_irc_channel;

#define IRC_COMMAND_CHECK_SERVER(__command, __check_connection)         \
    if (!ptr_server)                                                    \
    {                                                                   \
        dogechat_printf (NULL,                                           \
                        _("%s%s: command \"%s\" must be executed on "   \
                          "irc buffer (server or channel)"),            \
                        dogechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        __command);                                     \
        return DOGECHAT_RC_OK;                                           \
    }                                                                   \
    if (__check_connection && !ptr_server->is_connected)                \
    {                                                                   \
        dogechat_printf (NULL,                                           \
                        _("%s%s: command \"%s\" must be executed on "   \
                          "connected irc server"),                      \
                        dogechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        __command);                                     \
        return DOGECHAT_RC_OK;                                           \
    }

/* list of supported capabilities (for completion in command /cap) */
#define IRC_COMMAND_CAP_SUPPORTED_COMPLETION \
    "account-notify|away-notify|cap-notify|extended-join|"              \
    "multi-prefix|server-time|userhost-in-names|%*"

extern void irc_command_away_server (struct t_irc_server *server,
                                     const char *arguments,
                                     int reset_unread_marker);
extern void irc_command_join_server (struct t_irc_server *server,
                                     const char *arguments,
                                     int manual_join,
                                     int noswitch);
extern void irc_command_mode_server (struct t_irc_server *server,
                                     const char *command,
                                     struct t_irc_channel *channel,
                                     const char *arguments,
                                     int flags);
extern void irc_command_part_channel (struct t_irc_server *server,
                                      const char *channel_name,
                                      const char *part_message);
extern void irc_command_quit_server (struct t_irc_server *server,
                                     const char *arguments);
extern void irc_command_init ();

#endif /* DOGECHAT_IRC_COMMAND_H */
