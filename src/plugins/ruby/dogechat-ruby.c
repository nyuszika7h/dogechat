/*
 * dogechat-ruby.c - ruby plugin for DogeChat
 *
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
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

#include <ruby.h>
#if (defined(RUBY_API_VERSION_MAJOR) && defined(RUBY_API_VERSION_MINOR)) && (RUBY_API_VERSION_MAJOR >= 2 || (RUBY_API_VERSION_MAJOR == 1 && RUBY_API_VERSION_MINOR >= 9))
#include <ruby/encoding.h>
#endif
#ifdef HAVE_RUBY_VERSION_H
#include <ruby/version.h>
#endif

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../dogechat-plugin.h"
#include "../plugin-script.h"
#include "dogechat-ruby.h"
#include "dogechat-ruby-api.h"

#ifndef StringValuePtr
#define StringValuePtr(s) STR2CSTR(s)
#endif
#ifndef RARRAY_LEN
#define RARRAY_LEN(s) RARRAY(s)->len
#endif
#ifndef RARRAY_PTR
#define RARRAY_PTR(s) RARRAY(s)->ptr
#endif
#ifndef RSTRING_LEN
#define RSTRING_LEN(s) RSTRING(s)->len
#endif
#ifndef RSTRING_PTR
#define RSTRING_PTR(s) RSTRING(s)->ptr
#endif


DOGECHAT_PLUGIN_NAME(RUBY_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("Support of ruby scripts"));
DOGECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(3000);

struct t_dogechat_plugin *dogechat_ruby_plugin = NULL;

int ruby_quiet = 0;
int ruby_hide_errors = 0;
struct t_plugin_script *ruby_scripts = NULL;
struct t_plugin_script *last_ruby_script = NULL;
struct t_plugin_script *ruby_current_script = NULL;
struct t_plugin_script *ruby_registered_script = NULL;
const char *ruby_current_script_filename = NULL;
VALUE ruby_current_module;

/*
 * string used to execute action "install":
 * when signal "ruby_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *ruby_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "ruby_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *ruby_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "ruby_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *ruby_action_autoload_list = NULL;

VALUE ruby_mDogechat, ruby_mDogechatOutputs;

#define MOD_NAME_PREFIX "DogechatRubyModule"
int ruby_num = 0;

char ruby_buffer_output[128];

typedef struct protect_call_arg {
    VALUE recv;
    ID mid;
    int argc;
    VALUE *argv;
} protect_call_arg_t;


/*
 * Callback called for each key/value in a hashtable.
 */

void
dogechat_ruby_hashtable_map_cb (void *data,
                               struct t_hashtable *hashtable,
                               const char *key,
                               const char *value)
{
    VALUE *hash;

    /* make C compiler happy */
    (void) hashtable;

    hash = (VALUE *)data;

    rb_hash_aset (hash[0], rb_str_new2 (key), rb_str_new2 (value));
}

/*
 * Converts a DogeChat hashtable to a ruby hash.
 */

VALUE
dogechat_ruby_hashtable_to_hash (struct t_hashtable *hashtable)
{
    VALUE hash;

    hash = rb_hash_new ();
    if (NIL_P (hash))
        return Qnil;

    dogechat_hashtable_map_string (hashtable,
                                  &dogechat_ruby_hashtable_map_cb,
                                  &hash);

    return hash;
}

/*
 * Callback called for each key/value in a hashtable.
 */

int
dogechat_ruby_hash_foreach_cb (VALUE key, VALUE value, void *arg)
{
    struct t_hashtable *hashtable;
    const char *type_values;

    hashtable = (struct t_hashtable *)arg;

    if ((TYPE(key) == T_STRING) && (TYPE(value) == T_STRING))
    {
        type_values = dogechat_hashtable_get_string (hashtable, "type_values");
        if (strcmp (type_values, DOGECHAT_HASHTABLE_STRING) == 0)
        {
            dogechat_hashtable_set (hashtable, StringValuePtr(key),
                                   StringValuePtr(value));
        }
        else if (strcmp (type_values, DOGECHAT_HASHTABLE_POINTER) == 0)
        {
            dogechat_hashtable_set (hashtable, StringValuePtr(key),
                                   plugin_script_str2ptr (
                                       dogechat_ruby_plugin,
                                       NULL, NULL,
                                       StringValuePtr(value)));
        }
    }

    return 0;
}

