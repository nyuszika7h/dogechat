/*
 * dogechat-lua.c - lua plugin for DogeChat
 *
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

#undef _

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "../dogechat-plugin.h"
#include "../plugin-script.h"
#include "dogechat-lua.h"
#include "dogechat-lua-api.h"


DOGECHAT_PLUGIN_NAME(LUA_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("Support of lua scripts"));
DOGECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(3000);

struct t_dogechat_plugin *dogechat_lua_plugin;

int lua_quiet = 0;
struct t_plugin_script *lua_scripts = NULL;
struct t_plugin_script *last_lua_script = NULL;
struct t_plugin_script *lua_current_script = NULL;
struct t_plugin_script *lua_registered_script = NULL;
const char *lua_current_script_filename = NULL;
lua_State *lua_current_interpreter = NULL;

/*
 * string used to execute action "install":
 * when signal "lua_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *lua_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "lua_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *lua_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "lua_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *lua_action_autoload_list = NULL;


/*
 * Callback called for each key/value in a hashtable.
 */

void
dogechat_lua_hashtable_map_cb (void *data,
                              struct t_hashtable *hashtable,
                              const char *key,
                              const char *value)
{
    lua_State *interpreter;

    /* make C compiler happy */
    (void) hashtable;

    interpreter = (lua_State *)data;

    lua_pushstring (interpreter, key);
    lua_pushstring (interpreter, value);
    lua_rawset (interpreter, -3);
}

/*
 * Converts a DogeChat hashtable to a lua hash (as lua table on the stack).
 */

void
dogechat_lua_pushhashtable (lua_State *interpreter, struct t_hashtable *hashtable)
{
    lua_newtable (interpreter);

    dogechat_hashtable_map_string (hashtable,
                                  &dogechat_lua_hashtable_map_cb,
                                  interpreter);
}

/*
 * Converts a lua hash (on stack) to a DogeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
dogechat_lua_tohashtable (lua_State *interpreter, int index, int size,
                         const char *type_keys, const char *type_values)
{
    struct t_hashtable *hashtable;

    hashtable = dogechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    lua_pushnil (interpreter);
    while (lua_next (interpreter, index - 1) != 0)
    {
        if (strcmp (type_values, DOGECHAT_HASHTABLE_STRING) == 0)
        {
            dogechat_hashtable_set (hashtable,
                                   lua_tostring (interpreter, -2),
                                   lua_tostring (interpreter, -1));
        }
        else if (strcmp (type_values, DOGECHAT_HASHTABLE_POINTER) == 0)
        {
            dogechat_hashtable_set (hashtable,
                                   lua_tostring (interpreter, -2),
                                   plugin_script_str2ptr (
                                       dogechat_lua_plugin,
                                       NULL, NULL,
                                       lua_tostring (interpreter, -1)));
        }
        /* remove value from stack (keep key for next iteration) */
        lua_pop (interpreter, 1);
    }

    return hashtable;
}

/*
 * Executes a lua function.
 */

void *
dogechat_lua_exec (struct t_plugin_script *script, int ret_type,
                  const char *function, const char *format, void **argv)
{
    void *ret_value;
    int argc, i, *ret_i;
    lua_State *old_lua_current_interpreter;
    struct t_plugin_script *old_lua_current_script;

    old_lua_current_interpreter = lua_current_interpreter;
    if (script->interpreter)
        lua_current_interpreter = script->interpreter;

    lua_getglobal (lua_current_interpreter, function);

    old_lua_current_script = lua_current_script;
    lua_current_script = script;

    argc = 0;
    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    lua_pushstring (lua_current_interpreter, (char *)argv[i]);
                    break;
                case 'i': /* integer */
                    lua_pushnumber (lua_current_interpreter, *((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    dogechat_lua_pushhashtable (lua_current_interpreter,
                                               (struct t_hashtable *)argv[i]);
                    break;
            }
        }
    }

    ret_value = NULL;

    if (lua_pcall (lua_current_interpreter, argc, 1, 0) == 0)
    {
        if (ret_type == DOGECHAT_SCRIPT_EXEC_STRING)
        {
            ret_value = strdup ((char *) lua_tostring (lua_current_interpreter, -1));
        }
        else if (ret_type == DOGECHAT_SCRIPT_EXEC_INT)
        {
            ret_i = malloc (sizeof (*ret_i));
            if (ret_i)
                *ret_i = lua_tonumber (lua_current_interpreter, -1);
            ret_value = ret_i;
        }
        else if (ret_type == DOGECHAT_SCRIPT_EXEC_HASHTABLE)
        {
            ret_value = dogechat_lua_tohashtable (lua_current_interpreter, -1,
                                                 DOGECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                 DOGECHAT_HASHTABLE_STRING,
                                                 DOGECHAT_HASHTABLE_STRING);
        }
        else
        {
            DOGECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, function);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to run function \"%s\""),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME, function);
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error: %s"),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME,
                        lua_tostring (lua_current_interpreter, -1));
    }

    lua_pop (lua_current_interpreter, 1);

    lua_current_script = old_lua_current_script;
    lua_current_interpreter = old_lua_current_interpreter;

    return ret_value;
}

