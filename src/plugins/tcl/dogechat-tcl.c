/*
 * dogechat-tcl.c - tcl plugin for DogeChat
 *
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

#undef _

#include <tcl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "../dogechat-plugin.h"
#include "../plugin-script.h"
#include "dogechat-tcl.h"
#include "dogechat-tcl-api.h"


DOGECHAT_PLUGIN_NAME(TCL_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("Support of tcl scripts"));
DOGECHAT_PLUGIN_AUTHOR("Dmitry Kobylin <fnfal@academ.tsc.ru>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(3000);

struct t_dogechat_plugin *dogechat_tcl_plugin = NULL;

int tcl_quiet = 0;
struct t_plugin_script *tcl_scripts = NULL;
struct t_plugin_script *last_tcl_script = NULL;
struct t_plugin_script *tcl_current_script = NULL;
struct t_plugin_script *tcl_registered_script = NULL;
const char *tcl_current_script_filename = NULL;

/*
 * string used to execute action "install":
 * when signal "tcl_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *tcl_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "tcl_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *tcl_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "tcl_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *tcl_action_autoload_list = NULL;

Tcl_Interp* cinterp;


/*
 * Callback called for each key/value in a hashtable.
 */

void
dogechat_tcl_hashtable_map_cb (void *data,
                              struct t_hashtable *hashtable,
                              const char *key,
                              const char *value)
{
    void **data_array;
    Tcl_Interp *interp;
    Tcl_Obj *dict;

    /* make C compiler happy */
    (void) hashtable;

    data_array = (void **)data;
    interp = data_array[0];
    dict = data_array[1];

    Tcl_DictObjPut (interp, dict,
                    Tcl_NewStringObj (key, -1),
                    Tcl_NewStringObj (value, -1));
}

/*
 * Converts a DogeChat hashtable to a tcl dict.
 */

Tcl_Obj *
dogechat_tcl_hashtable_to_dict (Tcl_Interp *interp,
                               struct t_hashtable *hashtable)
{
    Tcl_Obj *dict;
    void *data[2];

    dict = Tcl_NewDictObj ();
    if (!dict)
        return NULL;

    data[0] = interp;
    data[1] = dict;

    dogechat_hashtable_map_string (hashtable,
                                  &dogechat_tcl_hashtable_map_cb,
                                  data);

    return dict;
}

/*
 * Converts a tcl dict to a DogeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
dogechat_tcl_dict_to_hashtable (Tcl_Interp *interp, Tcl_Obj *dict,
                               int size, const char *type_keys,
                               const char *type_values)
{
    struct t_hashtable *hashtable;
    Tcl_DictSearch search;
    Tcl_Obj *key, *value;
    int done;

    hashtable = dogechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    if (Tcl_DictObjFirst (interp, dict, &search, &key, &value, &done) == TCL_OK)
    {
        for (; !done ; Tcl_DictObjNext(&search, &key, &value, &done))
        {
            if (strcmp (type_values, DOGECHAT_HASHTABLE_STRING) == 0)
            {
                dogechat_hashtable_set (hashtable,
                                       Tcl_GetString (key),
                                       Tcl_GetString (value));
            }
            else if (strcmp (type_values, DOGECHAT_HASHTABLE_POINTER) == 0)
            {
                dogechat_hashtable_set (hashtable,
                                       Tcl_GetString (key),
                                       plugin_script_str2ptr (
                                           dogechat_tcl_plugin,
                                           NULL, NULL,
                                           Tcl_GetString (value)));
            }
        }
    }

    Tcl_DictObjDone (&search);

    return hashtable;
}

/*
 * Executes a tcl function.
 */

