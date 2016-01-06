/*
 * dogechat-perl.c - perl plugin for DogeChat
 *
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2008 Emmanuel Bouthenot <kolter@openics.org>
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

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include "../dogechat-plugin.h"
#include "../plugin-script.h"
#include "dogechat-perl.h"
#include "dogechat-perl-api.h"


DOGECHAT_PLUGIN_NAME(PERL_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("Support of perl scripts"));
DOGECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(3000);

struct t_dogechat_plugin *dogechat_perl_plugin = NULL;

int perl_quiet = 0;
struct t_plugin_script *perl_scripts = NULL;
struct t_plugin_script *last_perl_script = NULL;
struct t_plugin_script *perl_current_script = NULL;
struct t_plugin_script *perl_registered_script = NULL;
const char *perl_current_script_filename = NULL;
#ifdef MULTIPLICITY
PerlInterpreter *perl_current_interpreter = NULL;
#endif /* MULTIPLICITY */
int perl_quit_or_upgrade = 0;

/*
 * string used to execute action "install":
 * when signal "perl_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *perl_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "perl_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *perl_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "perl_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *perl_action_autoload_list = NULL;

#ifdef NO_PERL_MULTIPLICITY
#undef MULTIPLICITY
#endif /* NO_PERL_MULTIPLICITY */

#ifndef MULTIPLICITY
#define PKG_NAME_PREFIX "DogechatPerlPackage"
static PerlInterpreter *perl_main = NULL;
int perl_num = 0;
#endif /* MULTIPLICITY */

char *perl_args[] = { "", "-e", "0", "-w", NULL };
int perl_args_count = 4;

char *perl_dogechat_code =
{
#ifndef MULTIPLICITY
    "package %s;"
#endif /* MULTIPLICITY */
    "$SIG{__WARN__} = sub { dogechat::print('', 'perl\twarning: '.$_[0]) };"
    "$SIG{__DIE__} = sub { dogechat::print('', 'perl\terror: '.$_[0]) };"
    "do '%s';"
};

/*
 * Callback called for each key/value in a hashtable.
 */

void
dogechat_perl_hashtable_map_cb (void *data,
                               struct t_hashtable *hashtable,
                               const char *key,
                               const char *value)
{
    HV *hash;

    /* make C compiler happy */
    (void) hashtable;

    hash = (HV *)data;

    (void) hv_store (hash, key, strlen (key), newSVpv (value, 0), 0);
}

/*
 * Converts a DogeChat hashtable to a perl hash.
 */

HV *
dogechat_perl_hashtable_to_hash (struct t_hashtable *hashtable)
{
    HV *hash;

    hash = (HV *)newHV ();
    if (!hash)
        return NULL;

    dogechat_hashtable_map_string (hashtable,
                                  &dogechat_perl_hashtable_map_cb,
                                  hash);

    return hash;
}

/*
 * Converts a perl hash to a DogeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
dogechat_perl_hash_to_hashtable (SV *hash, int size, const char *type_keys,
                                const char *type_values)
{
    struct t_hashtable *hashtable;
    HV *hash2;
    SV *value;
    char *str_key;
    I32 retlen;

    hashtable = dogechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    if ((hash) && SvROK(hash) && SvRV(hash)
        && (SvTYPE(SvRV(hash)) == SVt_PVHV))
    {
        hash2 = (HV *)SvRV(hash);
        hv_iterinit (hash2);
        while ((value = hv_iternextsv (hash2, &str_key, &retlen)))
        {
            if (strcmp (type_values, DOGECHAT_HASHTABLE_STRING) == 0)
            {
                dogechat_hashtable_set (hashtable, str_key,
                                       SvPV (value, PL_na));
            }
            else if (strcmp (type_values, DOGECHAT_HASHTABLE_POINTER) == 0)
            {
                dogechat_hashtable_set (hashtable, str_key,
                                       plugin_script_str2ptr (
                                           dogechat_perl_plugin,
                                           NULL, NULL,
                                           SvPV (value, PL_na)));
            }
        }
    }

    return hashtable;
}

/*
 * Executes a perl function.
 */