/*
 * Adds a constant.
 */

void
dogechat_lua_add_constant (lua_State *L, struct t_lua_const *ptr_const)
{
    lua_pushstring (L, ptr_const->name);
    if (ptr_const->str_value)
        lua_pushstring (L, ptr_const->str_value);
    else
        lua_pushnumber (L, ptr_const->int_value);
    lua_settable(L, -3);
}

/*
 * Called when a constant is modified.
 */

int
dogechat_lua_newindex (lua_State *L)
{
    luaL_error(L, "Error: read-only constant");

    return 0;
}

/*
 * Registers a library to use inside lua script.
 */

void
dogechat_lua_register_lib (lua_State *L, const char *libname,
                          const luaL_Reg *lua_api_funcs,
                          struct t_lua_const lua_api_consts[])
{
    int i;

#if LUA_VERSION_NUM >= 502
    if (libname)
    {
        lua_newtable (L);
        luaL_setfuncs (L, lua_api_funcs, 0);
        lua_pushvalue (L, -1);
        lua_setglobal (L, libname);
    }
    else
        luaL_setfuncs (L, lua_api_funcs, 0);
#else
    luaL_register (L, libname, lua_api_funcs);
#endif /* LUA_VERSION_NUM >= 502 */

    luaL_newmetatable (L, "dogechat");
    lua_pushliteral (L, "__index");
    lua_newtable (L);

    for (i= 0; lua_api_consts[i].name; i++)
    {
        dogechat_lua_add_constant (L, &lua_api_consts[i]);
    }
    lua_settable (L, -3);

    lua_pushliteral (L, "__newindex");
    lua_pushcfunction (L, dogechat_lua_newindex);
    lua_settable (L, -3);

    lua_setmetatable (L, -2);
    lua_pop (L, 1);
}

/*
 * Loads a lua script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dogechat_lua_load (const char *filename)
{
    FILE *fp;
    char *dogechat_lua_code = {
        "dogechat_outputs = {\n"
        "    write = function (self, str)\n"
        "        dogechat.print(\"\", \"lua: stdout/stderr: \" .. str)\n"
        "    end\n"
        "}\n"
        "io.stdout = dogechat_outputs\n"
        "io.stderr = dogechat_outputs\n"
    };

    if ((fp = fopen (filename, "r")) == NULL)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not found"),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        return 0;
    }

    if ((dogechat_lua_plugin->debug >= 2) || !lua_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: loading script \"%s\""),
                        LUA_PLUGIN_NAME, filename);
    }

    lua_current_script = NULL;
    lua_registered_script = NULL;

    lua_current_interpreter = luaL_newstate();

    if (lua_current_interpreter == NULL)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME);
        fclose (fp);
        return 0;
    }

#ifdef LUA_VERSION_NUM /* LUA_VERSION_NUM is defined only in lua >= 5.1.0 */
    luaL_openlibs (lua_current_interpreter);
#else
    luaopen_base (lua_current_interpreter);
    luaopen_string (lua_current_interpreter);
    luaopen_table (lua_current_interpreter);
    luaopen_math (lua_current_interpreter);
    luaopen_io (lua_current_interpreter);
    luaopen_debug (lua_current_interpreter);
#endif /* LUA_VERSION_NUM */

    dogechat_lua_register_lib (lua_current_interpreter, "dogechat",
                              dogechat_lua_api_funcs,
                              dogechat_lua_api_consts);

#ifdef LUA_VERSION_NUM
    if (luaL_dostring (lua_current_interpreter, dogechat_lua_code) != 0)
#else
    if (lua_dostring (lua_current_interpreter, dogechat_lua_code) != 0)
