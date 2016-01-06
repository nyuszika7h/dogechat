/*
 * doge-list.c - sorted lists
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "dogechat.h"
#include "doge-list.h"
#include "doge-log.h"
#include "doge-string.h"
#include "../plugins/plugin.h"


/*
 * Creates a new list.
 *
 * Returns pointer to new list, NULL if error.
 */

struct t_dogelist *
dogelist_new ()
{
    struct t_dogelist *new_dogelist;

    new_dogelist = malloc (sizeof (*new_dogelist));
    if (new_dogelist)
    {
        new_dogelist->items = NULL;
        new_dogelist->last_item = NULL;
        new_dogelist->size = 0;
    }
    return new_dogelist;
}

/*
 * Searches for position of data (to keep list sorted).
 */

struct t_dogelist_item *
dogelist_find_pos (struct t_dogelist *dogelist, const char *data)
{
    struct t_dogelist_item *ptr_item;

    if (!dogelist || !data)
        return NULL;

    for (ptr_item = dogelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (string_strcasecmp (data, ptr_item->data) < 0)
            return ptr_item;
    }
    /* position not found, best position is at the end */
    return NULL;
}

/*
 * Inserts an element in the list (keeping list sorted).
 */

void
dogelist_insert (struct t_dogelist *dogelist, struct t_dogelist_item *item,
                const char *where)
{
    struct t_dogelist_item *pos_item;

    if (!dogelist || !item)
        return;

    if (dogelist->items)
    {
        /* remove element if already in list */
        pos_item = dogelist_search (dogelist, item->data);
        if (pos_item)
            dogelist_remove (dogelist, pos_item);
    }

    if (dogelist->items)
    {
        /* search position for new element, according to pos asked */
        pos_item = NULL;
        if (string_strcasecmp (where, DOGECHAT_LIST_POS_BEGINNING) == 0)
            pos_item = dogelist->items;
        else if (string_strcasecmp (where, DOGECHAT_LIST_POS_END) == 0)
            pos_item = NULL;
        else
            pos_item = dogelist_find_pos (dogelist, item->data);

        if (pos_item)
        {
            /* insert data into the list (before position found) */
            item->prev_item = pos_item->prev_item;
            item->next_item = pos_item;
            if (pos_item->prev_item)
                (pos_item->prev_item)->next_item = item;
            else
                dogelist->items = item;
            pos_item->prev_item = item;
        }
        else
        {
            /* add data to the end */
            item->prev_item = dogelist->last_item;
            item->next_item = NULL;
            (dogelist->last_item)->next_item = item;
            dogelist->last_item = item;
        }
    }
    else
    {
        item->prev_item = NULL;
        item->next_item = NULL;
        dogelist->items = item;
        dogelist->last_item = item;
    }
}

/*
 * Creates new data and add it to the list.
 *
 * Returns pointer to new item, NULL if error.
 */

struct t_dogelist_item *
dogelist_add (struct t_dogelist *dogelist, const char *data, const char *where,
             void *user_data)
{
    struct t_dogelist_item *new_item;

    if (!dogelist || !data || !data[0] || !where || !where[0])
        return NULL;

    new_item = malloc (sizeof (*new_item));
    if (new_item)
    {
        new_item->data = strdup (data);
        new_item->user_data = user_data;
        dogelist_insert (dogelist, new_item, where);
        dogelist->size++;
    }
    return new_item;
}

/*
 * Searches for data in a list (case sensitive).
 *
 * Returns pointer to item found, NULL if not found.
 */

struct t_dogelist_item *
dogelist_search (struct t_dogelist *dogelist, const char *data)
{
    struct t_dogelist_item *ptr_item;

    if (!dogelist || !data)
        return NULL;

    for (ptr_item = dogelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (strcmp (data, ptr_item->data) == 0)
            return ptr_item;
    }
    /* data not found in list */
    return NULL;
}

/*
 * Searches for data in a list (case sensitive).
 *
 * Returns position of item found (>= 0), -1 if not found.
 */

int
dogelist_search_pos (struct t_dogelist *dogelist, const char *data)
{
    struct t_dogelist_item *ptr_item;
    int i;

    if (!dogelist || !data)
        return -1;

    i = 0;
    for (ptr_item = dogelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (strcmp (data, ptr_item->data) == 0)
            return i;
        i++;
    }
    /* data not found in list */
    return -1;
}

/*
 * Searches for data in a list (case insensitive).
 *
 * Returns pointer to item found, NULL if not found.
 */

