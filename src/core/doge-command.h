/*
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2006 Emmanuel Bouthenot <kolter@openics.org>
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

#ifndef DOGECHAT_COMMAND_H
#define DOGECHAT_COMMAND_H 1

#define COMMAND_CALLBACK(__command)                                     \
    int                                                                 \
    command_##__command (void *data, struct t_gui_buffer *buffer,       \
                         int argc, char **argv, char **argv_eol)

/*
 * This macro is used to create an "empty" command in DogeChat core:
 * command does nothing, but plugins or scripts can catch it when it
 * is used by user, with dogechat_hook_command_run("/xxx", ...)
 * where "xxx" is command name.
 */
#define COMMAND_EMPTY(__command)                                        \
    int                                                                 \
    command_##__command (void *data, struct t_gui_buffer *buffer,       \
                         int argc, char **argv, char **argv_eol)        \
    {                                                                   \
        (void) data;                                                    \
        (void) buffer;                                                  \
        (void) argc;                                                    \
        (void) argv;                                                    \
        (void) argv_eol;                                                \
        return DOGECHAT_RC_OK;                                           \
    }

/*
 * macro to return error in case of missing arguments in callback of
 * hook_command
 */
#define COMMAND_MIN_ARGS(__min_args, __option)                          \
    if (argc < __min_args)                                              \
    {                                                                   \
        gui_chat_printf_date_tags (                                     \
            NULL, 0, GUI_FILTER_TAG_NO_FILTER,                          \
            _("%sToo few arguments for command \"%s%s%s\" "             \
              "(help on command: /help %s)"),                           \
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],                     \
            argv[0],                                                    \
            (__option && __option[0]) ? " " : "",                       \
            (__option && __option[0]) ? __option : "",                  \
            argv[0] + 1);                                               \
        return DOGECHAT_RC_ERROR;                                        \
    }

/* macro to return error in callback of hook_command */
#define COMMAND_ERROR                                                   \
    {                                                                   \
        gui_chat_printf_date_tags (                                     \
            NULL, 0, GUI_FILTER_TAG_NO_FILTER,                          \
            _("%sError with command \"%s\" "                            \
              "(help on command: /help %s)"),                           \
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],                     \
            argv_eol[0],                                                \
            argv[0] + 1);                                               \
        return DOGECHAT_RC_ERROR;                                        \
    }


struct t_gui_buffer;

extern int command_reload (void *data, struct t_gui_buffer *buffer,
                           int argc, char **argv, char **argv_eol);
extern void command_init ();
extern void command_startup (int plugins_loaded);
extern void command_version_display (struct t_gui_buffer *buffer,
                                     int send_to_buffer_as_input,
                                     int translated_string,
                                     int display_git_version);

#endif /* DOGECHAT_COMMAND_H */