/*
 * Converts a ruby hash to a DogeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
dogechat_ruby_hash_to_hashtable (VALUE hash, int size, const char *type_keys,
                                const char *type_values)
{
    struct t_hashtable *hashtable;

    hashtable = dogechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    rb_hash_foreach (hash, &dogechat_ruby_hash_foreach_cb,
                     (unsigned long)hashtable);

    return hashtable;
}

/*
 * Used to protect a function call.
 */

static VALUE
protect_funcall0 (VALUE arg)
{
    return rb_funcall2 (((protect_call_arg_t *)arg)->recv,
                        ((protect_call_arg_t *)arg)->mid,
                        ((protect_call_arg_t *)arg)->argc,
                        ((protect_call_arg_t *)arg)->argv);
}

/*
 * Calls function in protected mode.
 */

VALUE
rb_protect_funcall (VALUE recv, ID mid, int *state, int argc, VALUE *argv)
{
    struct protect_call_arg arg;

    arg.recv = recv;
    arg.mid = mid;
    arg.argc = argc;
    arg.argv = argv;
    return rb_protect (protect_funcall0, (VALUE) &arg, state);
}

/*
 * Displays ruby exception.
 */

int
dogechat_ruby_print_exception (VALUE err)
{
    VALUE backtrace, tmp1, tmp2, tmp3;
    int i;
    int ruby_error;
    char* line;
    char* cline;
    char* err_msg;
    char* err_class;

    backtrace = rb_protect_funcall (err, rb_intern("backtrace"),
                                    &ruby_error, 0, NULL);

    tmp1 = rb_protect_funcall(err, rb_intern("message"), &ruby_error, 0, NULL);
    err_msg = StringValueCStr(tmp1);

    tmp2 = rb_protect_funcall(rb_protect_funcall(err, rb_intern("class"),
                                                 &ruby_error, 0, NULL),
                              rb_intern("name"), &ruby_error, 0, NULL);
    err_class = StringValuePtr(tmp2);

    if (strcmp (err_class, "SyntaxError") == 0)
    {
        tmp3 = rb_inspect(err);
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: error: %s"),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME,
                        StringValuePtr(tmp3));
    }
    else
    {
        for (i = 0; i < RARRAY_LEN(backtrace); i++)
        {
            line = StringValuePtr(RARRAY_PTR(backtrace)[i]);
            cline = NULL;
            if (i == 0)
            {
                cline = (char *)calloc (strlen (line) + 2 + strlen (err_msg) +
                                        3 + strlen (err_class) + 1,
                                        sizeof (char));
                if (cline)
                {
                    strcat (cline, line);
                    strcat (cline, ": ");
                    strcat (cline, err_msg);
                    strcat (cline, " (");
                    strcat (cline, err_class);
                    strcat (cline, ")");
                }
            }
            else
            {
                cline = (char *)calloc(strlen (line) + strlen ("     from ") + 1,
                                       sizeof (char));
                if (cline)
                {
                    strcat (cline, "     from ");
                    strcat (cline, line);
                }
            }
            if (cline)
            {
                dogechat_printf (NULL,
                                dogechat_gettext ("%s%s: error: %s"),
                                dogechat_prefix ("error"), RUBY_PLUGIN_NAME,
                                cline);
            }

            if (cline)
                free (cline);
        }
    }

    return 0;
}

/*
 * Executes a ruby function.
 */

