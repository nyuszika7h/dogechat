/*
 * Copyright (C) 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
 * Copyright (C) 2008-2016 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef DOGECHAT_TCL_H
#define DOGECHAT_TCL_H 1

#define dogechat_plugin dogechat_tcl_plugin
#define TCL_PLUGIN_NAME "tcl"

#define TCL_CURRENT_SCRIPT_NAME ((tcl_current_script) ? tcl_current_script->name : "-")

extern struct t_dogechat_plugin *dogechat_tcl_plugin;

extern int tcl_quiet;
extern struct t_plugin_script *tcl_scripts;
extern struct t_plugin_script *last_tcl_script;
extern struct t_plugin_script *tcl_current_script;
extern struct t_plugin_script *tcl_registered_script;
extern const char *tcl_current_script_filename;

extern Tcl_Obj *dogechat_tcl_hashtable_to_dict (Tcl_Interp *interp,
                                               struct t_hashtable *hashtable);
extern struct t_hashtable *dogechat_tcl_dict_to_hashtable (Tcl_Interp *interp,
                                                          Tcl_Obj *dict,
                                                          int size,
                                                          const char *type_keys,
                                                          const char *type_values);
extern void *dogechat_tcl_exec (struct t_plugin_script *script,
                               int ret_type, const char *function,
                               const char *format, void **argv);

#endif /* DOGECHAT_TCL_H */