struct t_dogelist_item *
dogelist_casesearch (struct t_dogelist *dogelist, const char *data)
{
    struct t_dogelist_item *ptr_item;

    if (!dogelist || !data)
        return NULL;

    for (ptr_item = dogelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (string_strcasecmp (data, ptr_item->data) == 0)
            return ptr_item;
    }
    /* data not found in list */
    return NULL;
}

/*
 * Searches for data in a list (case insensitive).
 *
 * Returns position of item found (>= 0), -1 if not found.
 */

int
dogelist_casesearch_pos (struct t_dogelist *dogelist, const char *data)
{
    struct t_dogelist_item *ptr_item;
    int i;

    if (!dogelist || !data)
        return -1;

    i = 0;
    for (ptr_item = dogelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (string_strcasecmp (data, ptr_item->data) == 0)
            return i;
        i++;
    }
    /* data not found in list */
    return -1;
}

/*
 * Gets an item in a list by position (0 is first element).
 */

struct t_dogelist_item *
dogelist_get (struct t_dogelist *dogelist, int position)
{
    int i;
    struct t_dogelist_item *ptr_item;

    if (!dogelist)
        return NULL;

    i = 0;
    ptr_item = dogelist->items;
    while (ptr_item)
    {
        if (i == position)
            return ptr_item;
        ptr_item = ptr_item->next_item;
        i++;
    }
    /* item not found */
    return NULL;
}

/*
 * Sets a new value for an item.
 */

void
dogelist_set (struct t_dogelist_item *item, const char *value)
{
    if (!item || !value)
        return;

    if (item->data)
        free (item->data);
    item->data = strdup (value);
}

/*
 * Gets next item.
 *
 * Returns NULL if end of list has been reached.
 */

struct t_dogelist_item *
dogelist_next (struct t_dogelist_item *item)
{
    if (item)
        return item->next_item;

    return NULL;
}

/*
 * Gets previous item.
 *
 * Returns NULL if beginning of list has been reached.
 */

struct t_dogelist_item *
dogelist_prev (struct t_dogelist_item *item)
{
    if (item)
        return item->prev_item;

    return NULL;
}

/*
 * Gets string pointer to item data.
 */

const char *
dogelist_string (struct t_dogelist_item *item)
{
    if (item)
        return item->data;

    return NULL;
}

/*
 * Gets size of list.
 */

int
dogelist_size (struct t_dogelist *dogelist)
{
    if (dogelist)
        return dogelist->size;

    return 0;
}

/*
 * Removes an item from a list.
 */

void
dogelist_remove (struct t_dogelist *dogelist, struct t_dogelist_item *item)
{
    struct t_dogelist_item *new_items;

    if (!dogelist || !item)
        return;

    /* remove item from list */
    if (dogelist->last_item == item)
        dogelist->last_item = item->prev_item;
    if (item->prev_item)
    {
        (item->prev_item)->next_item = item->next_item;
        new_items = dogelist->items;
    }
    else
        new_items = item->next_item;

    if (item->next_item)
        (item->next_item)->prev_item = item->prev_item;

    /* free data */
    if (item->data)
        free (item->data);
    free (item);
    dogelist->items = new_items;

    dogelist->size--;
}

/*
 * Removes all items from a list.
 */

void
dogelist_remove_all (struct t_dogelist *dogelist)
{
    if (!dogelist)
        return;

    while (dogelist->items)
    {
        dogelist_remove (dogelist, dogelist->items);
    }
}

/*
 * Frees a list.
 */

void
dogelist_free (struct t_dogelist *dogelist)
{
    if (!dogelist)
        return;

    dogelist_remove_all (dogelist);
    free (dogelist);
}

/*
 * Prints list in DogeChat log file (usually for crash dump).
 */

void
dogelist_print_log (struct t_dogelist *dogelist, const char *name)
{
    struct t_dogelist_item *ptr_item;
    int i;

    log_printf ("[dogelist %s (addr:0x%lx)]", name, dogelist);
    log_printf ("  items. . . . . . . . . : 0x%lx", dogelist->items);
    log_printf ("  last_item. . . . . . . : 0x%lx", dogelist->last_item);
    log_printf ("  size . . . . . . . . . : %d", dogelist->size);

    i = 0;
    for (ptr_item = dogelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        log_printf ("  [item %d (addr:0x%lx)]", i, ptr_item);
        log_printf ("    data . . . . . . . . : '%s'",  ptr_item->data);
        log_printf ("    user_data. . . . . . : 0x%lx", ptr_item->user_data);
        log_printf ("    prev_item. . . . . . : 0x%lx", ptr_item->prev_item);
        log_printf ("    next_item. . . . . . : 0x%lx", ptr_item->next_item);
        i++;
    }
}
