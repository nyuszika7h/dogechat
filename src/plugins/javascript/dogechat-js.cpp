/*
 * dogechat-js.cpp - javascript plugin for DogeChat
 *
 * Copyright (C) 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * Copyright (C) 2015-2016 Sébastien Helleu <flashcode@flashtux.org>
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

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C"
{
#include "../dogechat-plugin.h"
#include "../plugin-script.h"
}

#include "dogechat-js.h"
#include "dogechat-js-api.h"
#include "dogechat-js-v8.h"

DOGECHAT_PLUGIN_NAME(JS_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION("Support of javascript scripts");
DOGECHAT_PLUGIN_AUTHOR("Koka El Kiwi <kokakiwi@kokakiwi.net>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(3000);

struct t_dogechat_plugin *dogechat_js_plugin;

int js_quiet = 0;
struct t_plugin_script *js_scripts = NULL;
struct t_plugin_script *last_js_script = NULL;
struct t_plugin_script *js_current_script = NULL;
struct t_plugin_script *js_registered_script = NULL;
const char *js_current_script_filename = NULL;
DogechatJsV8 *js_current_interpreter = NULL;

/*
 * string used to execute action "install":
 * when signal "js_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *js_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "js_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *js_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "js_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *js_action_autoload_list = NULL;


/*
 * Callback called for each key/value in a hashtable.
 */

void
dogechat_js_hashtable_map_cb (void *data,
                             struct t_hashtable *hashtable,
                             const char *key,
                             const char *value)
{
    /* make C++ compiler happy */
    (void) hashtable;

    v8::Handle<v8::Object> *obj = (v8::Handle<v8::Object> *)data;

    (*obj)->Set(v8::String::New(key), v8::String::New(value));
}

/*
 * Converts a DogeChat hashtable to a javascript hashtable.
 */

v8::Handle<v8::Object>
dogechat_js_hashtable_to_object (struct t_hashtable *hashtable)
{
    v8::Handle<v8::Object> obj = v8::Object::New();

    dogechat_hashtable_map_string (hashtable,
                                  &dogechat_js_hashtable_map_cb,
                                  &obj);
    return obj;
}

/*
 * Converts a javascript hashtable to a DogeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
dogechat_js_object_to_hashtable (v8::Handle<v8::Object> obj,
                                int size,
                                const char *type_keys,
                                const char *type_values)
{
    struct t_hashtable *hashtable;
    unsigned int i;
    v8::Handle<v8::Array> keys;
    v8::Handle<v8::Value> key, value;

    hashtable = dogechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);

    if (!hashtable)
        return NULL;

    keys = obj->GetPropertyNames();
    for (i = 0; i < keys->Length(); i++)
    {
        key = keys->Get(i);
        value = obj->Get(key);
        v8::String::Utf8Value str_key(key);
        v8::String::Utf8Value str_value(value);
        if (strcmp (type_values, DOGECHAT_HASHTABLE_STRING) == 0)
        {
            dogechat_hashtable_set (hashtable, *str_key, *str_value);
        }
        else if (strcmp (type_values, DOGECHAT_HASHTABLE_POINTER) == 0)
        {
            dogechat_hashtable_set (hashtable, *str_key,
                                   plugin_script_str2ptr (dogechat_js_plugin,
                                                          NULL, NULL,
                                                          *str_value));
        }
    }

    return hashtable;
}

/*
 * Executes a javascript function.
 */