void *
dogechat_ruby_exec (struct t_plugin_script *script,
                   int ret_type, const char *function,
                   const char *format, void **argv)
{
    VALUE rc, err;
    int ruby_error, i, argc, *ret_i;
    VALUE argv2[16];
    void *ret_value;
    struct t_plugin_script *old_ruby_current_script;

    old_ruby_current_script = ruby_current_script;
    ruby_current_script = script;

    argc = 0;
    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    argv2[i] = rb_str_new2 ((char *)argv[i]);
                    break;
                case 'i': /* integer */
                    argv2[i] = INT2FIX (*((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    argv2[i] = dogechat_ruby_hashtable_to_hash (argv[i]);
                    break;
            }
        }
    }

    if (argc > 0)
    {
        rc = rb_protect_funcall ((VALUE) script->interpreter,
                                 rb_intern(function),
                                 &ruby_error, argc, argv2);
    }
    else
    {
        rc = rb_protect_funcall ((VALUE) script->interpreter,
                                 rb_intern(function),
                                 &ruby_error, 0, NULL);
    }

    if (ruby_error)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to run function \"%s\""),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME, function);

        err = rb_gv_get("$!");
        dogechat_ruby_print_exception(err);

        return NULL;
    }

    if ((TYPE(rc) == T_STRING) && (ret_type == DOGECHAT_SCRIPT_EXEC_STRING))
    {
        if (StringValuePtr (rc))
            ret_value = strdup (StringValuePtr (rc));
        else
            ret_value = NULL;
    }
    else if ((TYPE(rc) == T_FIXNUM) && (ret_type == DOGECHAT_SCRIPT_EXEC_INT))
    {
        ret_i = malloc (sizeof (*ret_i));
        if (ret_i)
            *ret_i = NUM2INT(rc);
        ret_value = ret_i;
    }
    else if (ret_type == DOGECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        ret_value = dogechat_ruby_hash_to_hashtable (rc,
                                                    DOGECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                    DOGECHAT_HASHTABLE_STRING,
                                                    DOGECHAT_HASHTABLE_STRING);
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"%s\" must return a "
                                         "valid value"),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME, function);
        ruby_current_script = old_ruby_current_script;
        return DOGECHAT_RC_OK;
    }

    if (ret_value == NULL)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: not enough memory in function "
                                         "\"%s\""),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME, function);
        ruby_current_script = old_ruby_current_script;
        return NULL;
    }

    ruby_current_script = old_ruby_current_script;

    return ret_value;
}

/*
 * Redirection for stdout and stderr.
 */

static VALUE
dogechat_ruby_output (VALUE self, VALUE str)
{
    char *msg, *p, *m;

    /* make C compiler happy */
    (void) self;

    if (ruby_hide_errors)
        return Qnil;

    msg = strdup(StringValuePtr(str));

    m = msg;
    while ((p = strchr (m, '\n')) != NULL)
    {
        *p = '\0';
        if (strlen (m) + strlen (ruby_buffer_output) > 0)
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s%s: stdout/stderr: %s%s"),
                            dogechat_prefix ("error"), RUBY_PLUGIN_NAME,
                            ruby_buffer_output, m);
        }
        *p = '\n';
        ruby_buffer_output[0] = '\0';
        m = ++p;
    }

    if (strlen(m) + strlen(ruby_buffer_output) > sizeof(ruby_buffer_output))
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: stdout/stderr: %s%s"),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME,
                        ruby_buffer_output, m);
        ruby_buffer_output[0] = '\0';
    }
    else
        strcat (ruby_buffer_output, m);

    if (msg)
        free (msg);

    return Qnil;
}

/*
 * Function used for compatibility.
 */

static VALUE
dogechat_ruby_output_flush (VALUE self)
{
    /* make C compiler happy */
    (void) self;

    return Qnil;
}

