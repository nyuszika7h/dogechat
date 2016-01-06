/*
 * dogechat-aspell-info.c - info for aspell plugin
 *
 * Copyright (C) 2013-2016 Sébastien Helleu <flashcode@flashtux.org>
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
#include "dogechat-aspell.h"


/*
 * Returns aspell info "aspell_dict".
 */

const char *
dogechat_aspell_info_info_aspell_dict_cb (void *data, const char *info_name,
                                         const char *arguments)
{
    int rc;
    long unsigned int value;
    struct t_gui_buffer *buffer;
    const char *buffer_full_name;

    /* make C compiler happy */
    (void) data;
    (void) info_name;

    if (!arguments)
        return NULL;

    buffer_full_name = NULL;
    if (strncmp (arguments, "0x", 2) == 0)
    {
        rc = sscanf (arguments, "%lx", &value);
        if ((rc != EOF) && (rc != 0))
        {
            buffer = (struct t_gui_buffer *)value;
            if (buffer)
            {
                buffer_full_name = dogechat_buffer_get_string (buffer,
                                                              "full_name");
            }
        }
    }
    else
        buffer_full_name = arguments;

    if (buffer_full_name)
        return dogechat_aspell_get_dict_with_buffer_name (buffer_full_name);

    return NULL;
}

/*
 * Hooks info for aspell plugin.
 */

void
dogechat_aspell_info_init ()
{
    /* info hooks */
    dogechat_hook_info (
        "aspell_dict",
        N_("comma-separated list of dictionaries used in buffer"),
        N_("buffer pointer (\"0x12345678\") or buffer full name "
           "(\"irc.freenode.#dogechat\")"),
        &dogechat_aspell_info_info_aspell_dict_cb, NULL);
}