void *
dogechat_js_exec (struct t_plugin_script *script,
                 int ret_type, const char *function,
                 const char *format, void **argv)
{
    struct t_plugin_script *old_js_current_script;
    DogechatJsV8 *js_v8;
    void *ret_value;
    v8::Handle<v8::Value> argv2[16], ret_js;
    int i, argc, *ret_int;

    ret_value = NULL;

    old_js_current_script = js_current_script;
    js_current_script = script;
    js_v8 = (DogechatJsV8 *)(script->interpreter);

    if (!js_v8->functionExists(function))
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to run function \"%s\""),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME, function);
        goto end;
    }

    argc = 0;
    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    argv2[i] = v8::String::New((const char *)argv[i]);
                    break;
                case 'i': /* integer */
                    argv2[i] = v8::Integer::New(*((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    argv2[i] = dogechat_js_hashtable_to_object (
                        (struct t_hashtable *)argv[i]);
                    break;
            }
        }
    }

    ret_js = js_v8->execFunction(function,
                                 argc,
                                 (argc > 0) ? argv2 : NULL);

    if (!ret_js.IsEmpty())
    {
        if ((ret_type == DOGECHAT_SCRIPT_EXEC_STRING) && (ret_js->IsString()))
        {
            v8::String::Utf8Value temp_str(ret_js);
            ret_value = *temp_str;
        }
        else if ((ret_type == DOGECHAT_SCRIPT_EXEC_INT) && (ret_js->IsInt32()))
        {
            ret_int = (int *)malloc (sizeof (*ret_int));
            if (ret_int)
                *ret_int = (int)(ret_js->IntegerValue());
            ret_value = ret_int;
        }
        else if ((ret_type == DOGECHAT_SCRIPT_EXEC_HASHTABLE)
                 && (ret_js->IsObject()))
        {
            ret_value = (struct t_hashtable *)dogechat_js_object_to_hashtable (
                ret_js->ToObject(),
                DOGECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                DOGECHAT_HASHTABLE_STRING,
                DOGECHAT_HASHTABLE_STRING);
        }
        else
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s%s: function \"%s\" must "
                                             "return a valid value"),
                            dogechat_prefix ("error"), JS_PLUGIN_NAME,
                            function);
        }
    }

    if (!ret_value)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error in function \"%s\""),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME, function);
    }

end:
    js_current_script = old_js_current_script;

    return ret_value;
}

/*
 * Loads a javascript script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dogechat_js_load (const char *filename)
{
    char *source;

    source = dogechat_file_get_content (filename);
    if (!source)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not found"),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME, filename);
        return 0;
    }

    if ((dogechat_js_plugin->debug >= 2) || !js_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: loading script \"%s\""),
                        JS_PLUGIN_NAME, filename);
    }

    js_current_script = NULL;
    js_registered_script = NULL;

    js_current_interpreter = new DogechatJsV8();

    if (!js_current_interpreter)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME);
        free (source);
        return 0;
    }

    /* load libs */
    js_current_interpreter->loadLibs();

    js_current_script_filename = filename;

    if (!js_current_interpreter->load(source))
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to load file \"%s\""),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME);
        delete js_current_interpreter;
        free (source);

        /* if script was registered, remove it from list */
        if (js_current_script)
        {
            plugin_script_remove (dogechat_js_plugin,
                                  &js_scripts, &last_js_script,
                                  js_current_script);
            js_current_script = NULL;
        }

        return 0;
    }

    free (source);

    if (!js_current_interpreter->execScript())
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to execute file "
                                         "\"%s\""),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME, filename);
        delete js_current_interpreter;

        /* if script was registered, remove it from list */
        if (js_current_script)
        {
            plugin_script_remove (dogechat_js_plugin,
                                  &js_scripts, &last_js_script,
                                  js_current_script);
            js_current_script = NULL;
        }
        return 0;
    }

    if (!js_registered_script)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME, filename);
        delete js_current_interpreter;
        return 0;
    }

    js_current_script = js_registered_script;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (dogechat_js_plugin,
                                        js_scripts,
                                        js_current_script,
                                        &dogechat_js_api_buffer_input_data_cb,
                                        &dogechat_js_api_buffer_close_cb);

    dogechat_hook_signal_send ("javascript_script_loaded",
                              DOGECHAT_HOOK_SIGNAL_STRING,
                              js_current_script->filename);

    return 1;
}

/*
 * Callback for script_auto_load() function.
 */

void
dogechat_js_load_cb (void *data, const char *filename)
{
    /* make C++ compiler happy */
    (void) data;

    dogechat_js_load (filename);
}

/*
 * Unloads a javascript script.
 */

