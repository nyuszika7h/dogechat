/*
 * Copyright (C) 2003-2010 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_PYTHON_H
#define __WEECHAT_PYTHON_H 1

#define weechat_plugin weechat_python_plugin
#define PYTHON_PLUGIN_NAME "python"

#define PYTHON_CURRENT_SCRIPT_NAME ((python_current_script) ? python_current_script->name : "-")

extern struct t_weechat_plugin *weechat_python_plugin;

extern int python_quiet;
extern struct t_plugin_script *python_scripts;
extern struct t_plugin_script *last_python_script;
extern struct t_plugin_script *python_current_script;
extern struct t_plugin_script *python_registered_script;
extern const char *python_current_script_filename;

extern void *weechat_python_exec (struct t_plugin_script *script,
                                  int ret_type, const char *function,
                                  char **argv);

#endif /* weechat-perl.h */
