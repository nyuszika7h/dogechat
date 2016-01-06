/*
 * dogechat-aspell-bar-item.c - bar items for aspell plugin
 *
 * Copyright (C) 2012 Nils Görs <dogechatter@arcor.de>
 * Copyright (C) 2012-2016 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../dogechat-plugin.h"
#include "dogechat-aspell.h"
#include "dogechat-aspell-config.h"


/*
 * Returns content of bar item "aspell_dict": aspell dictionary used on current
 * buffer.
 */

char *
dogechat_aspell_bar_item_dict (void *data, struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    const char *dict_list;

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    dict_list = dogechat_aspell_get_dict (buffer);

    return (dict_list) ? strdup (dict_list) : NULL;
}

/*
 * Returns content of bar item "aspell_suggest": aspell suggestions.
 */

char *
dogechat_aspell_bar_item_suggest (void *data, struct t_gui_bar_item *item,
                                 struct t_gui_window *window,
                                 struct t_gui_buffer *buffer,
                                 struct t_hashtable *extra_info)
{
    const char *ptr_suggestions, *pos;
    char **suggestions, *suggestions2;
    int i, num_suggestions, length;

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!aspell_enabled)
        return NULL;

    if (!buffer)
        return NULL;

    ptr_suggestions = dogechat_buffer_get_string (buffer,
                                                 "localvar_aspell_suggest");
    if (!ptr_suggestions)
        return NULL;

    pos = strchr (ptr_suggestions, ':');
    if (pos)
        pos++;
    else
        pos = ptr_suggestions;
    suggestions = dogechat_string_split (pos, "/", 0, 0, &num_suggestions);
    if (suggestions)
    {
        length = 64 + 1;
        for (i = 0; i < num_suggestions; i++)
        {
            length += strlen (suggestions[i]) + 64;
        }
        suggestions2 = malloc (length);
        if (suggestions2)
        {
            suggestions2[0] = '\0';
            strcat (suggestions2,
                    dogechat_color (dogechat_config_string (dogechat_aspell_config_color_suggestions)));
            for (i = 0; i < num_suggestions; i++)
            {
                if (i > 0)
                {
                    strcat (suggestions2, dogechat_color ("bar_delim"));
                    strcat (suggestions2, "/");
                    strcat (suggestions2,
                            dogechat_color (dogechat_config_string (dogechat_aspell_config_color_suggestions)));
                }
                strcat (suggestions2, suggestions[i]);
            }
            dogechat_string_free_split (suggestions);
            return suggestions2;
        }
        dogechat_string_free_split (suggestions);
    }
    return strdup (pos);
}

/*
 * Initializes aspell bar items.
 */

void
dogechat_aspell_bar_item_init ()
{
    dogechat_bar_item_new ("aspell_dict", &dogechat_aspell_bar_item_dict, NULL);
    dogechat_bar_item_new ("aspell_suggest", &dogechat_aspell_bar_item_suggest, NULL);
}