void *
dogechat_tcl_exec (struct t_plugin_script *script,
                  int ret_type, const char *function,
                  const char *format, void **argv)
{
    int argc, i, llength;
    int *ret_i;
    char *ret_cv;
    void *ret_val;
    Tcl_Obj *cmdlist;
    Tcl_Interp *interp;
    struct t_plugin_script *old_tcl_script;

    old_tcl_script = tcl_current_script;
    tcl_current_script = script;
    interp = (Tcl_Interp*)script->interpreter;

    if (function && function[0])
    {
        cmdlist = Tcl_NewListObj (0, NULL);
        Tcl_IncrRefCount (cmdlist); /* +1 */
        Tcl_ListObjAppendElement (interp, cmdlist, Tcl_NewStringObj (function,-1));
    }
    else
    {
        tcl_current_script = old_tcl_script;
        return NULL;
    }

    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    Tcl_ListObjAppendElement (interp, cmdlist,
                                              Tcl_NewStringObj (argv[i], -1));
                    break;
                case 'i': /* integer */
                    Tcl_ListObjAppendElement (interp, cmdlist,
                                              Tcl_NewIntObj (*((int *)argv[i])));
                    break;
                case 'h': /* hash */
                    Tcl_ListObjAppendElement (interp, cmdlist,
                                              dogechat_tcl_hashtable_to_dict (interp, argv[i]));
                    break;
            }
        }
    }

    if (Tcl_ListObjLength (interp, cmdlist, &llength) != TCL_OK)
        llength = 0;

    if (Tcl_EvalObjEx (interp, cmdlist, TCL_EVAL_DIRECT) == TCL_OK)
    {
        Tcl_ListObjReplace (interp, cmdlist, 0, llength, 0, NULL); /* remove elements, decrement their ref count */
        Tcl_DecrRefCount (cmdlist); /* -1 */
        ret_val = NULL;
        if (ret_type == DOGECHAT_SCRIPT_EXEC_STRING)
        {
            ret_cv = Tcl_GetStringFromObj (Tcl_GetObjResult (interp), &i);
            if (ret_cv)
                ret_val = (void *)strdup (ret_cv);
            else
                ret_val = NULL;
        }
        else if ( ret_type == DOGECHAT_SCRIPT_EXEC_INT
                  && Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp), &i) == TCL_OK)
        {
            ret_i = (int *)malloc (sizeof (*ret_i));
            if (ret_i)
                *ret_i = i;
            ret_val = (void *)ret_i;
        }
        else if (ret_type == DOGECHAT_SCRIPT_EXEC_HASHTABLE)
        {
            ret_val = dogechat_tcl_dict_to_hashtable (interp,
                                                     Tcl_GetObjResult (interp),
                                                     DOGECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                     DOGECHAT_HASHTABLE_STRING,
                                                     DOGECHAT_HASHTABLE_STRING);
        }

        tcl_current_script = old_tcl_script;
        if (ret_val)
            return ret_val;

        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"%s\" must return a "
                                         "valid value"),
                        dogechat_prefix ("error"), TCL_PLUGIN_NAME, function);
        return NULL;
    }

    Tcl_ListObjReplace (interp, cmdlist, 0, llength, 0, NULL); /* remove elements, decrement their ref count */
    Tcl_DecrRefCount (cmdlist); /* -1 */
    dogechat_printf (NULL,
                    dogechat_gettext ("%s%s: unable to run function \"%s\": %s"),
                    dogechat_prefix ("error"), TCL_PLUGIN_NAME, function,
                    Tcl_GetStringFromObj (Tcl_GetObjResult (interp), &i));
    tcl_current_script = old_tcl_script;

    return NULL;
}

/*
 * Loads a tcl script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dogechat_tcl_load (const char *filename)
{
    int i;
    Tcl_Interp *interp;
    struct stat buf;

    if (stat (filename, &buf) != 0)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not found"),
                        dogechat_prefix ("error"), TCL_PLUGIN_NAME, filename);
        return 0;
    }

    if ((dogechat_tcl_plugin->debug >= 2) || !tcl_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: loading script \"%s\""),
                        TCL_PLUGIN_NAME, filename);
    }

    tcl_current_script = NULL;
    tcl_registered_script = NULL;

    if (!(interp = Tcl_CreateInterp ())) {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to create new "
                                         "interpreter"),
                        dogechat_prefix ("error"), TCL_PLUGIN_NAME);
        return 0;
    }
    tcl_current_script_filename = filename;

    dogechat_tcl_api_init (interp);

    if (Tcl_EvalFile (interp, filename) != TCL_OK)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error occurred while "
                                         "parsing file \"%s\": %s"),
                        dogechat_prefix ("error"), TCL_PLUGIN_NAME, filename,
                        Tcl_GetStringFromObj (Tcl_GetObjResult (interp), &i));

        /* if script was registered, remove it from list */
        if (tcl_current_script)
        {
            plugin_script_remove (dogechat_tcl_plugin,
                                  &tcl_scripts, &last_tcl_script,
                                  tcl_current_script);
            tcl_current_script = NULL;
        }

        return 0;
    }

    if (!tcl_registered_script)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        dogechat_prefix ("error"), TCL_PLUGIN_NAME, filename);
        Tcl_DeleteInterp (interp);
        return 0;
    }
    tcl_current_script = tcl_registered_script;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (dogechat_tcl_plugin,
                                        tcl_scripts,
                                        tcl_current_script,
                                        &dogechat_tcl_api_buffer_input_data_cb,
                                        &dogechat_tcl_api_buffer_close_cb);

    (void) dogechat_hook_signal_send ("tcl_script_loaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING,
                                     tcl_current_script->filename);

    return 1;
}

