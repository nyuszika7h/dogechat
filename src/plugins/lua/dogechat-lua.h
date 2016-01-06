/*
 * Copyright (C) 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2016 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef DOGECHAT_LUA_H
#define DOGECHAT_LUA_H 1

#define dogechat_plugin dogechat_lua_plugin
#define LUA_PLUGIN_NAME "lua"

#define LUA_CURRENT_SCRIPT_NAME ((lua_current_script) ? lua_current_script->name : "-")

struct t_lua_const
{
    char *name;
    int int_value;
    char *str_value;
};

extern struct t_dogechat_plugin *dogechat_lua_plugin;

extern int lua_quiet;
extern struct t_plugin_script *lua_scripts;
extern struct t_plugin_script *last_lua_script;
extern struct t_plugin_script *lua_current_script;
extern struct t_plugin_script *lua_registered_script;
extern const char *lua_current_script_filename;
extern lua_State *lua_current_interpreter;

extern void dogechat_lua_pushhashtable (lua_State *interpreter,
                                       struct t_hashtable *hashtable);
extern struct t_hashtable *dogechat_lua_tohashtable (lua_State *interpreter,
                                                    int index,
                                                    int size,
                                                    const char *type_keys,
                                                    const char *type_values);
extern void *dogechat_lua_exec (struct t_plugin_script *script,
                               int ret_type,
                               const char *function,
                               const char *format, void **argv);
extern void dogechat_lua_register_lib(lua_State *L, const char *libname,
                                     const luaL_Reg *lua_api_funcs,
                                     struct t_lua_const lua_api_consts[]);

#endif /* DOGECHAT_LUA_H */
