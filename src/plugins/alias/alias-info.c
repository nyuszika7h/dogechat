/*
 * alias-info.c - info and infolist hooks for alias plugin
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
#include "alias.h"


/*
 * Returns alias infolist "alias".
 */

struct t_infolist *
alias_info_infolist_alias_cb (void *data, const char *infolist_name,
                              void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) data;
    (void) infolist_name;
    (void) arguments;

    if (pointer && !alias_valid (pointer))
        return NULL;

    ptr_infolist = dogechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (pointer)
    {
        /* build list with only one alias */
        if (!alias_add_to_infolist (ptr_infolist, pointer))
        {
            dogechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all aliases matching arguments */
        for (ptr_alias = alias_list; ptr_alias;
             ptr_alias = ptr_alias->next_alias)
        {
            if (!arguments || !arguments[0]
                || dogechat_string_match (ptr_alias->name, arguments, 0))
            {
                if (!alias_add_to_infolist (ptr_infolist, ptr_alias))
                {
                    dogechat_infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }
    return NULL;
}

/*
 * Hooks infolist for alias plugin.
 */

void
alias_info_init ()
{
    dogechat_hook_infolist (
        "alias", N_("list of aliases"),
        N_("alias pointer (optional)"),
        N_("alias name (wildcard \"*\" is allowed) (optional)"),
        &alias_info_infolist_alias_cb, NULL);
}