void
dogechat_js_unload (struct t_plugin_script *script)
{
    int *rc;
    char *filename;
    void *interpreter;

    if ((dogechat_js_plugin->debug >= 2) || !js_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: unloading script \"%s\""),
                        JS_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)dogechat_js_exec (script, DOGECHAT_SCRIPT_EXEC_INT,
                                     script->shutdown_func, NULL, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (js_current_script == script)
    {
        js_current_script = (js_current_script->prev_script) ?
            js_current_script->prev_script : js_current_script->next_script;
    }

    plugin_script_remove (dogechat_js_plugin,
                          &js_scripts, &last_js_script, script);

    if (interpreter)
        delete((DogechatJsV8 *)interpreter);

    (void) dogechat_hook_signal_send ("javascript_script_unloaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a javascript script by name.
 */

void
dogechat_js_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (dogechat_js_plugin, js_scripts, name);
    if (ptr_script)
    {
        dogechat_js_unload (ptr_script);
        if (!js_quiet)
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s: script \"%s\" unloaded"),
                            JS_PLUGIN_NAME, name);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all javascript scripts.
 */

void
dogechat_js_unload_all ()
{
    while (js_scripts)
    {
        dogechat_js_unload (js_scripts);
    }
}

/*
 * Reloads a javascript script by name.
 */

void
dogechat_js_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (dogechat_js_plugin, js_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            dogechat_js_unload (ptr_script);
            if (!js_quiet)
            {
                dogechat_printf (NULL,
                                dogechat_gettext ("%s: script \"%s\" unloaded"),
                                JS_PLUGIN_NAME, name);
            }
            dogechat_js_load (filename);
            free (filename);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), JS_PLUGIN_NAME, name);
    }
}

/*
 * Callback for command "/javascript".
 */

