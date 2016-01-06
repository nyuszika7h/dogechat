/*
 * irc.c - IRC (Internet Relay Chat) plugin for DogeChat
 *
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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../dogechat-plugin.h"
#include "irc.h"
#include "irc-bar-item.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-command.h"
#include "irc-completion.h"
#include "irc-config.h"
#include "irc-debug.h"
#include "irc-ignore.h"
#include "irc-info.h"
#include "irc-input.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-protocol.h"
#include "irc-raw.h"
#include "irc-redirect.h"
#include "irc-server.h"
#include "irc-upgrade.h"


DOGECHAT_PLUGIN_NAME(IRC_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("IRC (Internet Relay Chat) protocol"));
DOGECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(5000);

struct t_dogechat_plugin *dogechat_irc_plugin = NULL;

struct t_hook *irc_hook_timer = NULL;

int irc_signal_upgrade_received = 0;   /* signal "upgrade" received ?       */


/*
 * Callback for signal "quit".
 */

int
irc_signal_quit_cb (void *data, const char *signal, const char *type_data,
                    void *signal_data)
{
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) data;
    (void) signal;

    if (strcmp (type_data, DOGECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            irc_command_quit_server (
                ptr_server,
                (signal_data) ? (char *)signal_data : NULL);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback for signal "upgrade".
 */

int
irc_signal_upgrade_cb (void *data, const char *signal, const char *type_data,
                       void *signal_data)
{
    struct t_irc_server *ptr_server;
    int quit, ssl_disconnected;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    irc_signal_upgrade_received = 1;

    quit = (signal_data && (strcmp (signal_data, "quit") == 0));
    ssl_disconnected = 0;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /*
         * FIXME: it's not possible to upgrade with SSL servers connected
         * (GnuTLS library can't reload data after upgrade), so we close
         * connection for all SSL servers currently connected
         */
        if (ptr_server->is_connected && (ptr_server->ssl_connected || quit))
        {
            if (!quit)
            {
                ssl_disconnected++;
                dogechat_printf (
                    ptr_server->buffer,
                    _("%s%s: disconnecting from server because upgrade can't "
                      "work for servers connected via SSL"),
                    dogechat_prefix ("error"), IRC_PLUGIN_NAME);
            }
            irc_server_disconnect (ptr_server, 0, 0);
            /*
             * schedule reconnection: DogeChat will reconnect to this server
             * after restart
             */
            ptr_server->index_current_address = 0;
            ptr_server->reconnect_delay = IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY);
            ptr_server->reconnect_start = time (NULL) - ptr_server->reconnect_delay - 1;
        }
    }
    if (ssl_disconnected > 0)
    {
        dogechat_printf (
            NULL,
            /* TRANSLATORS: "%s" after "%d" is "server" or "servers" */
            _("%s%s: disconnected from %d %s (SSL connection not supported "
              "with upgrade)"),
            dogechat_prefix ("error"),
            IRC_PLUGIN_NAME,
            ssl_disconnected,
            NG_("server", "servers", ssl_disconnected));
    }

    return DOGECHAT_RC_OK;
}

/*
 * Initializes IRC plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    int i, auto_connect, upgrading;

    dogechat_plugin = plugin;

    if (!irc_config_init ())
        return DOGECHAT_RC_ERROR;

    irc_config_read ();

    irc_command_init ();

    irc_info_init ();

    irc_redirect_init ();

    irc_notify_init ();

    /* hook some signals */
    irc_debug_init ();
    dogechat_hook_signal ("quit", &irc_signal_quit_cb, NULL);
    dogechat_hook_signal ("upgrade", &irc_signal_upgrade_cb, NULL);
    dogechat_hook_signal ("xfer_send_ready", &irc_server_xfer_send_ready_cb, NULL);
    dogechat_hook_signal ("xfer_resume_ready", &irc_server_xfer_resume_ready_cb, NULL);
    dogechat_hook_signal ("xfer_send_accept_resume", &irc_server_xfer_send_accept_resume_cb, NULL);
    dogechat_hook_signal ("irc_input_send", &irc_input_send_cb, NULL);

    /* hook hsignals for redirection */
    dogechat_hook_hsignal ("irc_redirect_pattern", &irc_redirect_pattern_hsignal_cb, NULL);
    dogechat_hook_hsignal ("irc_redirect_command", &irc_redirect_command_hsignal_cb, NULL);

    /* modifiers */
    dogechat_hook_modifier ("irc_color_decode", &irc_color_modifier_cb, NULL);
    dogechat_hook_modifier ("irc_color_encode", &irc_color_modifier_cb, NULL);
    dogechat_hook_modifier ("irc_color_decode_ansi", &irc_color_modifier_cb, NULL);

    /* hook completions */
    irc_completion_init ();

    irc_bar_item_init ();

    /* look at arguments */
    auto_connect = 1;
    upgrading = 0;
    for (i = 0; i < argc; i++)
    {
        if ((dogechat_strcasecmp (argv[i], "-a") == 0)
            || (dogechat_strcasecmp (argv[i], "--no-connect") == 0))
        {
            auto_connect = 0;
        }
        else if ((dogechat_strncasecmp (argv[i], IRC_PLUGIN_NAME, 3) == 0))
        {
            if (!irc_server_alloc_with_url (argv[i]))
            {
                dogechat_printf (
                    NULL,
                    _("%s%s: unable to add temporary server \"%s\" (check "
                      "if there is already a server with this name)"),
                    dogechat_prefix ("error"), IRC_PLUGIN_NAME, argv[i]);
            }
        }
        else if (dogechat_strcasecmp (argv[i], "--upgrade") == 0)
        {
            upgrading = 1;
        }
    }

    if (upgrading)
    {
        if (!irc_upgrade_load ())
        {
            dogechat_printf (
                NULL,
                _("%s%s: WARNING: some network connections may still be "
                  "opened and not visible, you should restart DogeChat now "
                  "(with /quit)."),
                dogechat_prefix ("error"), IRC_PLUGIN_NAME);
        }
    }
    else
    {
        irc_server_auto_connect (auto_connect);
    }

    irc_hook_timer = dogechat_hook_timer (1 * 1000, 0, 0,
                                         &irc_server_timer_cb, NULL);

    return DOGECHAT_RC_OK;
}

/*
 * Ends IRC plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    if (irc_hook_timer)
        dogechat_unhook (irc_hook_timer);

    if (irc_signal_upgrade_received)
    {
        irc_config_write (1);
        irc_upgrade_save ();
    }
    else
    {
        irc_config_write (0);
        irc_server_disconnect_all ();
    }

    irc_ignore_free_all ();

    irc_raw_message_free_all ();

    irc_server_free_all ();

    irc_config_free ();

    irc_notify_end ();

    irc_redirect_end ();

    irc_color_end ();

    return DOGECHAT_RC_OK;
}
