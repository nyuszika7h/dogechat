/*
 * dogechat-guile.c - guile (scheme) plugin for DogeChat
 *
 * Copyright (C) 2011-2016 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#undef _

#include <libguile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include "../dogechat-plugin.h"
#include "../plugin-script.h"
#include "dogechat-guile.h"
#include "dogechat-guile-api.h"


DOGECHAT_PLUGIN_NAME(GUILE_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("Support of scheme scripts (with Guile)"));
DOGECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(3000);

struct t_dogechat_plugin *dogechat_guile_plugin = NULL;

int guile_quiet;
struct t_plugin_script *guile_scripts = NULL;
struct t_plugin_script *last_guile_script = NULL;
struct t_plugin_script *guile_current_script = NULL;
struct t_plugin_script *guile_registered_script = NULL;
const char *guile_current_script_filename = NULL;
SCM guile_module_dogechat;
SCM guile_port;
char *guile_stdout = NULL;

struct t_guile_function
{
    SCM proc;                          /* proc to call                      */
    SCM *argv;                         /* arguments for proc                */
    size_t nargs;                      /* length of arguments               */
};

/*
 * string used to execute action "install":
 * when signal "guile_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *guile_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "guile_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *guile_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "guile_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *guile_action_autoload_list = NULL;


/*
 * Flushes stdout.
 */

void
dogechat_guile_stdout_flush ()
{
    if (guile_stdout)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: stdout/stderr: %s%s"),
                        GUILE_PLUGIN_NAME, guile_stdout, "");
        free (guile_stdout);
        guile_stdout = NULL;
    }
}

/*
 * Executes scheme procedure with internal catch and returns value.
 */

SCM
dogechat_guile_catch (void *procedure, void *data)
{
    SCM value;

    value = scm_internal_catch (SCM_BOOL_T,
                                (scm_t_catch_body)procedure,
                                data,
                                (scm_t_catch_handler) scm_handle_by_message_noexit,
                                NULL);
    return value;
}

/*
 * Encapsulates call to scm_call_n (to give arguments).
 */

SCM
dogechat_guile_scm_call_n (void *proc)
{
    struct t_guile_function *guile_function;

    guile_function = (struct t_guile_function *)proc;

    return scm_call_n (guile_function->proc,
                       guile_function->argv, guile_function->nargs);
}

/*
 * Executes scheme function (with optional args) and returns value.
 */

SCM
dogechat_guile_exec_function (const char *function, SCM *argv, size_t nargs)
{
    SCM func, func2, value;
    struct t_guile_function guile_function;

    func = dogechat_guile_catch (scm_c_lookup, (void *)function);
    func2 = dogechat_guile_catch (scm_variable_ref, func);

    if (argv)
    {
        guile_function.proc = func2;
        guile_function.argv = argv;
        guile_function.nargs = nargs;
        value = dogechat_guile_catch (dogechat_guile_scm_call_n, &guile_function);
    }
    else
    {
        value = dogechat_guile_catch (scm_call_0, func2);
    }

    return value;
}

/*
 * Callback called for each key/value in a hashtable.
 */

void
dogechat_guile_hashtable_map_cb (void *data,
                                struct t_hashtable *hashtable,
                                const char *key,
                                const char *value)
{
    SCM *alist, pair, list;

    /* make C compiler happy */
    (void) hashtable;

    alist = (SCM *)data;

    pair = scm_cons (scm_from_locale_string (key),
                     scm_from_locale_string (value));
    list = scm_list_1 (pair);

    *alist = scm_append (scm_list_2 (*alist, list));
}

/*
 * Converts a DogeChat hashtable to a guile alist.
 */

SCM
dogechat_guile_hashtable_to_alist (struct t_hashtable *hashtable)
{
    SCM alist;

    alist = scm_list_n (SCM_UNDEFINED);

    dogechat_hashtable_map_string (hashtable,
                                  &dogechat_guile_hashtable_map_cb,
                                  &alist);

    return alist;
}

/*
 * Converts a guile alist to a DogeChat hashtable.
 *
 * Note: hashtable must be free after use.
 */