#endif /* LUA_VERSION_NUM */
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to redirect stdout "
                                         "and stderr"),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME);
    }

    lua_current_script_filename = filename;

    if (luaL_loadfile (lua_current_interpreter, filename) != 0)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to load file \"%s\""),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error: %s"),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME,
                        lua_tostring (lua_current_interpreter, -1));
        lua_close (lua_current_interpreter);
        fclose (fp);
        return 0;
    }

    if (lua_pcall (lua_current_interpreter, 0, 0, 0) != 0)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to execute file "
                                         "\"%s\""),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error: %s"),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME,
                        lua_tostring (lua_current_interpreter, -1));
        lua_close (lua_current_interpreter);
        fclose (fp);

        /* if script was registered, remove it from list */
        if (lua_current_script)
        {
            plugin_script_remove (dogechat_lua_plugin,
                                  &lua_scripts, &last_lua_script,
                                  lua_current_script);
            lua_current_script = NULL;
        }

        return 0;
    }
    fclose (fp);

    if (!lua_registered_script)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        lua_close (lua_current_interpreter);
        return 0;
    }
    lua_current_script = lua_registered_script;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (dogechat_lua_plugin,
                                        lua_scripts,
                                        lua_current_script,
                                        &dogechat_lua_api_buffer_input_data_cb,
                                        &dogechat_lua_api_buffer_close_cb);

    (void) dogechat_hook_signal_send ("lua_script_loaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING,
                                     lua_current_script->filename);

    return 1;
}

/*
 * Callback for dogechat_script_auto_load() function.
 */

void
dogechat_lua_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    dogechat_lua_load (filename);
}

/*
 * Unloads a lua script.
 */

void
dogechat_lua_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((dogechat_lua_plugin->debug >= 2) || !lua_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: unloading script \"%s\""),
                        LUA_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)dogechat_lua_exec (script,
                                      DOGECHAT_SCRIPT_EXEC_INT,
                                      script->shutdown_func,
                                      NULL, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (lua_current_script == script)
        lua_current_script = (lua_current_script->prev_script) ?
            lua_current_script->prev_script : lua_current_script->next_script;

    plugin_script_remove (dogechat_lua_plugin, &lua_scripts, &last_lua_script, script);

    if (interpreter)
        lua_close (interpreter);

    if (lua_current_script)
        lua_current_interpreter = lua_current_script->interpreter;

    (void) dogechat_hook_signal_send ("lua_script_unloaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a lua script by name.
 */

void
dogechat_lua_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (dogechat_lua_plugin, lua_scripts, name);
    if (ptr_script)
    {
        dogechat_lua_unload (ptr_script);
        if (!lua_quiet)
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s: script \"%s\" unloaded"),
                            LUA_PLUGIN_NAME, name);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME, name);
    }
}

/*
 * Reloads a lua script by name.
 */

void
dogechat_lua_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (dogechat_lua_plugin, lua_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            dogechat_lua_unload (ptr_script);
            if (!lua_quiet)
            {
                dogechat_printf (NULL,
                                dogechat_gettext ("%s: script \"%s\" unloaded"),
                                LUA_PLUGIN_NAME, name);
            }
            dogechat_lua_load (filename);
            free (filename);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), LUA_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all lua scripts.
 */

void
dogechat_lua_unload_all ()
{
    while (lua_scripts)
    {
        dogechat_lua_unload (lua_scripts);
    }
}

/*
 * Callback for command "/lua".
 */

