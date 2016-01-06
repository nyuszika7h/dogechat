/*
 * logger-info.c - info and infolist hooks for logger plugin
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

#include "../dogechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"


/*
 * Returns logger infolist "logger_buffer".
 */

struct t_infolist *
logger_info_infolist_logger_buffer_cb (void *data, const char *infolist_name,
                                       void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_logger_buffer *ptr_logger_buffer;

    /* make C compiler happy */
    (void) data;
    (void) infolist_name;
    (void) arguments;

    if (pointer && !logger_buffer_valid (pointer))
        return NULL;

    ptr_infolist = dogechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (pointer)
    {
        /* build list with only one logger buffer */
        if (!logger_buffer_add_to_infolist (ptr_infolist, pointer))
        {
            dogechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all logger buffers */
        for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
             ptr_logger_buffer = ptr_logger_buffer->next_buffer)
        {
            if (!logger_buffer_add_to_infolist (ptr_infolist,
                                                ptr_logger_buffer))
            {
                dogechat_infolist_free (ptr_infolist);
                return NULL;
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Hooks infolist for logger plugin.
 */

void
logger_info_init ()
{
    dogechat_hook_infolist (
        "logger_buffer", N_("list of logger buffers"),
        N_("logger pointer (optional)"),
        NULL,
        &logger_info_infolist_logger_buffer_cb, NULL);
}