struct t_hashtable *
dogechat_guile_alist_to_hashtable (SCM alist, int size, const char *type_keys,
                                  const char *type_values)
{
    struct t_hashtable *hashtable;
    int length, i;
    SCM pair;
    char *str, *str2;

    hashtable = dogechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    length = scm_to_int (scm_length (alist));
    for (i = 0; i < length; i++)
    {
        pair = scm_list_ref (alist, scm_from_int (i));
        if (strcmp (type_values, DOGECHAT_HASHTABLE_STRING) == 0)
        {
            str = scm_to_locale_string (scm_list_ref (pair, scm_from_int (0)));
            str2 = scm_to_locale_string (scm_list_ref (pair, scm_from_int (1)));
            dogechat_hashtable_set (hashtable, str, str2);
            if (str)
                free (str);
            if (str2)
                free (str2);
        }
        else if (strcmp (type_values, DOGECHAT_HASHTABLE_POINTER) == 0)
        {
            str = scm_to_locale_string (scm_list_ref (pair, scm_from_int (0)));
            str2 = scm_to_locale_string (scm_list_ref (pair, scm_from_int (1)));
            dogechat_hashtable_set (hashtable, str,
                                   plugin_script_str2ptr (dogechat_guile_plugin,
                                                          NULL, NULL, str2));
            if (str)
                free (str);
            if (str2)
                free (str2);
        }
    }

    return hashtable;
}

/*
 * Executes a guile function.
 */

void *
dogechat_guile_exec (struct t_plugin_script *script,
                    int ret_type, const char *function,
                    char *format, void **argv)
{
    struct t_plugin_script *old_guile_current_script;
    SCM rc, old_current_module;
    void *argv2[17], *ret_value;
    int i, argc, *ret_int;

    old_guile_current_script = guile_current_script;
    old_current_module = NULL;
    if (script->interpreter)
    {
        old_current_module = scm_current_module ();
        scm_set_current_module ((SCM)(script->interpreter));
    }
    guile_current_script = script;

    if (argv && argv[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    argv2[i] = scm_from_locale_string ((char *)argv[i]);
                    break;
                case 'i': /* integer */
                    argv2[i] = scm_from_int (*((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    argv2[i] = dogechat_guile_hashtable_to_alist (argv[i]);
                    break;
            }
        }
        for (i = argc; i < 17; i++)
        {
            argv2[i] = SCM_UNDEFINED;
        }
        rc = dogechat_guile_exec_function (function, (SCM *)argv2, argc);
    }
    else
    {
        rc = dogechat_guile_exec_function (function, NULL, 0);
    }

    ret_value = NULL;

    if ((ret_type == DOGECHAT_SCRIPT_EXEC_STRING) && (scm_is_string (rc)))
    {
        ret_value = scm_to_locale_string (rc);
    }
    else if ((ret_type == DOGECHAT_SCRIPT_EXEC_INT) && (scm_is_integer (rc)))
    {
        ret_int = malloc (sizeof (*ret_int));
        if (ret_int)
            *ret_int = scm_to_int (rc);
        ret_value = ret_int;
    }
    else if (ret_type == DOGECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        ret_value = dogechat_guile_alist_to_hashtable (rc,
                                                      DOGECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                      DOGECHAT_HASHTABLE_STRING,
                                                      DOGECHAT_HASHTABLE_STRING);
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"%s\" must return "
                                         "a valid value"),
                        dogechat_prefix ("error"), GUILE_PLUGIN_NAME, function);
    }

    if (ret_value == NULL)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error in function \"%s\""),
                        dogechat_prefix ("error"), GUILE_PLUGIN_NAME, function);
    }

    if (old_current_module)
        scm_set_current_module (old_current_module);

    guile_current_script = old_guile_current_script;

    return ret_value;
}

/*
 * Initializes guile module for script.
 */

void
dogechat_guile_module_init_script (void *data)
{
    SCM rc;

    dogechat_guile_catch (scm_c_eval_string, "(use-modules (dogechat))");
    rc = dogechat_guile_catch (scm_c_primitive_load, data);

    /* error loading script? */
    if (rc == SCM_BOOL_F)
    {
        /* if script was registered, remove it from list */
        if (guile_current_script)
        {
            plugin_script_remove (dogechat_guile_plugin,
                                  &guile_scripts, &last_guile_script,
                                  guile_current_script);
        }
        guile_current_script = NULL;
        guile_registered_script = NULL;
    }
}