void *
dogechat_perl_exec (struct t_plugin_script *script,
                   int ret_type, const char *function,
                   const char *format, void **argv)
{
    char *func;
    unsigned int count;
    void *ret_value;
    int *ret_i, mem_err, length, i, argc;
    SV *ret_s;
    HV *hash;
    struct t_plugin_script *old_perl_current_script;
#ifdef MULTIPLICITY
    void *old_context;
#endif /* MULTIPLICITY */

    old_perl_current_script = perl_current_script;
    perl_current_script = script;

#ifdef MULTIPLICITY
    (void) length;
    func = (char *) function;
    old_context = PERL_GET_CONTEXT;
    if (script->interpreter)
        PERL_SET_CONTEXT (script->interpreter);
#else
    length = strlen ((script->interpreter) ? script->interpreter : perl_main) +
        strlen (function) + 3;
    func = (char *) malloc (length);
    if (!func)
        return NULL;
    snprintf (func, length, "%s::%s",
              (char *) ((script->interpreter) ? script->interpreter : perl_main),
              function);
#endif /* MULTIPLICITY */

    dSP;
    ENTER;
    SAVETMPS;

    PUSHMARK(SP);
    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    XPUSHs(sv_2mortal(newSVpv((char *)argv[i], 0)));
                    break;
                case 'i': /* integer */
                    XPUSHs(sv_2mortal(newSViv(*((int *)argv[i]))));
                    break;
                case 'h': /* hash */
                    hash = dogechat_perl_hashtable_to_hash (argv[i]);
                    XPUSHs(sv_2mortal(newRV_inc((SV *)hash)));
                    break;
            }
        }
    }
    PUTBACK;
    count = call_pv (func, G_EVAL | G_SCALAR);

    ret_value = NULL;
    mem_err = 1;

    SPAGAIN;

    if (SvTRUE (ERRSV))
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error: %s"),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME,
                        SvPV_nolen (ERRSV));
        (void) POPs; /* poping the 'undef' */
        mem_err = 0;
    }
    else
    {
        if (count != 1)
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s%s: function \"%s\" must "
                                             "return one valid value (%d)"),
                            dogechat_prefix ("error"), PERL_PLUGIN_NAME,
                            function, count);
            mem_err = 0;
        }
        else
        {
            if (ret_type == DOGECHAT_SCRIPT_EXEC_STRING)
            {
                ret_s = newSVsv(POPs);
                ret_value = strdup (SvPV_nolen (ret_s));
                SvREFCNT_dec (ret_s);
            }
            else if (ret_type == DOGECHAT_SCRIPT_EXEC_INT)
            {
                ret_i = malloc (sizeof (*ret_i));
                if (ret_i)
                    *ret_i = POPi;
                ret_value = ret_i;
            }
            else if (ret_type == DOGECHAT_SCRIPT_EXEC_HASHTABLE)
            {
                ret_value = dogechat_perl_hash_to_hashtable (POPs,
                                                            DOGECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                            DOGECHAT_HASHTABLE_STRING,
                                                            DOGECHAT_HASHTABLE_STRING);
            }
            else
            {
                dogechat_printf (NULL,
                                dogechat_gettext ("%s%s: function \"%s\" is "
                                                 "internally misused"),
                                dogechat_prefix ("error"), PERL_PLUGIN_NAME,
                                function);
                mem_err = 0;
            }
        }
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    perl_current_script = old_perl_current_script;
#ifdef MULTIPLICITY
    PERL_SET_CONTEXT (old_context);
#else
    free (func);
#endif /* MULTIPLICITY */

    if (!ret_value && (mem_err == 1))
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: not enough memory in function "
                                         "\"%s\""),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME, function);
        return NULL;
    }

    return ret_value;
}

/*
 * Loads a perl script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dogechat_perl_load (const char *filename)
{
    struct t_plugin_script temp_script;
    struct stat buf;
    char *perl_code;
    int length;
#ifndef MULTIPLICITY
    char pkgname[64];
#endif /* MULTIPLICITY */

    temp_script.filename = NULL;
    temp_script.interpreter = NULL;
    temp_script.name = NULL;
    temp_script.author = NULL;
    temp_script.version = NULL;
    temp_script.license = NULL;
    temp_script.description = NULL;
    temp_script.shutdown_func = NULL;
    temp_script.charset = NULL;

    if (stat (filename, &buf) != 0)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not found"),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME, filename);
        return 0;
    }

    if ((dogechat_perl_plugin->debug >= 2) || !perl_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: loading script \"%s\""),
                        PERL_PLUGIN_NAME, filename);
    }

    perl_current_script = NULL;
    perl_current_script_filename = filename;
    perl_registered_script = NULL;