/*
 * Loads a ruby script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dogechat_ruby_load (const char *filename)
{
    char modname[64];
    VALUE ruby_retcode, err, argv[1];
    int ruby_error;
    struct stat buf;

    if (stat (filename, &buf) != 0)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not found"),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME, filename);
        return 0;
    }

    if ((dogechat_ruby_plugin->debug >= 2) || !ruby_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: loading script \"%s\""),
                        RUBY_PLUGIN_NAME, filename);
    }

    ruby_current_script = NULL;
    ruby_registered_script = NULL;

    snprintf (modname, sizeof(modname), "%s%d", MOD_NAME_PREFIX, ruby_num);
    ruby_num++;

    ruby_current_module = rb_define_module (modname);

    ruby_current_script_filename = filename;

    argv[0] = rb_str_new2 (filename);
    ruby_retcode = rb_protect_funcall (ruby_current_module,
                                       rb_intern ("load_eval_file"),
                                       &ruby_error, 1, argv);

    if (ruby_retcode == Qnil)
    {
        err = rb_gv_get("$!");
        dogechat_ruby_print_exception(err);
        return 0;
    }

    if (NUM2INT(ruby_retcode) != 0)
    {
        switch (NUM2INT(ruby_retcode))
        {
            case 1:
                dogechat_printf (NULL,
                                dogechat_gettext ("%s%s: unable to read file "
                                                 "\"%s\""),
                                dogechat_prefix ("error"), RUBY_PLUGIN_NAME,
                                filename);
                break;
            case 2:
                dogechat_printf (NULL,
                                dogechat_gettext ("%s%s: error while loading "
                                                 "file \"%s\""),
                                dogechat_prefix ("error"), RUBY_PLUGIN_NAME,
                                filename);
                break;
            case 3:
                dogechat_printf (NULL,
                                dogechat_gettext ("%s%s: function "
                                                 "\"dogechat_init\" is missing "
                                                 "in file \"%s\""),
                                dogechat_prefix ("error"), RUBY_PLUGIN_NAME,
                                filename);
                break;
        }

        if (NUM2INT(ruby_retcode) == 2)
        {
            dogechat_ruby_print_exception (rb_iv_get (ruby_current_module,
                                                     "@load_eval_file_error"));
        }

        return 0;
    }

    (void) rb_protect_funcall (ruby_current_module, rb_intern ("dogechat_init"),
                               &ruby_error, 0, NULL);

    if (ruby_error)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to eval function "
                                         "\"dogechat_init\" in file \"%s\""),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME, filename);

        err = rb_gv_get("$!");
        dogechat_ruby_print_exception(err);

        if (ruby_current_script)
        {
            plugin_script_remove (dogechat_ruby_plugin,
                                  &ruby_scripts, &last_ruby_script,
                                  ruby_current_script);
            ruby_current_script = NULL;
        }

        return 0;
    }

    if (!ruby_registered_script)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME, filename);
        return 0;
    }
    ruby_current_script = ruby_registered_script;

    rb_gc_register_address (ruby_current_script->interpreter);

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (dogechat_ruby_plugin,
                                        ruby_scripts,
                                        ruby_current_script,
                                        &dogechat_ruby_api_buffer_input_data_cb,
                                        &dogechat_ruby_api_buffer_close_cb);

    (void) dogechat_hook_signal_send ("ruby_script_loaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING,
                                     ruby_current_script->filename);

    return 1;
}

/*
 * Callback for dogechat_script_auto_load() function.
 */

void
dogechat_ruby_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    dogechat_ruby_load (filename);
}

/*
 * Unloads a ruby script.
 */