/*
 * Loads a guile script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dogechat_guile_load (const char *filename)
{
    char *filename2, *ptr_base_name, *base_name;
    SCM module;

    if ((dogechat_guile_plugin->debug >= 2) || !guile_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: loading script \"%s\""),
                        GUILE_PLUGIN_NAME, filename);
    }

    guile_current_script = NULL;
    guile_registered_script = NULL;
    guile_current_script_filename = filename;

    filename2 = strdup (filename);
    if (!filename2)
        return 0;

    ptr_base_name = basename (filename2);
    base_name = strdup (ptr_base_name);
    module = scm_c_define_module (base_name,
                                  &dogechat_guile_module_init_script, filename2);
    free (filename2);

    if (!guile_registered_script)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        dogechat_prefix ("error"), GUILE_PLUGIN_NAME, filename);
        return 0;
    }

    dogechat_guile_catch (scm_gc_protect_object, (void *)module);

    guile_current_script = guile_registered_script;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (dogechat_guile_plugin,
                                        guile_scripts,
                                        guile_current_script,
                                        &dogechat_guile_api_buffer_input_data_cb,
                                        &dogechat_guile_api_buffer_close_cb);

    (void) dogechat_hook_signal_send ("guile_script_loaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING,
                                     guile_current_script->filename);

    return 1;
}

/*
 * Callback for script_auto_load() function.
 */

void
dogechat_guile_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    dogechat_guile_load (filename);
}

/*
 * Unloads a guile script.
 */

void
dogechat_guile_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((dogechat_guile_plugin->debug >= 2) || !guile_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: unloading script \"%s\""),
                        GUILE_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)dogechat_guile_exec (script, DOGECHAT_SCRIPT_EXEC_INT,
                                        script->shutdown_func, NULL, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (guile_current_script == script)
        guile_current_script = (guile_current_script->prev_script) ?
            guile_current_script->prev_script : guile_current_script->next_script;

    plugin_script_remove (dogechat_guile_plugin, &guile_scripts, &last_guile_script,
                          script);

    if (interpreter)
        dogechat_guile_catch (scm_gc_unprotect_object, interpreter);

    if (guile_current_script)
        scm_set_current_module ((SCM)(guile_current_script->interpreter));

    (void) dogechat_hook_signal_send ("guile_script_unloaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a guile script by name.
 */

void
dogechat_guile_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (dogechat_guile_plugin, guile_scripts, name);
    if (ptr_script)
    {
        dogechat_guile_unload (ptr_script);
        if (!guile_quiet)
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s: script \"%s\" unloaded"),
                            GUILE_PLUGIN_NAME, name);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), GUILE_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all guile scripts.
 */

void
dogechat_guile_unload_all ()
{
    while (guile_scripts)
    {
        dogechat_guile_unload (guile_scripts);
    }
}

/*
 * Reloads a guile script by name.
 */

void
dogechat_guile_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (dogechat_guile_plugin, guile_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            dogechat_guile_unload (ptr_script);
            if (!guile_quiet)
            {
                dogechat_printf (NULL,
                                dogechat_gettext ("%s: script \"%s\" unloaded"),
                                GUILE_PLUGIN_NAME, name);
            }
            dogechat_guile_load (filename);
            free (filename);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), GUILE_PLUGIN_NAME, name);
    }
}

/*
 * Callback for command "/guile".
 */

