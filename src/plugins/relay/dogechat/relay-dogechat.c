/*
 * relay-dogechat.c - DogeChat protocol for relay to client
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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>

#include "../../dogechat-plugin.h"
#include "../relay.h"
#include "relay-dogechat.h"
#include "relay-dogechat-nicklist.h"
#include "relay-dogechat-protocol.h"
#include "../relay-client.h"
#include "../relay-config.h"
#include "../relay-raw.h"


char *relay_dogechat_compression_string[] = /* strings for compressions      */
{ "off", "zlib" };


/*
 * Searches for a compression.
 *
 * Returns index of compression in enum t_relay_dogechat_compression, -1 if
 * compression is not found.
 */

int
relay_dogechat_compression_search (const char *compression)
{
    int i;

    for (i = 0; i < RELAY_DOGECHAT_NUM_COMPRESSIONS; i++)
    {
        if (dogechat_strcasecmp (relay_dogechat_compression_string[i], compression) == 0)
            return i;
    }

    /* compression not found */
    return -1;
}

/*
 * Hooks signals for a client.
 */

void
relay_dogechat_hook_signals (struct t_relay_client *client)
{
    RELAY_DOGECHAT_DATA(client, hook_signal_buffer) =
        dogechat_hook_signal ("buffer_*",
                             &relay_dogechat_protocol_signal_buffer_cb,
                             client);
    RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist) =
        dogechat_hook_hsignal ("nicklist_*",
                              &relay_dogechat_protocol_hsignal_nicklist_cb,
                              client);
    RELAY_DOGECHAT_DATA(client, hook_signal_upgrade) =
        dogechat_hook_signal ("upgrade*",
                             &relay_dogechat_protocol_signal_upgrade_cb,
                             client);
}

/*
 * Unhooks signals for a client.
 */

void
relay_dogechat_unhook_signals (struct t_relay_client *client)
{
    if (RELAY_DOGECHAT_DATA(client, hook_signal_buffer))
    {
        dogechat_unhook (RELAY_DOGECHAT_DATA(client, hook_signal_buffer));
        RELAY_DOGECHAT_DATA(client, hook_signal_buffer) = NULL;
    }
    if (RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist))
    {
        dogechat_unhook (RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist));
        RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist) = NULL;
    }
    if (RELAY_DOGECHAT_DATA(client, hook_signal_upgrade))
    {
        dogechat_unhook (RELAY_DOGECHAT_DATA(client, hook_signal_upgrade));
        RELAY_DOGECHAT_DATA(client, hook_signal_upgrade) = NULL;
    }
}

/*
 * Hooks timer to update nicklist.
 */

void
relay_dogechat_hook_timer_nicklist (struct t_relay_client *client)
{
    RELAY_DOGECHAT_DATA(client, hook_timer_nicklist) =
        dogechat_hook_timer (100, 0, 1,
                            &relay_dogechat_protocol_timer_nicklist_cb,
                            client);
}

/*
 * Reads data from a client.
 */

void
relay_dogechat_recv (struct t_relay_client *client, const char *data)
{
    relay_dogechat_protocol_recv (client, data);
}

/*
 * Closes connection with a client.
 */

void
relay_dogechat_close_connection (struct t_relay_client *client)
{
    relay_dogechat_unhook_signals (client);
}

/*
 * Frees a value of hashtable "buffers_nicklist".
 */

void
relay_dogechat_free_buffers_nicklist (struct t_hashtable *hashtable,
                                     const void *key, void *value)
{
    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    relay_dogechat_nicklist_free ((struct t_relay_dogechat_nicklist *)value);
}

/*
 * Initializes relay data specific to DogeChat protocol.
 */

void
relay_dogechat_alloc (struct t_relay_client *client)
{
    struct t_relay_dogechat_data *dogechat_data;
    char *password;

    password = dogechat_string_eval_expression (dogechat_config_string (relay_config_network_password),
                                               NULL, NULL, NULL);

    client->protocol_data = malloc (sizeof (*dogechat_data));
    if (client->protocol_data)
    {
        RELAY_DOGECHAT_DATA(client, password_ok) = (password && password[0]) ? 0 : 1;
        RELAY_DOGECHAT_DATA(client, compression) = RELAY_DOGECHAT_COMPRESSION_ZLIB;
        RELAY_DOGECHAT_DATA(client, buffers_sync) =
            dogechat_hashtable_new (32,
                                   DOGECHAT_HASHTABLE_STRING,
                                   DOGECHAT_HASHTABLE_INTEGER,
                                   NULL,
                                   NULL);
        RELAY_DOGECHAT_DATA(client, hook_signal_buffer) = NULL;
        RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist) = NULL;
        RELAY_DOGECHAT_DATA(client, hook_signal_upgrade) = NULL;
        RELAY_DOGECHAT_DATA(client, buffers_nicklist) =
            dogechat_hashtable_new (32,
                                   DOGECHAT_HASHTABLE_POINTER,
                                   DOGECHAT_HASHTABLE_POINTER,
                                   NULL,
                                   NULL);
        dogechat_hashtable_set_pointer (RELAY_DOGECHAT_DATA(client, buffers_nicklist),
                                       "callback_free_value",
                                       &relay_dogechat_free_buffers_nicklist);
        RELAY_DOGECHAT_DATA(client, hook_timer_nicklist) = NULL;

        relay_dogechat_hook_signals (client);
    }

    if (password)
        free (password);
}

/*
 * Initializes relay data specific to DogeChat protocol with an infolist.
 *
 * This is called after /upgrade.
 */

void
relay_dogechat_alloc_with_infolist (struct t_relay_client *client,
                                   struct t_infolist *infolist)
{
    struct t_relay_dogechat_data *dogechat_data;
    int index, value;
    char name[64];
    const char *key;

