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

#ifndef DOGECHAT_LOGGER_H
#define DOGECHAT_LOGGER_H 1

#define dogechat_plugin dogechat_logger_plugin
#define LOGGER_PLUGIN_NAME "logger"

#define LOGGER_LEVEL_DEFAULT 9

extern struct t_dogechat_plugin *dogechat_logger_plugin;

extern struct t_hook *logger_timer;

extern void logger_start_buffer_all (int write_info_line);
extern void logger_stop_all (int write_info_line);
extern void logger_adjust_log_filenames ();
extern int logger_timer_cb (void *data, int remaining_calls);

#endif /* DOGECHAT_LOGGER_H */
