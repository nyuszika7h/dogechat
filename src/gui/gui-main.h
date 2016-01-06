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

#ifndef DOGECHAT_GUI_MAIN_H
#define DOGECHAT_GUI_MAIN_H 1

/* main functions (GUI dependent) */

extern void gui_main_get_password (const char **prompt,
                                   char *password, int size);
extern void gui_main_debug_libs ();
extern void gui_main_end (int clean_exit);

/* terminal functions (GUI dependent) */
extern void gui_term_set_eat_newline_glitch (int value);

#endif /* DOGECHAT_GUI_MAIN_H */