int
dogechat_lua_command_cb (void *data, struct t_gui_buffer *buffer,
                        int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (dogechat_lua_plugin, lua_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_lua_plugin, lua_scripts,
                                        NULL, 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_lua_plugin, lua_scripts,
                                        NULL, 1);
        }
        else if (dogechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (dogechat_lua_plugin, &dogechat_lua_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "reload") == 0)
        {
            dogechat_lua_unload_all ();
            plugin_script_auto_load (dogechat_lua_plugin, &dogechat_lua_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "unload") == 0)
        {
            dogechat_lua_unload_all ();
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }
    else
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_lua_plugin, lua_scripts,
                                        argv_eol[2], 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_lua_plugin, lua_scripts,
                                        argv_eol[2], 1);
        }
        else if ((dogechat_strcasecmp (argv[1], "load") == 0)
                 || (dogechat_strcasecmp (argv[1], "reload") == 0)
                 || (dogechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                lua_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (dogechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load lua script */
                path_script = plugin_script_search_path (dogechat_lua_plugin,
                                                         ptr_name);
                dogechat_lua_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (dogechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one lua script */
                dogechat_lua_reload_name (ptr_name);
            }
            else if (dogechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload lua script */
                dogechat_lua_unload_name (ptr_name);
            }
            lua_quiet = 0;
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds lua scripts to completion list.
 */

int
dogechat_lua_completion_cb (void *data, const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (dogechat_lua_plugin, completion, lua_scripts);

    return DOGECHAT_RC_OK;
}

/*
 * Returns hdata for lua scripts.
 */

struct t_hdata *
dogechat_lua_hdata_cb (void *data, const char *hdata_name)
{
    /* make C compiler happy */
    (void) data;

    return plugin_script_hdata_script (dogechat_plugin,
                                       &lua_scripts, &last_lua_script,
                                       hdata_name);
}

/*
 * Returns infolist with lua scripts.
 */

struct t_infolist *
dogechat_lua_infolist_cb (void *data, const char *infolist_name,
                         void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (dogechat_strcasecmp (infolist_name, "lua_script") == 0)
    {
        return plugin_script_infolist_list_scripts (dogechat_lua_plugin,
                                                    lua_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps lua plugin data in DogeChat log file.
 */

int
dogechat_lua_signal_debug_dump_cb (void *data, const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (dogechat_strcasecmp ((char *)signal_data, LUA_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (dogechat_lua_plugin, lua_scripts);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Display infos about external libraries used.
 */

int
dogechat_lua_signal_debug_libs_cb (void *data, const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

#ifdef LUA_VERSION
    dogechat_printf (NULL, "  %s: %s", LUA_PLUGIN_NAME, LUA_VERSION);
#else
    dogechat_printf (NULL, "  %s: (?)", LUA_PLUGIN_NAME);
#endif /* LUA_VERSION */

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a buffer is closed.
 */

int
dogechat_lua_signal_buffer_closed_cb (void *data, const char *signal,
                                     const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        plugin_script_remove_buffer_callbacks (lua_scripts, signal_data);

    return DOGECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
dogechat_lua_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &lua_action_install_list)
        {
            plugin_script_action_install (dogechat_lua_plugin,
                                          lua_scripts,
                                          &dogechat_lua_unload,
                                          &dogechat_lua_load,
                                          &lua_quiet,
                                          &lua_action_install_list);
        }
        else if (data == &lua_action_remove_list)
        {
            plugin_script_action_remove (dogechat_lua_plugin,
                                         lua_scripts,
                                         &dogechat_lua_unload,
                                         &lua_quiet,
                                         &lua_action_remove_list);
        }
        else if (data == &lua_action_autoload_list)
        {
            plugin_script_action_autoload (dogechat_lua_plugin,
                                           &lua_quiet,
                                           &lua_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
dogechat_lua_signal_script_action_cb (void *data, const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (strcmp (type_data, DOGECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "lua_script_install") == 0)
        {
            plugin_script_action_add (&lua_action_install_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_lua_timer_action_cb,
                                &lua_action_install_list);
        }
        else if (strcmp (signal, "lua_script_remove") == 0)
        {
            plugin_script_action_add (&lua_action_remove_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_lua_timer_action_cb,
                                &lua_action_remove_list);
        }
        else if (strcmp (signal, "lua_script_autoload") == 0)
        {
            plugin_script_action_add (&lua_action_autoload_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_lua_timer_action_cb,
                                &lua_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Initializes lua plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;

    dogechat_lua_plugin = plugin;

    init.callback_command = &dogechat_lua_command_cb;
    init.callback_completion = &dogechat_lua_completion_cb;
    init.callback_hdata = &dogechat_lua_hdata_cb;
    init.callback_infolist = &dogechat_lua_infolist_cb;
    init.callback_signal_debug_dump = &dogechat_lua_signal_debug_dump_cb;
    init.callback_signal_debug_libs = &dogechat_lua_signal_debug_libs_cb;
    init.callback_signal_buffer_closed = &dogechat_lua_signal_buffer_closed_cb;
    init.callback_signal_script_action = &dogechat_lua_signal_script_action_cb;
    init.callback_load_file = &dogechat_lua_load_cb;

    lua_quiet = 1;
    plugin_script_init (dogechat_lua_plugin, argc, argv, &init);
    lua_quiet = 0;

    plugin_script_display_short_list (dogechat_lua_plugin,
                                      lua_scripts);

    /* init OK */
    return DOGECHAT_RC_OK;
}

/*
 * Ends lua plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* unload all scripts */
    lua_quiet = 1;
    plugin_script_end (plugin, &lua_scripts, &dogechat_lua_unload_all);
    lua_quiet = 0;

    /* free some data */
    if (lua_action_install_list)
        free (lua_action_install_list);
    if (lua_action_remove_list)
        free (lua_action_remove_list);
    if (lua_action_autoload_list)
        free (lua_action_autoload_list);

    return DOGECHAT_RC_OK;
}
