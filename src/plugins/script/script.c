/*
 * script.c - scripts manager for DogeChat
 *
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#include "../dogechat-plugin.h"
#include "script.h"
#include "script-buffer.h"
#include "script-command.h"
#include "script-completion.h"
#include "script-config.h"
#include "script-info.h"
#include "script-repo.h"


DOGECHAT_PLUGIN_NAME(SCRIPT_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("Scripts manager"));
DOGECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(2000);

struct t_dogechat_plugin *dogechat_script_plugin = NULL;

char *script_language[SCRIPT_NUM_LANGUAGES] =
{ "guile", "lua", "perl", "python", "ruby", "tcl", "javascript" };
char *script_extension[SCRIPT_NUM_LANGUAGES] =
{ "scm",   "lua", "pl",   "py",     "rb",   "tcl", "js" };

int script_plugin_loaded[SCRIPT_NUM_LANGUAGES];
struct t_hashtable *script_loaded = NULL;
struct t_hook *script_timer_refresh = NULL;


/*
 * Searches for a language.
 *
 * Returns index of language, -1 if not found.
 */

int
script_language_search (const char *language)
{
    int i;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        if (strcmp (script_language[i], language) == 0)
            return i;
    }

    /* language not found */
    return -1;
}

/*
 * Searches for a language by extension.
 *
 * Returns index of language, -1 if not found.
 */

int
script_language_search_by_extension (const char *extension)
{
    int i;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        if (strcmp (script_extension[i], extension) == 0)
            return i;
    }

    /* extension not found */
    return -1;
}

/*
 * Builds download URL (to use with hook_process or hook_process_hashtable).
 *
 * If the option script.scripts.url_force_https is enabled, the protocol is
 * forced to HTTPS (if URL starts with "http://").
 *
 * Note: result must be freed after use.
 */

char *
script_build_download_url (const char *url)
{
    char *result;
    int length;

    if (!url || !url[0])
        return NULL;

    /* length of url + "url:" + 1 (for httpS) */
    length = 4 + 1 + strlen (url) + 1;
    result = malloc (length);
    if (!result)
        return NULL;

    if (dogechat_config_boolean (script_config_scripts_url_force_https)
        && (dogechat_strncasecmp (url, "http://", 7) == 0))
    {
        snprintf (result, length, "url:https://%s", url + 7);
    }
    else
    {
        snprintf (result, length, "url:%s", url);
    }

    return result;
}

/*
 * Gets loaded plugins (in array of integers).
 */

void
script_get_loaded_plugins ()
{
    int i, language;
    struct t_hdata *hdata;
    void *ptr_plugin;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        script_plugin_loaded[i] = 0;
    }
    hdata = dogechat_hdata_get ("plugin");
    ptr_plugin = dogechat_hdata_get_list (hdata, "dogechat_plugins");
    while (ptr_plugin)
    {
        language = script_language_search (dogechat_hdata_string (hdata,
                                                                 ptr_plugin,
                                                                 "name"));
        if (language >= 0)
            script_plugin_loaded[language] = 1;
        ptr_plugin = dogechat_hdata_move (hdata, ptr_plugin, 1);
    }
}

/*
 * Gets scripts (in hashtable).
 */

void
script_get_scripts ()
{
    int i;
    char hdata_name[128], *filename, *ptr_base_name;
    const char *ptr_filename;
    struct t_hdata *hdata;
    void *ptr_script;

    if (!script_loaded)
    {
        script_loaded = dogechat_hashtable_new (32,
                                               DOGECHAT_HASHTABLE_STRING,
                                               DOGECHAT_HASHTABLE_STRING,
                                               NULL,
                                               NULL);
    }
    else
        dogechat_hashtable_remove_all (script_loaded);

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[i]);
        hdata = dogechat_hdata_get (hdata_name);
        ptr_script = dogechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            ptr_filename = dogechat_hdata_string (hdata, ptr_script, "filename");
            if (ptr_filename)
            {
                filename = strdup (ptr_filename);
                if (filename)
                {
                    ptr_base_name = basename (filename);
                    dogechat_hashtable_set (script_loaded,
                                           ptr_base_name,
                                           dogechat_hdata_string (hdata, ptr_script,
                                                                 "version"));
                    free (filename);
                }
            }
            ptr_script = dogechat_hdata_move (hdata, ptr_script, 1);
        }
    }
}

/*
 * Callback for signal "debug_dump".
 */

int
script_debug_dump_cb (void *data, const char *signal, const char *type_data,
                      void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (dogechat_strcasecmp ((char *)signal_data, SCRIPT_PLUGIN_NAME) == 0))
    {
        dogechat_log_printf ("");
        dogechat_log_printf ("***** \"%s\" plugin dump *****",
                            dogechat_plugin->name);

        script_repo_print_log ();

        dogechat_log_printf ("");
        dogechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            dogechat_plugin->name);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback for timer to refresh list of scripts.
 */

int
script_timer_refresh_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) data;

    script_get_loaded_plugins ();
    script_get_scripts ();
    script_repo_update_status_all ();
    script_buffer_refresh (0);

    if (remaining_calls == 0)
        script_timer_refresh = NULL;

    return DOGECHAT_RC_OK;
}

/*
 * Callback for signals "plugin_loaded" and "plugin_unloaded".
 */

