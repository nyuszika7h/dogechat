/*
 * script-completion.c - completions for script command
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
#include <libgen.h>

#include "../dogechat-plugin.h"
#include "script.h"
#include "script-repo.h"


/*
 * Adds script languages (python, perl, ruby, ...) to completion list.
 */

int
script_completion_languages_cb (void *data, const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        dogechat_hook_completion_list_add (completion,
                                          script_language[i],
                                          0, DOGECHAT_LIST_POS_SORT);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds script extensions (py, pl, rb, ...) to completion list.
 */

int
script_completion_extensions_cb (void *data, const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        dogechat_hook_completion_list_add (completion,
                                          script_extension[i],
                                          0, DOGECHAT_LIST_POS_SORT);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds scripts to completion list.
 */

int
script_completion_scripts_cb (void *data, const char *completion_item,
                              struct t_gui_buffer *buffer,
                              struct t_gui_completion *completion)
{
    struct t_script_repo *ptr_script;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        dogechat_hook_completion_list_add (completion,
                                          ptr_script->name_with_extension,
                                          0, DOGECHAT_LIST_POS_SORT);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds installed scripts to completion list.
 */

int
script_completion_scripts_installed_cb (void *data, const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_script_repo *ptr_script;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (ptr_script->status & SCRIPT_STATUS_INSTALLED)
        {
            dogechat_hook_completion_list_add (completion,
                                              ptr_script->name_with_extension,
                                              0, DOGECHAT_LIST_POS_SORT);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds files in script directories to completion list.
 */

void
script_completion_exec_file_cb (void *data, const char *filename)
{
    struct t_gui_completion *completion;
    const char *extension;
    char *pos, *filename2, *ptr_base_name;

    completion = (struct t_gui_completion *)(((void **)data)[0]);
    extension = (const char *)(((void **)data)[1]);

    pos = strrchr (filename, '.');
    if (!pos)
        return;

    /* ignore scripts that do not ends with expected extension */
    if (strcmp (pos + 1, extension) != 0)
        return;

    filename2 = strdup (filename);
    if (filename2)
    {
        ptr_base_name = basename (filename2);
        dogechat_hook_completion_list_add (completion,
                                          ptr_base_name,
                                          0, DOGECHAT_LIST_POS_SORT);
        free (filename2);
    }
}

/*
 * Adds files in script directories to completion list.
 */

int
script_completion_scripts_files_cb (void *data, const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    const char *dogechat_home;
    char *directory;
    int length, i;
    void *pointers[2];

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    dogechat_home = dogechat_info_get ("dogechat_dir", NULL);

    length = strlen (dogechat_home) + 128 + 1;
    directory = malloc (length);
    if (directory)
    {
        for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
        {
            pointers[0] = completion;
            pointers[1] = script_extension[i];

            /* look for files in "~/.dogechat/<language>/" */
            snprintf (directory, length,
                      "%s/%s", dogechat_home, script_language[i]);
            dogechat_exec_on_files (directory, 0,
                                   pointers, &script_completion_exec_file_cb);

            /* look for files in "~/.dogechat/<language>/autoload/" */
            snprintf (directory, length,
                      "%s/%s/autoload", dogechat_home, script_language[i]);
            dogechat_exec_on_files (directory, 0,
                                   pointers, &script_completion_exec_file_cb);
        }
        free (directory);
    }

    return DOGECHAT_RC_OK;
}

/*
 * Adds tags from scripts in repository to completion list.
 */

int
script_completion_tags_cb (void *data, const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    struct t_script_repo *ptr_script;
    char **list_tags;
    int num_tags, i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (ptr_script->tags)
        {
            list_tags = dogechat_string_split (ptr_script->tags, ",", 0, 0,
                                              &num_tags);
            if (list_tags)
            {
                for (i = 0; i < num_tags; i++)
                {
                    dogechat_hook_completion_list_add (completion,
                                                      list_tags[i],
                                                      0, DOGECHAT_LIST_POS_SORT);
                }
                dogechat_string_free_split (list_tags);
            }
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
script_completion_init ()
{
    dogechat_hook_completion ("script_languages",
                             N_("list of script languages"),
                             &script_completion_languages_cb, NULL);
    dogechat_hook_completion ("script_extensions",
                             N_("list of script extensions"),
                             &script_completion_extensions_cb, NULL);
    dogechat_hook_completion ("script_scripts",
                             N_("list of scripts in repository"),
                             &script_completion_scripts_cb, NULL);
    dogechat_hook_completion ("script_scripts_installed",
                             N_("list of scripts installed (from repository)"),
                             &script_completion_scripts_installed_cb, NULL);
    dogechat_hook_completion ("script_files",
                             N_("files in script directories"),
                             &script_completion_scripts_files_cb, NULL);
    dogechat_hook_completion ("script_tags",
                             N_("tags of scripts in repository"),
                             &script_completion_tags_cb, NULL);
}