#ifdef MULTIPLICITY
    perl_current_interpreter = perl_alloc();

    if (!perl_current_interpreter)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME);
        return 0;
    }

    PERL_SET_CONTEXT (perl_current_interpreter);
    perl_construct (perl_current_interpreter);
    temp_script.interpreter = (PerlInterpreter *) perl_current_interpreter;
    perl_parse (perl_current_interpreter, dogechat_perl_api_init,
                perl_args_count, perl_args, NULL);
    length = strlen (perl_dogechat_code) - 2 + strlen (filename) + 1;
    perl_code = malloc (length);
    if (!perl_code)
        return 0;
    snprintf (perl_code, length, perl_dogechat_code, filename);
#else
    snprintf (pkgname, sizeof (pkgname), "%s%d", PKG_NAME_PREFIX, perl_num);
    perl_num++;
    length = strlen (perl_dogechat_code) - 4 + strlen (pkgname) + strlen (filename) + 1;
    perl_code = malloc (length);
    if (!perl_code)
        return 0;
    snprintf (perl_code, length, perl_dogechat_code, pkgname, filename);
#endif /* MULTIPLICITY */
    eval_pv (perl_code, TRUE);
    free (perl_code);

    if (SvTRUE (ERRSV))
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to parse file "
                                         "\"%s\""),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME,
                        filename);
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error: %s"),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME,
                        SvPV_nolen(ERRSV));
#ifdef MULTIPLICITY
        perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif /* MULTIPLICITY */
        if (perl_current_script && (perl_current_script != &temp_script))
        {
            plugin_script_remove (dogechat_perl_plugin,
                                  &perl_scripts, &last_perl_script,
                                  perl_current_script);
            perl_current_script = NULL;
        }

        return 0;
    }

    if (!perl_registered_script)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME, filename);
#ifdef MULTIPLICITY
        perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif /* MULTIPLICITY */
        return 0;
    }
    perl_current_script = perl_registered_script;

#ifndef MULTIPLICITY
    perl_current_script->interpreter = strdup (pkgname);
#endif /* MULTIPLICITY */

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (dogechat_perl_plugin,
                                        perl_scripts,
                                        perl_current_script,
                                        &dogechat_perl_api_buffer_input_data_cb,
                                        &dogechat_perl_api_buffer_close_cb);

    (void) dogechat_hook_signal_send ("perl_script_loaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING,
                                     perl_current_script->filename);

    return 1;
}

/*
 * Callback for dogechat_script_auto_load() function.
 */

void
dogechat_perl_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    dogechat_perl_load (filename);
}

/*
 * Unloads a perl script.
 */

void
dogechat_perl_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((dogechat_perl_plugin->debug >= 2) || !perl_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: unloading script \"%s\""),
                        PERL_PLUGIN_NAME, script->name);
    }

#ifdef MULTIPLICITY
    PERL_SET_CONTEXT (script->interpreter);
#endif /* MULTIPLICITY */

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)dogechat_perl_exec (script,
                                       DOGECHAT_SCRIPT_EXEC_INT,
                                       script->shutdown_func,
                                       NULL, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (perl_current_script == script)
    {
        perl_current_script = (perl_current_script->prev_script) ?
            perl_current_script->prev_script : perl_current_script->next_script;
    }

    plugin_script_remove (dogechat_perl_plugin, &perl_scripts, &last_perl_script,
                          script);

#ifdef MULTIPLICITY
    if (interpreter)
    {
        perl_destruct (interpreter);
        perl_free (interpreter);
    }
    if (perl_current_script)
    {
        PERL_SET_CONTEXT (perl_current_script->interpreter);
    }
#else
    if (interpreter)
        free (interpreter);
#endif /* MULTIPLICITY */

    (void) dogechat_hook_signal_send ("perl_script_unloaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a perl script by name.
 */

void
dogechat_perl_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (dogechat_perl_plugin, perl_scripts, name);
    if (ptr_script)
    {
        dogechat_perl_unload (ptr_script);
        if (!perl_quiet)
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s: script \"%s\" unloaded"),
                            PERL_PLUGIN_NAME, name);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all perl scripts.
 */

void
dogechat_perl_unload_all ()
{
    while (perl_scripts)
    {
        dogechat_perl_unload (perl_scripts);
    }
}

/*
 * Reloads a perl script by name.
 */

void
dogechat_perl_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (dogechat_perl_plugin, perl_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            dogechat_perl_unload (ptr_script);
            if (!perl_quiet)
            {
                dogechat_printf (NULL,
                                dogechat_gettext ("%s: script \"%s\" unloaded"),
                                PERL_PLUGIN_NAME, name);
            }
            dogechat_perl_load (filename);
            free (filename);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME, name);
    }
}