/*
 * Callback for dogechat_script_auto_load() function.
 */

void
dogechat_tcl_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    dogechat_tcl_load (filename);
}

/*
 * Unloads a tcl script.
 */

void
dogechat_tcl_unload (struct t_plugin_script *script)
{
    Tcl_Interp* interp;
    int *rc;
    char *filename;

    if ((dogechat_tcl_plugin->debug >= 2) || !tcl_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: unloading script \"%s\""),
                        TCL_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)dogechat_tcl_exec (script,
                                      DOGECHAT_SCRIPT_EXEC_INT,
                                      script->shutdown_func,
                                      NULL, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    interp = (Tcl_Interp*)script->interpreter;

    if (tcl_current_script == script)
        tcl_current_script = (tcl_current_script->prev_script) ?
            tcl_current_script->prev_script : tcl_current_script->next_script;

    plugin_script_remove (dogechat_tcl_plugin, &tcl_scripts, &last_tcl_script, script);

    Tcl_DeleteInterp(interp);

    (void) dogechat_hook_signal_send ("tcl_script_unloaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a tcl script by name.
 */

void
dogechat_tcl_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (dogechat_tcl_plugin, tcl_scripts, name);
    if (ptr_script)
    {
        dogechat_tcl_unload (ptr_script);
        if (!tcl_quiet)
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s: script \"%s\" unloaded"),
                            TCL_PLUGIN_NAME, name);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), TCL_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all tcl scripts.
 */

void
dogechat_tcl_unload_all ()
{
    while (tcl_scripts)
    {
        dogechat_tcl_unload (tcl_scripts);
    }
}

/*
 * Reloads a tcl script by name.
 */

void
dogechat_tcl_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (dogechat_tcl_plugin, tcl_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            dogechat_tcl_unload (ptr_script);
            if (!tcl_quiet)
            {
                dogechat_printf (NULL,
                                dogechat_gettext ("%s: script \"%s\" unloaded"),
                                TCL_PLUGIN_NAME, name);
            }
            dogechat_tcl_load (filename);
            free (filename);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), TCL_PLUGIN_NAME, name);
    }
}

/*
 * Callback for command "/tcl".
 */

