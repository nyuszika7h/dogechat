/*
 * exec-buffer.c - buffers with output of commands
 *
 * Copyright (C) 2014-2016 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../dogechat-plugin.h"
#include "exec.h"
#include "exec-buffer.h"
#include "exec-command.h"
#include "exec-config.h"


/*
 * Callback for user data in an exec buffer.
 */

int
exec_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                      const char *input_data)
{
    char **argv, **argv_eol;
    int argc;

    /* make C compiler happy */
    (void) data;

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        dogechat_buffer_close (buffer);
        return DOGECHAT_RC_OK;
    }

    argv = dogechat_string_split (input_data, " ", 0, 0, &argc);
    argv_eol = dogechat_string_split (input_data, " ", 1, 0, NULL);

    if (argv && argv_eol)
        exec_command_run (buffer, argc, argv, argv_eol, 0);

    if (argv)
        dogechat_string_free_split (argv);
    if (argv_eol)
        dogechat_string_free_split (argv_eol);

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when an exec buffer is closed.
 */

int
exec_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    const char *full_name;
    struct t_exec_cmd *ptr_exec_cmd;

    /* make C compiler happy */
    (void) data;

    /* kill any command whose output is on this buffer */
    full_name = dogechat_buffer_get_string (buffer, "full_name");
    for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
         ptr_exec_cmd = ptr_exec_cmd->next_cmd)
    {
        if (ptr_exec_cmd->hook
            && ptr_exec_cmd->buffer_full_name
            && (strcmp (ptr_exec_cmd->buffer_full_name, full_name) == 0))
        {
            dogechat_hook_set (ptr_exec_cmd->hook, "signal", "kill");
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Restore buffer callbacks (input and close) for buffers created by exec
 * plugin.
 */

void
exec_buffer_set_callbacks ()
{
    struct t_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_buffer;
    const char *plugin_name;

    ptr_infolist = dogechat_infolist_get ("buffer", NULL, "");
    if (ptr_infolist)
    {
        while (dogechat_infolist_next (ptr_infolist))
        {
            ptr_buffer = dogechat_infolist_pointer (ptr_infolist, "pointer");
            plugin_name = dogechat_infolist_string (ptr_infolist, "plugin_name");
            if (ptr_buffer && plugin_name
                && (strcmp (plugin_name, EXEC_PLUGIN_NAME) == 0))
            {
                dogechat_buffer_set_pointer (ptr_buffer, "close_callback",
                                            &exec_buffer_close_cb);
                dogechat_buffer_set_pointer (ptr_buffer, "input_callback",
                                            &exec_buffer_input_cb);
            }
        }
        dogechat_infolist_free (ptr_infolist);
    }
}

/*
 * Creates a new exec buffer for a command.
 */

struct t_gui_buffer *
exec_buffer_new (const char *name, int free_content, int clear_buffer,
                 int switch_to_buffer)
{
    struct t_gui_buffer *new_buffer;
    int buffer_type;

    new_buffer = dogechat_buffer_search (EXEC_PLUGIN_NAME, name);
    if (new_buffer)
    {
        buffer_type = dogechat_buffer_get_integer (new_buffer, "type");
        if (((buffer_type == 0) && free_content)
            || ((buffer_type == 1) && !free_content))
        {
            /* change the type of buffer */
            dogechat_buffer_set (new_buffer,
                                "type",
                                (free_content) ? "free" : "formatted");
        }
        goto end;
    }

    new_buffer = dogechat_buffer_new (name,
                                     &exec_buffer_input_cb, NULL,
                                     &exec_buffer_close_cb, NULL);

    /* failed to create buffer ? then return */
    if (!new_buffer)
        return NULL;

    if (free_content)
        dogechat_buffer_set (new_buffer, "type", "free");
    dogechat_buffer_set (new_buffer, "clear", "1");
    dogechat_buffer_set (new_buffer, "title", _("Executed commands"));
    dogechat_buffer_set (new_buffer, "localvar_set_type", "exec");
    dogechat_buffer_set (new_buffer, "localvar_set_no_log", "1");
    dogechat_buffer_set (new_buffer, "time_for_each_line", "0");
    dogechat_buffer_set (new_buffer, "input_get_unknown_commands", "0");

end:
    if (clear_buffer)
        dogechat_buffer_clear (new_buffer);
    if (switch_to_buffer)
        dogechat_buffer_set (new_buffer, "display", "1");

    return new_buffer;
}
