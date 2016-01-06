/*
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

#ifndef DOGECHAT_LIST_H
#define DOGECHAT_LIST_H 1

struct t_dogelist_item
{
    char *data;                        /* item data                         */
    void *user_data;                   /* pointer to user data              */
    struct t_dogelist_item *prev_item;  /* link to previous item             */
    struct t_dogelist_item *next_item;  /* link to next item                 */
};

struct t_dogelist
{
    struct t_dogelist_item *items;      /* items in list                     */
    struct t_dogelist_item *last_item;  /* last item in list                 */
    int size;                          /* number of items in list           */
};

extern struct t_dogelist *dogelist_new ();
extern struct t_dogelist_item *dogelist_add (struct t_dogelist *dogelist,
                                           const char *data, const char *where,
                                           void *user_data);
extern struct t_dogelist_item *dogelist_search (struct t_dogelist *dogelist,
                                              const char *data);
extern int dogelist_search_pos (struct t_dogelist *dogelist, const char *data);
extern struct t_dogelist_item *dogelist_casesearch (struct t_dogelist *dogelist,
                                                  const char *data);
extern int dogelist_casesearch_pos (struct t_dogelist *dogelist, const char *data);
extern struct t_dogelist_item *dogelist_get (struct t_dogelist *dogelist,
                                           int position);
extern void dogelist_set (struct t_dogelist_item *item, const char *value);
extern struct t_dogelist_item *dogelist_next (struct t_dogelist_item *item);
extern struct t_dogelist_item *dogelist_prev (struct t_dogelist_item *item);
extern const char *dogelist_string (struct t_dogelist_item *item);
extern int dogelist_size (struct t_dogelist *dogelist);
extern void dogelist_remove (struct t_dogelist *dogelist,
                            struct t_dogelist_item *item);
extern void dogelist_remove_all (struct t_dogelist *dogelist);
extern void dogelist_free (struct t_dogelist *dogelist);
extern void dogelist_print_log (struct t_dogelist *dogelist, const char *name);

#endif /* DOGECHAT_LIST_H */
