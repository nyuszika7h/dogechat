/*
 * fifo-info.c - info and infolist hooks for fifo plugin
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

#include "../dogechat-plugin.h"
#include "fifo.h"


/*
 * Returns FIFO info "fifo_filename".
 */

const char *
fifo_info_info_fifo_filename_cb (void *data, const char *info_name,
                                 const char *arguments)
{
    /* make C compiler happy */
    (void) data;
    (void) info_name;
    (void) arguments;

    return fifo_filename;
}

/*
 * Hooks info for fifo plugin.
 */

void
fifo_info_init ()
{
    dogechat_hook_info ("fifo_filename", N_("name of FIFO pipe"), NULL,
                       &fifo_info_info_fifo_filename_cb, NULL);
}