int
script_signal_plugin_cb (void *data, const char *signal, const char *type_data,
                         void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) type_data;

    if (dogechat_script_plugin->debug >= 2)
    {
        dogechat_printf (NULL, "%s: signal: %s, data: %s",
                        SCRIPT_PLUGIN_NAME,
                        signal, (char *)signal_data);
    }

    if (!script_timer_refresh)
    {
        script_timer_refresh = dogechat_hook_timer (50, 0, 1,
                                                   &script_timer_refresh_cb, NULL);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback for signals "xxx_script_yyy" (example: "python_script_loaded").
 */

int
script_signal_script_cb (void *data, const char *signal, const char *type_data,
                         void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) type_data;

    if (dogechat_script_plugin->debug >= 2)
    {
        dogechat_printf (NULL, "%s: signal: %s, data: %s",
                        SCRIPT_PLUGIN_NAME,
                        signal, (char *)signal_data);
    }

    if (!script_timer_refresh)
    {
        script_timer_refresh = dogechat_hook_timer (50, 0, 1,
                                                   &script_timer_refresh_cb, NULL);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback called when a mouse action occurs in chat area.
 */

struct t_hashtable *
script_focus_chat_cb (void *data, struct t_hashtable *info)
{
    const char *buffer;
    int rc;
    long unsigned int value;
    struct t_gui_buffer *ptr_buffer;
    long x;
    char *error, str_date[64];
    struct t_script_repo *ptr_script;
    struct tm *tm;

    /* make C compiler happy */
    (void) data;

    if (!script_buffer)
        return info;

    buffer = dogechat_hashtable_get (info, "_buffer");
    if (!buffer)
        return info;

    rc = sscanf (buffer, "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        return info;

    ptr_buffer = (struct t_gui_buffer *)value;

    if (!ptr_buffer || (ptr_buffer != script_buffer))
        return info;

    if (script_buffer_detail_script)
        ptr_script = script_buffer_detail_script;
    else
    {
        error = NULL;
        x = strtol (dogechat_hashtable_get (info, "_chat_line_y"), &error, 10);
        if (!error || error[0])
            return info;

        if (x < 0)
            return info;

        ptr_script = script_repo_search_displayed_by_number (x);
        if (!ptr_script)
            return info;
    }

    dogechat_hashtable_set (info, "script_name", ptr_script->name);
    dogechat_hashtable_set (info, "script_name_with_extension", ptr_script->name_with_extension);
    dogechat_hashtable_set (info, "script_language", script_language[ptr_script->language]);
    dogechat_hashtable_set (info, "script_author",ptr_script->author);
    dogechat_hashtable_set (info, "script_mail", ptr_script->mail);
    dogechat_hashtable_set (info, "script_version", ptr_script->version);
    dogechat_hashtable_set (info, "script_license", ptr_script->license);
    dogechat_hashtable_set (info, "script_description", ptr_script->description);
    dogechat_hashtable_set (info, "script_tags", ptr_script->tags);
    dogechat_hashtable_set (info, "script_requirements", ptr_script->requirements);
    dogechat_hashtable_set (info, "script_min_dogechat", ptr_script->min_dogechat);
    dogechat_hashtable_set (info, "script_max_dogechat", ptr_script->max_dogechat);
    dogechat_hashtable_set (info, "script_md5sum", ptr_script->md5sum);
    dogechat_hashtable_set (info, "script_url", ptr_script->url);
    tm = localtime (&ptr_script->date_added);
    strftime (str_date, sizeof (str_date), "%Y-%m-%d %H:%M:%S", tm);
    dogechat_hashtable_set (info, "script_date_added", str_date);
    tm = localtime (&ptr_script->date_updated);
    strftime (str_date, sizeof (str_date), "%Y-%m-%d %H:%M:%S", tm);
    dogechat_hashtable_set (info, "script_date_updated", str_date);
    dogechat_hashtable_set (info, "script_version_loaded", ptr_script->version_loaded);

    return info;
}

/*
 * Initializes script plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    int i;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    dogechat_plugin = plugin;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        script_plugin_loaded[i] = 0;
    }

    script_buffer_set_callbacks ();

    if (!script_config_init ())
        return DOGECHAT_RC_ERROR;

    script_config_read ();

    dogechat_mkdir_home (SCRIPT_PLUGIN_NAME, 0755);

    script_command_init ();
    script_completion_init ();
    script_info_init ();

    dogechat_hook_signal ("debug_dump", &script_debug_dump_cb, NULL);
    dogechat_hook_signal ("window_scrolled", &script_buffer_window_scrolled_cb, NULL);
    dogechat_hook_signal ("plugin_*", &script_signal_plugin_cb, NULL);
    dogechat_hook_signal ("*_script_*", &script_signal_script_cb, NULL);

    dogechat_hook_focus ("chat", &script_focus_chat_cb, NULL);

    if (script_repo_file_exists ())
    {
        if (!script_repo_file_is_uptodate ())
            script_repo_file_update (0);
        else
            script_repo_file_read (0);
    }

    if (script_buffer)
        script_buffer_refresh (1);

    return DOGECHAT_RC_OK;
}

/*
 * Ends script plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    script_config_write ();

    script_repo_remove_all ();

    if (script_repo_filter)
        free (script_repo_filter);

    if (script_loaded)
        dogechat_hashtable_free (script_loaded);

    script_config_free ();

    return DOGECHAT_RC_OK;
}