/*
 * Callback for command "/perl".
 */

int
dogechat_perl_command_cb (void *data, struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (dogechat_perl_plugin, perl_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_perl_plugin, perl_scripts,
                                        NULL, 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_perl_plugin, perl_scripts,
                                        NULL, 1);
        }
        else if (dogechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (dogechat_perl_plugin, &dogechat_perl_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "reload") == 0)
        {
            dogechat_perl_unload_all ();
            plugin_script_auto_load (dogechat_perl_plugin, &dogechat_perl_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "unload") == 0)
        {
            dogechat_perl_unload_all ();
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }
    else
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_perl_plugin, perl_scripts,
                                        argv_eol[2], 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_perl_plugin, perl_scripts,
                                        argv_eol[2], 1);
        }
        else if ((dogechat_strcasecmp (argv[1], "load") == 0)
                 || (dogechat_strcasecmp (argv[1], "reload") == 0)
                 || (dogechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                perl_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (dogechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load perl script */
                path_script = plugin_script_search_path (dogechat_perl_plugin,
                                                         ptr_name);
                dogechat_perl_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (dogechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one perl script */
                dogechat_perl_reload_name (ptr_name);
            }
            else if (dogechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload perl script */
                dogechat_perl_unload_name (ptr_name);
            }
            perl_quiet = 0;
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds perl scripts to completion list.
 */

int
dogechat_perl_completion_cb (void *data, const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (dogechat_perl_plugin, completion, perl_scripts);

    return DOGECHAT_RC_OK;
}

/*
 * Returns hdata for perl scripts.
 */

struct t_hdata *
dogechat_perl_hdata_cb (void *data, const char *hdata_name)
{
    /* make C compiler happy */
    (void) data;

    return plugin_script_hdata_script (dogechat_plugin,
                                       &perl_scripts, &last_perl_script,
                                       hdata_name);
}

/*
 * Returns infolist with perl scripts.
 */

struct t_infolist *
dogechat_perl_infolist_cb (void *data, const char *infolist_name,
                          void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (dogechat_strcasecmp (infolist_name, "perl_script") == 0)
    {
        return plugin_script_infolist_list_scripts (dogechat_perl_plugin,
                                                    perl_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps perl plugin data in DogeChat log file.
 */

int
dogechat_perl_signal_debug_dump_cb (void *data, const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (dogechat_strcasecmp ((char *)signal_data, PERL_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (dogechat_perl_plugin, perl_scripts);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Display infos about external libraries used.
 */

int
dogechat_perl_signal_debug_libs_cb (void *data, const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

#ifdef PERL_VERSION_STRING
    dogechat_printf (NULL, "  %s: %s", PERL_PLUGIN_NAME, PERL_VERSION_STRING);
#else
    dogechat_printf (NULL, "  %s: (?)", PERL_PLUGIN_NAME);
#endif /* PERL_VERSION_STRING */

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a buffer is closed.
 */

int
dogechat_perl_signal_buffer_closed_cb (void *data, const char *signal,
                                      const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        plugin_script_remove_buffer_callbacks (perl_scripts, signal_data);

    return DOGECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
dogechat_perl_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &perl_action_install_list)
        {
            plugin_script_action_install (dogechat_perl_plugin,
                                          perl_scripts,
                                          &dogechat_perl_unload,
                                          &dogechat_perl_load,
                                          &perl_quiet,
                                          &perl_action_install_list);
        }
        else if (data == &perl_action_remove_list)
        {
            plugin_script_action_remove (dogechat_perl_plugin,
                                         perl_scripts,
                                         &dogechat_perl_unload,
                                         &perl_quiet,
                                         &perl_action_remove_list);
        }
        else if (data == &perl_action_autoload_list)
        {
            plugin_script_action_autoload (dogechat_perl_plugin,
                                           &perl_quiet,
                                           &perl_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
dogechat_perl_signal_script_action_cb (void *data, const char *signal,
                                      const char *type_data,
                                      void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (strcmp (type_data, DOGECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "perl_script_install") == 0)
        {
            plugin_script_action_add (&perl_action_install_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_perl_timer_action_cb,
                                &perl_action_install_list);
        }
        else if (strcmp (signal, "perl_script_remove") == 0)
        {
            plugin_script_action_add (&perl_action_remove_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_perl_timer_action_cb,
                                &perl_action_remove_list);
        }
        else if (strcmp (signal, "perl_script_autoload") == 0)
        {
            plugin_script_action_add (&perl_action_autoload_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_perl_timer_action_cb,
                                &perl_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when exiting or upgrading DogeChat.
 */

int
dogechat_perl_signal_quit_upgrade_cb (void *data, const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    perl_quit_or_upgrade = 1;

    return DOGECHAT_RC_OK;
}

/*
 * Initializes perl plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;
#ifdef PERL_SYS_INIT3
    int a;
    char **perl_args_local;
    char *perl_env[] = {};
    a = perl_args_count;
    perl_args_local = perl_args;
    (void) perl_env;
    PERL_SYS_INIT3 (&a, (char ***)&perl_args_local, (char ***)&perl_env);
#endif /* PERL_SYS_INIT3 */

    dogechat_perl_plugin = plugin;

#ifndef MULTIPLICITY
    perl_main = perl_alloc ();

    if (!perl_main)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to initialize %s"),
                        dogechat_prefix ("error"), PERL_PLUGIN_NAME,
                        PERL_PLUGIN_NAME);
        return DOGECHAT_RC_ERROR;
    }

    perl_construct (perl_main);
    perl_parse (perl_main, dogechat_perl_api_init, perl_args_count,
                perl_args, NULL);
#endif /* MULTIPLICITY */

    init.callback_command = &dogechat_perl_command_cb;
    init.callback_completion = &dogechat_perl_completion_cb;
    init.callback_hdata = &dogechat_perl_hdata_cb;
    init.callback_infolist = &dogechat_perl_infolist_cb;
    init.callback_signal_debug_dump = &dogechat_perl_signal_debug_dump_cb;
    init.callback_signal_debug_libs = &dogechat_perl_signal_debug_libs_cb;
    init.callback_signal_buffer_closed = &dogechat_perl_signal_buffer_closed_cb;
    init.callback_signal_script_action = &dogechat_perl_signal_script_action_cb;
    init.callback_load_file = &dogechat_perl_load_cb;

    perl_quiet = 1;
    plugin_script_init (dogechat_perl_plugin, argc, argv, &init);
    perl_quiet = 0;

    plugin_script_display_short_list (dogechat_perl_plugin,
                                      perl_scripts);

    dogechat_hook_signal ("quit", &dogechat_perl_signal_quit_upgrade_cb, NULL);
    dogechat_hook_signal ("upgrade", &dogechat_perl_signal_quit_upgrade_cb, NULL);

    /* init OK */
    return DOGECHAT_RC_OK;
}

/*
 * Ends perl plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* unload all scripts */
    perl_quiet = 1;
    plugin_script_end (plugin, &perl_scripts, &dogechat_perl_unload_all);
    perl_quiet = 0;

#ifndef MULTIPLICITY
    /* free perl interpreter */
    if (perl_main)
    {
        perl_destruct (perl_main);
        perl_free (perl_main);
        perl_main = NULL;
    }
#endif /* MULTIPLICITY */

#if defined(PERL_SYS_TERM) && !defined(__FreeBSD__) && !defined(WIN32) && !defined(__CYGWIN__) && !(defined(__APPLE__) && defined(__MACH__))
    /*
     * we call this function on all OS, but NOT on FreeBSD or Cygwin,
     * because it crashes with no reason (bug in Perl?)
     */
    if (perl_quit_or_upgrade)
        PERL_SYS_TERM ();
#endif

    /* free some data */
    if (perl_action_install_list)
        free (perl_action_install_list);
    if (perl_action_remove_list)
        free (perl_action_remove_list);
    if (perl_action_autoload_list)
        free (perl_action_autoload_list);

    return DOGECHAT_RC_OK;
}