int
dogechat_js_command_cb (void *data, struct t_gui_buffer *buffer,
                       int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    /* make C++ compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (dogechat_js_plugin, js_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_js_plugin, js_scripts,
                                        NULL, 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_js_plugin, js_scripts,
                                        NULL, 1);
        }
        else if (dogechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (dogechat_js_plugin, &dogechat_js_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "reload") == 0)
        {
            dogechat_js_unload_all ();
            plugin_script_auto_load (dogechat_js_plugin, &dogechat_js_load_cb);
        }
        else if (dogechat_strcasecmp(argv[1], "unload"))
        {
            dogechat_js_unload_all ();
        }
    }
    else
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_js_plugin, js_scripts,
                                        argv_eol[2], 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_js_plugin, js_scripts,
                                        argv_eol[2], 1);
        }
        else if ((dogechat_strcasecmp (argv[1], "load") == 0)
                 || (dogechat_strcasecmp (argv[1], "reload") == 0)
                 || (dogechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                js_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (dogechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load javascript script */
                path_script = plugin_script_search_path (dogechat_js_plugin,
                                                         ptr_name);
                dogechat_js_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (dogechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one javascript script */
                dogechat_js_reload_name (ptr_name);
            }
            else if (dogechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload javascript script */
                dogechat_js_unload_name (ptr_name);
            }
            js_quiet = 0;
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds javascript scripts to completion list.
 */

int
dogechat_js_completion_cb (void *data, const char *completion_item,
                          struct t_gui_buffer *buffer,
                          struct t_gui_completion *completion)
{
    /* make C++ compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (dogechat_js_plugin, completion, js_scripts);

    return DOGECHAT_RC_OK;
}

/*
 * Returns hdata for javascript scripts.
 */

struct t_hdata *
dogechat_js_hdata_cb (void *data, const char *hdata_name)
{
    /* make C++ compiler happy */
    (void) data;

    return plugin_script_hdata_script (dogechat_plugin,
                                       &js_scripts, &last_js_script,
                                       hdata_name);
}

/*
 * Returns infolist with javascript scripts.
 */

struct t_infolist *
dogechat_js_infolist_cb (void *data, const char *infolist_name,
                        void *pointer, const char *arguments)
{
    /* make C++ compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (dogechat_strcasecmp (infolist_name, "javascript_script") == 0)
    {
        return plugin_script_infolist_list_scripts (dogechat_js_plugin,
                                                    js_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps javascript plugin data in Dogechat log file.
 */

int
dogechat_js_signal_debug_dump_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C++ compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (dogechat_strcasecmp ((char *)signal_data, JS_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (dogechat_js_plugin, js_scripts);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Display infos about external libraries used.
 */

int
dogechat_js_signal_debug_libs_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C++ compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    dogechat_printf (NULL, "  %s (v8): %s",
                    JS_PLUGIN_NAME, v8::V8::GetVersion());

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a buffer is closed.
 */

int
dogechat_js_signal_buffer_closed_cb (void *data, const char *signal,
                                    const char *type_data, void *signal_data)
{
    /* make C++ compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
    {
        plugin_script_remove_buffer_callbacks (js_scripts,
                                               (struct t_gui_buffer *)signal_data);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
dogechat_js_timer_action_cb (void *data, int remaining_calls)
{
    /* make C++ compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &js_action_install_list)
        {
            plugin_script_action_install (dogechat_js_plugin,
                                          js_scripts,
                                          &dogechat_js_unload,
                                          &dogechat_js_load,
                                          &js_quiet,
                                          &js_action_install_list);
        }
        else if (data == &js_action_remove_list)
        {
            plugin_script_action_remove (dogechat_js_plugin,
                                         js_scripts,
                                         &dogechat_js_unload,
                                         &js_quiet,
                                         &js_action_remove_list);
        }
        else if (data == &js_action_autoload_list)
        {
            plugin_script_action_autoload (dogechat_js_plugin,
                                           &js_quiet,
                                           &js_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove/autoload a
 * script).
 */

int
dogechat_js_signal_script_action_cb (void *data, const char *signal,
                                    const char *type_data,
                                    void *signal_data)
{
    /* make C++ compiler happy */
    (void) data;

    if (strcmp (type_data, DOGECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "javascript_script_install") == 0)
        {
            plugin_script_action_add (&js_action_install_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_js_timer_action_cb,
                                &js_action_install_list);
        }
        else if (strcmp (signal, "javascript_script_remove") == 0)
        {
            plugin_script_action_add (&js_action_remove_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_js_timer_action_cb,
                                &js_action_remove_list);
        }
        else if (strcmp (signal, "javascript_script_autoload") == 0)
        {
            plugin_script_action_add (&js_action_autoload_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_js_timer_action_cb,
                                &js_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Initializes javascript plugin.
 */

EXPORT int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;

    dogechat_js_plugin = plugin;

    init.callback_command = &dogechat_js_command_cb;
    init.callback_completion = &dogechat_js_completion_cb;
    init.callback_hdata = &dogechat_js_hdata_cb;
    init.callback_infolist = &dogechat_js_infolist_cb;
    init.callback_signal_debug_dump = &dogechat_js_signal_debug_dump_cb;
    init.callback_signal_debug_libs = &dogechat_js_signal_debug_libs_cb;
    init.callback_signal_buffer_closed = &dogechat_js_signal_buffer_closed_cb;
    init.callback_signal_script_action = &dogechat_js_signal_script_action_cb;
    init.callback_load_file = &dogechat_js_load_cb;

    js_quiet = 1;
    plugin_script_init (plugin, argc, argv, &init);
    js_quiet = 0;

    plugin_script_display_short_list (dogechat_js_plugin, js_scripts);

    return DOGECHAT_RC_OK;
}

/*
 * Ends javascript plugin.
 */

EXPORT int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    js_quiet = 1;
    plugin_script_end (plugin, &js_scripts, &dogechat_js_unload_all);
    js_quiet = 0;

    /* free some data */
    if (js_action_install_list)
        free (js_action_install_list);
    if (js_action_remove_list)
        free (js_action_remove_list);
    if (js_action_autoload_list)
        free (js_action_autoload_list);

    return DOGECHAT_RC_OK;
}
