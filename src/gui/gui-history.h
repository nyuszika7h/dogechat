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

#ifndef DOGECHAT_GUI_HISTORY_H
#define DOGECHAT_GUI_HISTORY_H 1

struct t_gui_buffer;

struct t_gui_history
{
    char *text;                        /* text or command (entered by user) */
    struct t_gui_history *next_history;/* link to next text/command         */
    struct t_gui_history *prev_history;/* link to previous text/command     */
};

extern struct t_gui_history *gui_history;
extern struct t_gui_history *last_gui_history;
extern struct t_gui_history *gui_history_ptr;

extern void gui_history_buffer_add (struct t_gui_buffer *buffer,
                                    const char *string);
extern void gui_history_global_add (const char *string);
extern void gui_history_add (struct t_gui_buffer *buffer, const char *string);
extern void gui_history_global_free ();
extern void gui_history_buffer_free (struct t_gui_buffer *buffer);
extern struct t_hdata *gui_history_hdata_history_cb (void *data,
                                                     const char *hdata_name);
extern int gui_history_add_to_infolist (struct t_infolist *infolist,
                                        struct t_gui_history *history);

#endif /* DOGECHAT_GUI_HISTORY_H */