int
dogechat_tcl_command_cb (void *data, struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (dogechat_tcl_plugin, tcl_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_tcl_plugin, tcl_scripts,
                                        NULL, 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_tcl_plugin, tcl_scripts,
                                        NULL, 1);
        }
        else if (dogechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (dogechat_tcl_plugin, &dogechat_tcl_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "reload") == 0)
        {
            dogechat_tcl_unload_all ();
            plugin_script_auto_load (dogechat_tcl_plugin, &dogechat_tcl_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "unload") == 0)
        {
            dogechat_tcl_unload_all ();
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }
    else
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_tcl_plugin, tcl_scripts,
                                        argv_eol[2], 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_tcl_plugin, tcl_scripts,
                                        argv_eol[2], 1);
        }
        else if ((dogechat_strcasecmp (argv[1], "load") == 0)
                 || (dogechat_strcasecmp (argv[1], "reload") == 0)
                 || (dogechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                tcl_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (dogechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load tcl script */
                path_script = plugin_script_search_path (dogechat_tcl_plugin,
                                                         ptr_name);
                dogechat_tcl_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (dogechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one tcl script */
                dogechat_tcl_reload_name (ptr_name);
            }
            else if (dogechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload tcl script */
                dogechat_tcl_unload_name (ptr_name);
            }
            tcl_quiet = 0;
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds tcl scripts to completion list.
 */

int
dogechat_tcl_completion_cb (void *data, const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (dogechat_tcl_plugin, completion, tcl_scripts);

    return DOGECHAT_RC_OK;
}

/*
 * Returns hdata for tcl scripts.
 */

struct t_hdata *
dogechat_tcl_hdata_cb (void *data, const char *hdata_name)
{
    /* make C compiler happy */
    (void) data;

    return plugin_script_hdata_script (dogechat_plugin,
                                       &tcl_scripts, &last_tcl_script,
                                       hdata_name);
}

/*
 * Returns infolist with tcl scripts.
 */

struct t_infolist *
dogechat_tcl_infolist_cb (void *data, const char *infolist_name,
                         void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (dogechat_strcasecmp (infolist_name, "tcl_script") == 0)
    {
        return plugin_script_infolist_list_scripts (dogechat_tcl_plugin,
                                                    tcl_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps tcl plugin data in DogeChat log file.
 */

int
dogechat_tcl_signal_debug_dump_cb (void *data, const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (dogechat_strcasecmp ((char *)signal_data, TCL_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (dogechat_tcl_plugin, tcl_scripts);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Display infos about external libraries used.
 */

int
dogechat_tcl_signal_debug_libs_cb (void *data, const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

#ifdef TCL_VERSION
    dogechat_printf (NULL, "  %s: %s", TCL_PLUGIN_NAME, TCL_VERSION);
#else
    dogechat_printf (NULL, "  %s: (?)", TCL_PLUGIN_NAME);
#endif /* TCL_VERSION */

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a buffer is closed.
 */

int
dogechat_tcl_signal_buffer_closed_cb (void *data, const char *signal,
                                     const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        plugin_script_remove_buffer_callbacks (tcl_scripts, signal_data);

    return DOGECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
dogechat_tcl_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &tcl_action_install_list)
        {
            plugin_script_action_install (dogechat_tcl_plugin,
                                          tcl_scripts,
                                          &dogechat_tcl_unload,
                                          &dogechat_tcl_load,
                                          &tcl_quiet,
                                          &tcl_action_install_list);
        }
        else if (data == &tcl_action_remove_list)
        {
            plugin_script_action_remove (dogechat_tcl_plugin,
                                         tcl_scripts,
                                         &dogechat_tcl_unload,
                                         &tcl_quiet,
                                         &tcl_action_remove_list);
        }
        else if (data == &tcl_action_autoload_list)
        {
            plugin_script_action_autoload (dogechat_tcl_plugin,
                                           &tcl_quiet,
                                           &tcl_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
dogechat_tcl_signal_script_action_cb (void *data, const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (strcmp (type_data, DOGECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "tcl_script_install") == 0)
        {
            plugin_script_action_add (&tcl_action_install_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_tcl_timer_action_cb,
                                &tcl_action_install_list);
        }
        else if (strcmp (signal, "tcl_script_remove") == 0)
        {
            plugin_script_action_add (&tcl_action_remove_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_tcl_timer_action_cb,
                                &tcl_action_remove_list);
        }
        else if (strcmp (signal, "tcl_script_autoload") == 0)
        {
            plugin_script_action_add (&tcl_action_autoload_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_tcl_timer_action_cb,
                                &tcl_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Initializes tcl plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;

    dogechat_tcl_plugin = plugin;

    init.callback_command = &dogechat_tcl_command_cb;
    init.callback_completion = &dogechat_tcl_completion_cb;
    init.callback_hdata = &dogechat_tcl_hdata_cb;
    init.callback_infolist = &dogechat_tcl_infolist_cb;
    init.callback_signal_debug_dump = &dogechat_tcl_signal_debug_dump_cb;
    init.callback_signal_debug_libs = &dogechat_tcl_signal_debug_libs_cb;
    init.callback_signal_buffer_closed = &dogechat_tcl_signal_buffer_closed_cb;
    init.callback_signal_script_action = &dogechat_tcl_signal_script_action_cb;
    init.callback_load_file = &dogechat_tcl_load_cb;

    tcl_quiet = 1;
    plugin_script_init (dogechat_tcl_plugin, argc, argv, &init);
    tcl_quiet = 0;

    plugin_script_display_short_list (dogechat_tcl_plugin,
                                      tcl_scripts);

    /* init OK */
    return DOGECHAT_RC_OK;
}

/*
 * Ends tcl plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* unload all scripts */
    tcl_quiet = 1;
    plugin_script_end (plugin, &tcl_scripts, &dogechat_tcl_unload_all);
    tcl_quiet = 0;

    /* free some data */
    if (tcl_action_install_list)
        free (tcl_action_install_list);
    if (tcl_action_remove_list)
        free (tcl_action_remove_list);
    if (tcl_action_autoload_list)
        free (tcl_action_autoload_list);

    return DOGECHAT_RC_OK;
}