int
dogechat_guile_command_cb (void *data, struct t_gui_buffer *buffer,
                          int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;
    SCM value;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (dogechat_guile_plugin, guile_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_guile_plugin, guile_scripts,
                                        NULL, 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_guile_plugin, guile_scripts,
                                        NULL, 1);
        }
        else if (dogechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (dogechat_guile_plugin, &dogechat_guile_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "reload") == 0)
        {
            dogechat_guile_unload_all ();
            plugin_script_auto_load (dogechat_guile_plugin, &dogechat_guile_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "unload") == 0)
        {
            dogechat_guile_unload_all ();
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }
    else
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_guile_plugin, guile_scripts,
                                        argv_eol[2], 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_guile_plugin, guile_scripts,
                                        argv_eol[2], 1);
        }
        else if ((dogechat_strcasecmp (argv[1], "load") == 0)
                 || (dogechat_strcasecmp (argv[1], "reload") == 0)
                 || (dogechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                guile_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (dogechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load guile script */
                path_script = plugin_script_search_path (dogechat_guile_plugin,
                                                         ptr_name);
                dogechat_guile_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (dogechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one guile script */
                dogechat_guile_reload_name (ptr_name);
            }
            else if (dogechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload guile script */
                dogechat_guile_unload_name (ptr_name);
            }
            guile_quiet = 0;
        }
        else if (dogechat_strcasecmp (argv[1], "eval") == 0)
        {
            /* eval guile code */
            value = dogechat_guile_catch (scm_c_eval_string, argv_eol[2]);
            if (!SCM_EQ_P (value, SCM_UNDEFINED)
                && !SCM_EQ_P (value, SCM_UNSPECIFIED))
            {
                scm_display (value, guile_port);
            }
            dogechat_guile_stdout_flush ();
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds guile scripts to completion list.
 */

int
dogechat_guile_completion_cb (void *data, const char *completion_item,
                             struct t_gui_buffer *buffer,
                             struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (dogechat_guile_plugin, completion, guile_scripts);

    return DOGECHAT_RC_OK;
}

/*
 * Returns hdata for guile scripts.
 */

struct t_hdata *
dogechat_guile_hdata_cb (void *data, const char *hdata_name)
{
    /* make C compiler happy */
    (void) data;

    return plugin_script_hdata_script (dogechat_plugin,
                                       &guile_scripts, &last_guile_script,
                                       hdata_name);
}

/*
 * Returns infolist with guile scripts.
 */

struct t_infolist *
dogechat_guile_infolist_cb (void *data, const char *infolist_name,
                           void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (dogechat_strcasecmp (infolist_name, "guile_script") == 0)
    {
        return plugin_script_infolist_list_scripts (dogechat_guile_plugin,
                                                    guile_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps guile plugin data in DogeChat log file.
 */

int
dogechat_guile_signal_debug_dump_cb (void *data, const char *signal,
                                    const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (dogechat_strcasecmp ((char *)signal_data, GUILE_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (dogechat_guile_plugin, guile_scripts);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Display infos about external libraries used.
 */

int
dogechat_guile_signal_debug_libs_cb (void *data, const char *signal,
                                    const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

#if defined(SCM_MAJOR_VERSION) && defined(SCM_MINOR_VERSION) && defined(SCM_MICRO_VERSION)
    dogechat_printf (NULL, "  %s: %d.%d.%d",
                    GUILE_PLUGIN_NAME,
                    SCM_MAJOR_VERSION,
                    SCM_MINOR_VERSION,
                    SCM_MICRO_VERSION);
#else
    dogechat_printf (NULL, "  %s: (?)", GUILE_PLUGIN_NAME);
#endif /* defined(SCM_MAJOR_VERSION) && defined(SCM_MINOR_VERSION) && defined(SCM_MICRO_VERSION) */

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a buffer is closed.
 */

int
dogechat_guile_signal_buffer_closed_cb (void *data, const char *signal,
                                       const char *type_data,
                                       void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        plugin_script_remove_buffer_callbacks (guile_scripts, signal_data);

    return DOGECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
dogechat_guile_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &guile_action_install_list)
        {
            plugin_script_action_install (dogechat_guile_plugin,
                                          guile_scripts,
                                          &dogechat_guile_unload,
                                          &dogechat_guile_load,
                                          &guile_quiet,
                                          &guile_action_install_list);
        }
        else if (data == &guile_action_remove_list)
        {
            plugin_script_action_remove (dogechat_guile_plugin,
                                         guile_scripts,
                                         &dogechat_guile_unload,
                                         &guile_quiet,
                                         &guile_action_remove_list);
        }
        else if (data == &guile_action_autoload_list)
        {
            plugin_script_action_autoload (dogechat_guile_plugin,
                                           &guile_quiet,
                                           &guile_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
dogechat_guile_signal_script_action_cb (void *data, const char *signal,
                                       const char *type_data,
                                       void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (strcmp (type_data, DOGECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "guile_script_install") == 0)
        {
            plugin_script_action_add (&guile_action_install_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_guile_timer_action_cb,
                                &guile_action_install_list);
        }
        else if (strcmp (signal, "guile_script_remove") == 0)
        {
            plugin_script_action_add (&guile_action_remove_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_guile_timer_action_cb,
                                &guile_action_remove_list);
        }
        else if (strcmp (signal, "guile_script_autoload") == 0)
        {
            plugin_script_action_add (&guile_action_autoload_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_guile_timer_action_cb,
                                &guile_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Fills input.
 */

int
dogechat_guile_port_fill_input (SCM port)
{
    /* make C compiler happy */
    (void) port;

    return ' ';
}

/*
 * Write.
 */

void
dogechat_guile_port_write (SCM port, const void *data, size_t size)
{
    char *new_stdout;
    int length_stdout;

    /* make C compiler happy */
    (void) port;

    /* concatenate str to guile_stdout */
    if (guile_stdout)
    {
        length_stdout = strlen (guile_stdout);
        new_stdout = realloc (guile_stdout, length_stdout + size + 1);
        if (!new_stdout)
        {
            free (guile_stdout);
            return;
        }
        guile_stdout = new_stdout;
        memcpy (guile_stdout + length_stdout, data, size);
        guile_stdout[length_stdout + size] = '\0';
    }
    else
    {
        guile_stdout = malloc (size + 1);
        if (guile_stdout)
        {
            memcpy (guile_stdout, data, size);
            guile_stdout[size] = '\0';
        }
    }

    /* flush stdout if at least "\n" is in string */
    if (guile_stdout && (strchr (guile_stdout, '\n')))
        dogechat_guile_stdout_flush ();
}

/*
 * Initializes guile plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;

    dogechat_guile_plugin = plugin;

    guile_stdout = NULL;

#ifdef HAVE_GUILE_GMP_MEMORY_FUNCTIONS
    /*
     * prevent guile to use its own gmp allocator, because it can conflict
     * with other plugins using GnuTLS like relay, which can crash DogeChat
     * on unload (or exit)
     */
    scm_install_gmp_memory_functions = 0;
#endif /* HAVE_GUILE_GMP_MEMORY_FUNCTIONS */

    scm_init_guile ();

    guile_module_dogechat = scm_c_define_module ("dogechat",
                                                &dogechat_guile_api_module_init,
                                                NULL);
    scm_c_use_module ("dogechat");
    dogechat_guile_catch (scm_gc_protect_object, (void *)guile_module_dogechat);

    init.callback_command = &dogechat_guile_command_cb;
    init.callback_completion = &dogechat_guile_completion_cb;
    init.callback_hdata = &dogechat_guile_hdata_cb;
    init.callback_infolist = &dogechat_guile_infolist_cb;
    init.callback_signal_debug_dump = &dogechat_guile_signal_debug_dump_cb;
    init.callback_signal_debug_libs = &dogechat_guile_signal_debug_libs_cb;
    init.callback_signal_buffer_closed = &dogechat_guile_signal_buffer_closed_cb;
    init.callback_signal_script_action = &dogechat_guile_signal_script_action_cb;
    init.callback_load_file = &dogechat_guile_load_cb;

    guile_quiet = 1;
    plugin_script_init (dogechat_guile_plugin, argc, argv, &init);
    guile_quiet = 0;

    plugin_script_display_short_list (dogechat_guile_plugin,
                                      guile_scripts);

    /* init OK */
    return DOGECHAT_RC_OK;
}

/*
 * Ends guile plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* unload all scripts */
    guile_quiet = 1;
    plugin_script_end (plugin, &guile_scripts, &dogechat_guile_unload_all);
    guile_quiet = 0;

    /* unprotect module */
    dogechat_guile_catch (scm_gc_unprotect_object, (void *)guile_module_dogechat);

    /* free some data */
    if (guile_action_install_list)
        free (guile_action_install_list);
    if (guile_action_remove_list)
        free (guile_action_remove_list);
    if (guile_action_autoload_list)
        free (guile_action_autoload_list);

    return DOGECHAT_RC_OK;
}