void
dogechat_ruby_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((dogechat_ruby_plugin->debug >= 2) || !ruby_quiet)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s: unloading script \"%s\""),
                        RUBY_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)dogechat_ruby_exec (script,
                                       DOGECHAT_SCRIPT_EXEC_INT,
                                       script->shutdown_func,
                                       0, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (ruby_current_script == script)
        ruby_current_script = (ruby_current_script->prev_script) ?
            ruby_current_script->prev_script : ruby_current_script->next_script;

    plugin_script_remove (dogechat_ruby_plugin, &ruby_scripts, &last_ruby_script,
                          script);

    if (interpreter)
        rb_gc_unregister_address (interpreter);

    (void) dogechat_hook_signal_send ("ruby_script_unloaded",
                                     DOGECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a ruby script by name.
 */

void
dogechat_ruby_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (dogechat_ruby_plugin, ruby_scripts, name);
    if (ptr_script)
    {
        dogechat_ruby_unload (ptr_script);
        if (!ruby_quiet)
        {
            dogechat_printf (NULL,
                            dogechat_gettext ("%s: script \"%s\" unloaded"),
                            RUBY_PLUGIN_NAME, name);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME, name);
    }
}

/*
 * Reloads a ruby script by name.
 */

void
dogechat_ruby_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (dogechat_ruby_plugin, ruby_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            dogechat_ruby_unload (ptr_script);
            if (!ruby_quiet)
            {
                dogechat_printf (NULL,
                                dogechat_gettext ("%s: script \"%s\" unloaded"),
                                RUBY_PLUGIN_NAME, name);
            }
            dogechat_ruby_load (filename);
            free (filename);
        }
    }
    else
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: script \"%s\" not loaded"),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all ruby scripts.
 */

void
dogechat_ruby_unload_all ()
{
    while (ruby_scripts)
    {
        dogechat_ruby_unload (ruby_scripts);
    }
}

/*
 * Callback for command "/ruby".
 */