    client->protocol_data = malloc (sizeof (*dogechat_data));
    if (client->protocol_data)
    {
        /* general stuff */
        RELAY_DOGECHAT_DATA(client, password_ok) = dogechat_infolist_integer (infolist, "password_ok");
        RELAY_DOGECHAT_DATA(client, compression) = dogechat_infolist_integer (infolist, "compression");

        /* sync of buffers */
        RELAY_DOGECHAT_DATA(client, buffers_sync) = dogechat_hashtable_new (32,
                                                                          DOGECHAT_HASHTABLE_STRING,
                                                                          DOGECHAT_HASHTABLE_INTEGER,
                                                                          NULL,
                                                                          NULL);
        index = 0;
        while (1)
        {
            snprintf (name, sizeof (name), "buffers_sync_name_%05d", index);
            key = dogechat_infolist_string (infolist, name);
            if (!key)
                break;
            snprintf (name, sizeof (name), "buffers_sync_value_%05d", index);
            value = dogechat_infolist_integer (infolist, name);
            dogechat_hashtable_set (RELAY_DOGECHAT_DATA(client, buffers_sync),
                                   key,
                                   &value);
            index++;
        }
        RELAY_DOGECHAT_DATA(client, hook_signal_buffer) = NULL;
        RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist) = NULL;
        RELAY_DOGECHAT_DATA(client, hook_signal_upgrade) = NULL;
        RELAY_DOGECHAT_DATA(client, buffers_nicklist) =
            dogechat_hashtable_new (32,
                                   DOGECHAT_HASHTABLE_POINTER,
                                   DOGECHAT_HASHTABLE_POINTER,
                                   NULL,
                                   NULL);
        dogechat_hashtable_set_pointer (RELAY_DOGECHAT_DATA(client, buffers_nicklist),
                                       "callback_free_value",
                                       &relay_dogechat_free_buffers_nicklist);
        RELAY_DOGECHAT_DATA(client, hook_timer_nicklist) = NULL;

        if (RELAY_CLIENT_HAS_ENDED(client))
        {
            RELAY_DOGECHAT_DATA(client, hook_signal_buffer) = NULL;
            RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist) = NULL;
            RELAY_DOGECHAT_DATA(client, hook_signal_upgrade) = NULL;
        }
        else
            relay_dogechat_hook_signals (client);
    }
}

/*
 * Frees relay data specific to DogeChat protocol.
 */

void
relay_dogechat_free (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        if (RELAY_DOGECHAT_DATA(client, buffers_sync))
            dogechat_hashtable_free (RELAY_DOGECHAT_DATA(client, buffers_sync));
        if (RELAY_DOGECHAT_DATA(client, hook_signal_buffer))
            dogechat_unhook (RELAY_DOGECHAT_DATA(client, hook_signal_buffer));
        if (RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist))
            dogechat_unhook (RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist));
        if (RELAY_DOGECHAT_DATA(client, hook_signal_upgrade))
            dogechat_unhook (RELAY_DOGECHAT_DATA(client, hook_signal_upgrade));
        if (RELAY_DOGECHAT_DATA(client, buffers_nicklist))
            dogechat_hashtable_free (RELAY_DOGECHAT_DATA(client, buffers_nicklist));

        free (client->protocol_data);

        client->protocol_data = NULL;
    }
}

/*
 * Adds client DogeChat data in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_dogechat_add_to_infolist (struct t_infolist_item *item,
                               struct t_relay_client *client)
{
    if (!item || !client)
        return 0;

    if (!dogechat_infolist_new_var_integer (item, "password_ok", RELAY_DOGECHAT_DATA(client, password_ok)))
        return 0;
    if (!dogechat_infolist_new_var_integer (item, "compression", RELAY_DOGECHAT_DATA(client, compression)))
        return 0;
    if (!dogechat_hashtable_add_to_infolist (RELAY_DOGECHAT_DATA(client, buffers_sync), item, "buffers_sync"))
        return 0;

    return 1;
}

/*
 * Prints client DogeChat data in DogeChat log file (usually for crash dump).
 */

void
relay_dogechat_print_log (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        dogechat_log_printf ("    password_ok. . . . . . : %d",   RELAY_DOGECHAT_DATA(client, password_ok));
        dogechat_log_printf ("    compression. . . . . . : %d",   RELAY_DOGECHAT_DATA(client, compression));
        dogechat_log_printf ("    buffers_sync . . . . . : 0x%lx (hashtable: '%s')",
                            RELAY_DOGECHAT_DATA(client, buffers_sync),
                            dogechat_hashtable_get_string (RELAY_DOGECHAT_DATA(client, buffers_sync),
                                                          "keys_values"));
        dogechat_log_printf ("    hook_signal_buffer . . : 0x%lx", RELAY_DOGECHAT_DATA(client, hook_signal_buffer));
        dogechat_log_printf ("    hook_hsignal_nicklist. : 0x%lx", RELAY_DOGECHAT_DATA(client, hook_hsignal_nicklist));
        dogechat_log_printf ("    hook_signal_upgrade. . : 0x%lx", RELAY_DOGECHAT_DATA(client, hook_signal_upgrade));
        dogechat_log_printf ("    buffers_nicklist . . . : 0x%lx (hashtable: '%s')",
                            RELAY_DOGECHAT_DATA(client, buffers_nicklist),
                            dogechat_hashtable_get_string (RELAY_DOGECHAT_DATA(client, buffers_nicklist),
                                                          "keys_values"));
        dogechat_log_printf ("    hook_timer_nicklist. . : 0x%lx", RELAY_DOGECHAT_DATA(client, hook_timer_nicklist));
    }
}
