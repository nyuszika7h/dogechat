/*
 * alias-completion.c - completion for alias commands
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

#include "../dogechat-plugin.h"
#include "alias.h"


/*
 * Adds list of aliases to completion list.
 */

int
alias_completion_alias_cb (void *data, const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_alias = alias_list; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        dogechat_hook_completion_list_add (completion, ptr_alias->name,
                                          0, DOGECHAT_LIST_POS_SORT);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds value of an alias to completion list.
 */

int
alias_completion_alias_value_cb (void *data, const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    const char *args;
    char **argv, *alias_name;
    int argc;
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    args = dogechat_hook_completion_get_string (completion, "args");
    if (args)
    {
        argv = dogechat_string_split (args, " ", 0, 0, &argc);
        if (argv)
        {
            if (argc > 0)
                alias_name = strdup (argv[argc - 1]);
            else
                alias_name = strdup (args);

            if (alias_name)
            {
                ptr_alias = alias_search (alias_name);
                if (ptr_alias)
                {
                    dogechat_hook_completion_list_add (completion,
                                                      ptr_alias->command,
                                                      0,
                                                      DOGECHAT_LIST_POS_BEGINNING);
                }
                free (alias_name);
            }
            dogechat_string_free_split (argv);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
alias_completion_init ()
{
    dogechat_hook_completion ("alias", N_("list of aliases"),
                             &alias_completion_alias_cb, NULL);
    dogechat_hook_completion ("alias_value", N_("value of alias"),
                             &alias_completion_alias_value_cb, NULL);
}