int
dogechat_ruby_command_cb (void *data, struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (dogechat_ruby_plugin, ruby_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_ruby_plugin, ruby_scripts,
                                        NULL, 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_ruby_plugin, ruby_scripts,
                                        NULL, 1);
        }
        else if (dogechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (dogechat_ruby_plugin, &dogechat_ruby_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "reload") == 0)
        {
            dogechat_ruby_unload_all ();
            plugin_script_auto_load (dogechat_ruby_plugin, &dogechat_ruby_load_cb);
        }
        else if (dogechat_strcasecmp (argv[1], "unload") == 0)
        {
            dogechat_ruby_unload_all ();
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }
    else
    {
        if (dogechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (dogechat_ruby_plugin, ruby_scripts,
                                        argv_eol[2], 0);
        }
        else if (dogechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (dogechat_ruby_plugin, ruby_scripts,
                                        argv_eol[2], 1);
        }
        else if ((dogechat_strcasecmp (argv[1], "load") == 0)
                 || (dogechat_strcasecmp (argv[1], "reload") == 0)
                 || (dogechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                ruby_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (dogechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load ruby script */
                path_script = plugin_script_search_path (dogechat_ruby_plugin,
                                                         ptr_name);
                dogechat_ruby_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (dogechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one ruby script */
                dogechat_ruby_reload_name (ptr_name);
            }
            else if (dogechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload ruby script */
                dogechat_ruby_unload_name (ptr_name);
            }
            ruby_quiet = 0;
        }
        else
            DOGECHAT_COMMAND_ERROR;
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds ruby scripts to completion list.
 */

int
dogechat_ruby_completion_cb (void *data, const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (dogechat_ruby_plugin, completion, ruby_scripts);

    return DOGECHAT_RC_OK;
}

/*
 * Returns hdata for ruby scripts.
 */

struct t_hdata *
dogechat_ruby_hdata_cb (void *data, const char *hdata_name)
{
    /* make C compiler happy */
    (void) data;

    return plugin_script_hdata_script (dogechat_plugin,
                                       &ruby_scripts, &last_ruby_script,
                                       hdata_name);
}

/*
 * Returns infolist with ruby scripts.
 */

struct t_infolist *
dogechat_ruby_infolist_cb (void *data, const char *infolist_name,
                          void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (dogechat_strcasecmp (infolist_name, "ruby_script") == 0)
    {
        return plugin_script_infolist_list_scripts (dogechat_ruby_plugin,
                                                    ruby_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps ruby plugin data in DogeChat log file.
 */

int
dogechat_ruby_signal_debug_dump_cb (void *data, const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (dogechat_strcasecmp ((char *)signal_data, RUBY_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (dogechat_ruby_plugin, ruby_scripts);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Display infos about external libraries used.
 */

int
dogechat_ruby_signal_debug_libs_cb (void *data, const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

#ifdef HAVE_RUBY_VERSION_H
    dogechat_printf (NULL, "  %s: %s", RUBY_PLUGIN_NAME, ruby_version);
#else
    dogechat_printf (NULL, "  %s: (?)", RUBY_PLUGIN_NAME);
#endif /* HAVE_RUBY_VERSION_H */

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a buffer is closed.
 */

int
dogechat_ruby_signal_buffer_closed_cb (void *data, const char *signal,
                                      const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        plugin_script_remove_buffer_callbacks (ruby_scripts, signal_data);

    return DOGECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
dogechat_ruby_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &ruby_action_install_list)
        {
            plugin_script_action_install (dogechat_ruby_plugin,
                                          ruby_scripts,
                                          &dogechat_ruby_unload,
                                          &dogechat_ruby_load,
                                          &ruby_quiet,
                                          &ruby_action_install_list);
        }
        else if (data == &ruby_action_remove_list)
        {
            plugin_script_action_remove (dogechat_ruby_plugin,
                                         ruby_scripts,
                                         &dogechat_ruby_unload,
                                         &ruby_quiet,
                                         &ruby_action_remove_list);
        }
        else if (data == &ruby_action_autoload_list)
        {
            plugin_script_action_autoload (dogechat_ruby_plugin,
                                           &ruby_quiet,
                                           &ruby_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
dogechat_ruby_signal_script_action_cb (void *data, const char *signal,
                                      const char *type_data,
                                      void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (strcmp (type_data, DOGECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "ruby_script_install") == 0)
        {
            plugin_script_action_add (&ruby_action_install_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_ruby_timer_action_cb,
                                &ruby_action_install_list);
        }
        else if (strcmp (signal, "ruby_script_remove") == 0)
        {
            plugin_script_action_add (&ruby_action_remove_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_ruby_timer_action_cb,
                                &ruby_action_remove_list);
        }
        else if (strcmp (signal, "ruby_script_autoload") == 0)
        {
            plugin_script_action_add (&ruby_action_autoload_list,
                                      (const char *)signal_data);
            dogechat_hook_timer (1, 0, 1,
                                &dogechat_ruby_timer_action_cb,
                                &ruby_action_autoload_list);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Initializes ruby plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;
    int ruby_error;
    char *dogechat_ruby_code = {
        "$stdout = DogechatOutputs\n"
        "$stderr = DogechatOutputs\n"
        "begin"
        "  if RUBY_VERSION.split('.')[0] == '1' and RUBY_VERSION.split('.')[1] <= '8'\n"
        "    require 'rubygems'\n"
        "  else\n"
        "    require 'thread'\n"
        "    class ::Mutex\n"
        "      def synchronize(*args)\n"
        "        yield\n"
        "      end\n"
        "    end\n"
        "    require 'rubygems'\n"
        "  end\n"
        "rescue LoadError\n"
        "end\n"
        "\n"
        "class Module\n"
        "\n"
        "  def load_eval_file (file)\n"
        "    lines = ''\n"
        "    begin\n"
        "      lines = File.read(file)\n"
        "    rescue => e\n"
        "      return 1\n"
        "    end\n"
        "\n"
        "    begin\n"
        "      require 'enc/encdb.so'\n"
        "      require 'enc/trans/transdb.so'\n"
        "      module_eval(lines)\n"
        "    rescue Exception => e\n"
        "      @load_eval_file_error = e\n"
        "      return 2\n"
        "    end\n"
        "\n"
        "    has_init = false\n"
        "\n"
        "    instance_methods.each do |meth|\n"
        "      if meth.to_s == 'dogechat_init'\n"
        "        has_init = true\n"
        "      end\n"
        "      module_eval('module_function :' + meth.to_s)\n"
        "    end\n"
        "\n"
        "    unless has_init\n"
        "      return 3\n"
        "    end\n"
        "\n"
        "    return 0\n"
        "  end\n"
        "end\n"
    };

    dogechat_ruby_plugin = plugin;

    ruby_error = 0;

    /* init stdout/stderr buffer */
    ruby_buffer_output[0] = '\0';

#if (defined(RUBY_API_VERSION_MAJOR) && defined(RUBY_API_VERSION_MINOR)) && (RUBY_API_VERSION_MAJOR >= 2 || (RUBY_API_VERSION_MAJOR == 1 && RUBY_API_VERSION_MINOR >= 9))
    RUBY_INIT_STACK;
#endif

    ruby_init ();

    /* redirect stdin and stdout */
    ruby_mDogechatOutputs = rb_define_module ("DogechatOutputs");
    rb_define_singleton_method (ruby_mDogechatOutputs, "write",
                                dogechat_ruby_output, 1);
    rb_define_singleton_method (ruby_mDogechatOutputs, "puts",
                                dogechat_ruby_output, 1);
    rb_define_singleton_method (ruby_mDogechatOutputs, "p",
                                dogechat_ruby_output, 1);
    rb_define_singleton_method (ruby_mDogechatOutputs, "flush",
                                dogechat_ruby_output_flush, 0);

    ruby_script ("__dogechat_plugin__");

    ruby_mDogechat = rb_define_module ("Dogechat");
    dogechat_ruby_api_init (ruby_mDogechat);

    rb_eval_string_protect (dogechat_ruby_code, &ruby_error);
    if (ruby_error)
    {
        dogechat_printf (NULL,
                        dogechat_gettext ("%s%s: unable to eval DogeChat ruby "
                                         "internal code"),
                        dogechat_prefix ("error"), RUBY_PLUGIN_NAME);
        VALUE err = rb_gv_get ("$!");
        dogechat_ruby_print_exception (err);
        return DOGECHAT_RC_ERROR;
    }

    ruby_init_loadpath ();

    init.callback_command = &dogechat_ruby_command_cb;
    init.callback_completion = &dogechat_ruby_completion_cb;
    init.callback_hdata = &dogechat_ruby_hdata_cb;
    init.callback_infolist = &dogechat_ruby_infolist_cb;
    init.callback_signal_debug_dump = &dogechat_ruby_signal_debug_dump_cb;
    init.callback_signal_debug_libs = &dogechat_ruby_signal_debug_libs_cb;
    init.callback_signal_buffer_closed = &dogechat_ruby_signal_buffer_closed_cb;
    init.callback_signal_script_action = &dogechat_ruby_signal_script_action_cb;
    init.callback_load_file = &dogechat_ruby_load_cb;

    ruby_quiet = 1;
    plugin_script_init (dogechat_ruby_plugin, argc, argv, &init);
    ruby_quiet = 0;

    plugin_script_display_short_list (dogechat_ruby_plugin,
                                      ruby_scripts);

    /* init OK */
    return DOGECHAT_RC_OK;
}

/*
 * Ends ruby plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* unload all scripts */
    ruby_quiet = 1;
    plugin_script_end (plugin, &ruby_scripts, &dogechat_ruby_unload_all);
    ruby_quiet = 0;

    ruby_cleanup (0);

    /* free some data */
    if (ruby_action_install_list)
        free (ruby_action_install_list);
    if (ruby_action_remove_list)
        free (ruby_action_remove_list);
    if (ruby_action_autoload_list)
        free (ruby_action_autoload_list);

    return DOGECHAT_RC_OK;
}
