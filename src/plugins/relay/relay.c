/*
 * relay.c - network communication between DogeChat and remote client
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

#include "../dogechat-plugin.h"
#include "relay.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-command.h"
#include "relay-completion.h"
#include "relay-config.h"
#include "relay-info.h"
#include "relay-network.h"
#include "relay-raw.h"
#include "relay-server.h"
#include "relay-upgrade.h"


DOGECHAT_PLUGIN_NAME(RELAY_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("Relay DogeChat data to remote application "
                              "(irc/dogechat protocols)"));
DOGECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(4000);

struct t_dogechat_plugin *dogechat_relay_plugin = NULL;

int relay_signal_upgrade_received = 0; /* signal "upgrade" received ?       */

char *relay_protocol_string[] =        /* strings for protocols             */
{ "dogechat", "irc" };

struct t_hook *relay_hook_timer = NULL;


/*
 * Searches for a protocol.
 *
 * Returns index of protocol in enum t_relay_protocol, -1 if protocol is not
 * found.
 */

int
relay_protocol_search (const char *name)
{
    int i;

    for (i = 0; i < RELAY_NUM_PROTOCOLS; i++)
    {
        if (strcmp (relay_protocol_string[i], name) == 0)
            return i;
    }

    /* protocol not found */
    return -1;
}

/*
 * Callback for signal "upgrade".
 */

int
relay_signal_upgrade_cb (void *data, const char *signal, const char *type_data,
                         void *signal_data)
{
    struct t_relay_server *ptr_server;
    struct t_relay_client *ptr_client;
    int quit, ssl_disconnected;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    relay_signal_upgrade_received = 1;

    /* close socket for relay servers */
    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        relay_server_close_socket (ptr_server);
    }

    quit = (signal_data && (strcmp (signal_data, "quit") == 0));
    ssl_disconnected = 0;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        /*
         * FIXME: it's not possible to upgrade with SSL clients connected (GnuTLS
         * lib can't reload data after upgrade), so we close connection for
         * all SSL clients currently connected
         */
        if ((ptr_client->sock >= 0) && (ptr_client->ssl || quit))
        {
            if (!quit)
            {
                ssl_disconnected++;
                dogechat_printf (NULL,
                                _("%s%s: disconnecting from client %s%s%s because "
                                  "upgrade can't work for clients connected via SSL"),
                                dogechat_prefix ("error"),
                                RELAY_PLUGIN_NAME,
                                RELAY_COLOR_CHAT_CLIENT,
                                ptr_client->desc,
                                RELAY_COLOR_CHAT);
            }
            relay_client_set_status (ptr_client, RELAY_STATUS_DISCONNECTED);
        }
    }
    if (ssl_disconnected > 0)
    {
        dogechat_printf (NULL,
                        /* TRANSLATORS: "%s" after "%d" is "client" or "clients" */
                        _("%s%s: disconnected from %d %s (SSL connection "
                          "not supported with upgrade)"),
                        dogechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        ssl_disconnected,
                        NG_("client", "clients", ssl_disconnected));
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback for signal "debug_dump".
 */

int
relay_debug_dump_cb (void *data, const char *signal, const char *type_data,
                     void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (dogechat_strcasecmp ((char *)signal_data, RELAY_PLUGIN_NAME) == 0))
    {
        dogechat_log_printf ("");
        dogechat_log_printf ("***** \"%s\" plugin dump *****",
                            dogechat_plugin->name);

        relay_server_print_log ();
        relay_client_print_log ();

        dogechat_log_printf ("");
        dogechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            dogechat_plugin->name);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Initializes relay plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    int i, upgrading;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    dogechat_plugin = plugin;

    if (!relay_config_init ())
        return DOGECHAT_RC_ERROR;

    relay_config_read ();

    relay_network_init ();

    relay_command_init ();

    /* hook completions */
    relay_completion_init ();

    dogechat_hook_signal ("upgrade", &relay_signal_upgrade_cb, NULL);
    dogechat_hook_signal ("debug_dump", &relay_debug_dump_cb, NULL);

    relay_info_init ();

    /* look at arguments */
    upgrading = 0;
    for (i = 0; i < argc; i++)
    {
        if (dogechat_strcasecmp (argv[i], "--upgrade") == 0)
        {
            upgrading = 1;
        }
    }

    if (upgrading)
        relay_upgrade_load ();

    relay_hook_timer = dogechat_hook_timer (1 * 1000, 0, 0,
                                           &relay_client_timer_cb, NULL);

    return DOGECHAT_RC_OK;
}

/*
 * Ends relay plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    if (relay_hook_timer)
        dogechat_unhook (relay_hook_timer);

    relay_config_write ();

    if (relay_signal_upgrade_received)
        relay_upgrade_save ();
    else
    {
        relay_raw_message_free_all ();

        relay_server_free_all ();

        relay_client_disconnect_all ();

        if (relay_buffer)
            dogechat_buffer_close (relay_buffer);

        relay_client_free_all ();
    }

    relay_network_end ();

    relay_config_free ();

    return DOGECHAT_RC_OK;
}
