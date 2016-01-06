/*
 * relay-dogechat-nicklist.c - nicklist functions for DogeChat protocol
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

#include "../../dogechat-plugin.h"
#include "../relay.h"
#include "relay-dogechat.h"
#include "relay-dogechat-nicklist.h"


/*
 * Builds a new nicklist structure (to store nicklist diffs).
 *
 * Returns pointer to new nicklist structure, NULL if error.
 */

struct t_relay_dogechat_nicklist *
relay_dogechat_nicklist_new ()
{
    struct t_relay_dogechat_nicklist *new_nicklist;

    new_nicklist = malloc (sizeof (*new_nicklist));
    if (!new_nicklist)
        return NULL;

    new_nicklist->nicklist_count = 0;
    new_nicklist->items_count = 0;
    new_nicklist->items = NULL;

    return new_nicklist;
}

/*
 * Adds a nicklist item in nicklist structure.
 */

void
relay_dogechat_nicklist_add_item (struct t_relay_dogechat_nicklist *nicklist,
                                 char diff, struct t_gui_nick_group *group,
                                 struct t_gui_nick *nick)
{
    struct t_relay_dogechat_nicklist_item *new_items, *ptr_item;
    struct t_hdata *hdata;
    const char *str;
    int i;

    /*
     * check if the last "parent_group" (with diff = '^') of items is the same
     * as this one: if yes, don't add this parent group
     */
    if ((diff == RELAY_DOGECHAT_NICKLIST_DIFF_PARENT)
        && (nicklist->items_count > 0))
    {
        for (i = nicklist->items_count - 1; i >= 0; i--)
        {
            if (nicklist->items[i].diff == RELAY_DOGECHAT_NICKLIST_DIFF_PARENT)
            {
                if (nicklist->items[i].pointer == group)
                    return;
                break;
            }
        }
    }

    new_items = realloc (nicklist->items,
                         (nicklist->items_count + 1) * sizeof (new_items[0]));
    if (!new_items)
        return;

    nicklist->items = new_items;
    ptr_item = &(nicklist->items[nicklist->items_count]);
    if (group)
    {
        hdata = dogechat_hdata_get ("nick_group");
        ptr_item->pointer = group;
    }
    else
    {
        hdata = dogechat_hdata_get ("nick");
        ptr_item->pointer = nick;
    }
    ptr_item->diff = diff;
    ptr_item->group = (group) ? 1 : 0;
    ptr_item->visible = dogechat_hdata_integer (hdata, ptr_item->pointer, "visible");
    ptr_item->level = (group) ? dogechat_hdata_integer (hdata, ptr_item->pointer, "level") : 0;
    str = dogechat_hdata_string (hdata, ptr_item->pointer, "name");
    ptr_item->name = (str) ? strdup (str) : NULL;
    str = dogechat_hdata_string (hdata, ptr_item->pointer, "color");
    ptr_item->color = (str) ? strdup (str) : NULL;
    str = dogechat_hdata_string (hdata, ptr_item->pointer, "prefix");
    ptr_item->prefix = (str) ? strdup (str) : NULL;
    str = dogechat_hdata_string (hdata, ptr_item->pointer, "prefix_color");
    ptr_item->prefix_color = (str) ? strdup (str) : NULL;

    nicklist->items_count++;
}

/*
 * Frees a nicklist_item structure.
 */

void
relay_dogechat_nicklist_item_free (struct t_relay_dogechat_nicklist_item *item)
{
    if (item->name)
        free (item->name);
    if (item->color)
        free (item->color);
    if (item->prefix)
        free (item->prefix);
    if (item->prefix_color)
        free (item->prefix_color);
}

/*
 * Frees a new nicklist structure.
 */

void
relay_dogechat_nicklist_free (struct t_relay_dogechat_nicklist *nicklist)
{
    int i;

    /* free items */
    if (nicklist->items_count > 0)
    {
        for (i = 0; i < nicklist->items_count; i++)
        {
            relay_dogechat_nicklist_item_free (&(nicklist->items[i]));
        }
        free (nicklist->items);
    }

    free (nicklist);
}
