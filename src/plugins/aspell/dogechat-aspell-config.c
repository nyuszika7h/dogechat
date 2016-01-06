/*
 * dogechat-aspell-config.c - aspell configuration options (file aspell.conf)
 *
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../dogechat-plugin.h"
#include "dogechat-aspell.h"
#include "dogechat-aspell-config.h"
#include "dogechat-aspell-speller.h"


struct t_config_file *dogechat_aspell_config_file = NULL;
struct t_config_section *dogechat_aspell_config_section_dict = NULL;

int dogechat_aspell_config_loading = 0;

/* aspell config, color section */

struct t_config_option *dogechat_aspell_config_color_misspelled;
struct t_config_option *dogechat_aspell_config_color_suggestions;

/* aspell config, check section */

struct t_config_option *dogechat_aspell_config_check_commands;
struct t_config_option *dogechat_aspell_config_check_default_dict;
struct t_config_option *dogechat_aspell_config_check_during_search;
struct t_config_option *dogechat_aspell_config_check_enabled;
struct t_config_option *dogechat_aspell_config_check_real_time;
struct t_config_option *dogechat_aspell_config_check_suggestions;
struct t_config_option *dogechat_aspell_config_check_word_min_length;


char **dogechat_aspell_commands_to_check = NULL;
int dogechat_aspell_count_commands_to_check = 0;
int *dogechat_aspell_length_commands_to_check = NULL;


/*
 * Callback for changes on option "aspell.check.commands".
 */

void
dogechat_aspell_config_change_commands (void *data,
                                       struct t_config_option *option)
{
    const char *value;
    int i;

    /* make C compiler happy */
    (void) data;

    if (dogechat_aspell_commands_to_check)
    {
        dogechat_string_free_split (dogechat_aspell_commands_to_check);
        dogechat_aspell_commands_to_check = NULL;
        dogechat_aspell_count_commands_to_check = 0;
    }

    if (dogechat_aspell_length_commands_to_check)
    {
        free (dogechat_aspell_length_commands_to_check);
        dogechat_aspell_length_commands_to_check = NULL;
    }

    value = dogechat_config_string (option);
    if (value && value[0])
    {
        dogechat_aspell_commands_to_check = dogechat_string_split (value,
                                                                 ",", 0, 0,
                                                                 &dogechat_aspell_count_commands_to_check);
        if (dogechat_aspell_count_commands_to_check > 0)
        {
            dogechat_aspell_length_commands_to_check = malloc (dogechat_aspell_count_commands_to_check *
                                                              sizeof (int));
            for (i = 0; i < dogechat_aspell_count_commands_to_check; i++)
            {
                dogechat_aspell_length_commands_to_check[i] = strlen (dogechat_aspell_commands_to_check[i]);
            }
        }
    }
}

/*
 * Callback for changes on option "aspell.check.default_dict".
 */

void
dogechat_aspell_config_change_default_dict (void *data,
                                           struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    dogechat_hashtable_remove_all (dogechat_aspell_speller_buffer);
    if (!dogechat_aspell_config_loading)
        dogechat_aspell_speller_remove_unused ();
}

/*
 * Callback for changes on option "aspell.check.enabled".
 */

void
dogechat_aspell_config_change_enabled (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;

    aspell_enabled = dogechat_config_boolean (option);

    /* refresh input and aspell suggestions */
    dogechat_bar_item_update ("input_text");
    dogechat_bar_item_update ("aspell_suggest");
}

/*
 * Callback for changes on option "aspell.check.suggestions".
 */

void
dogechat_aspell_config_change_suggestions (void *data,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    dogechat_bar_item_update ("aspell_suggest");
}

/*
 * Callback for changes on a dictionary.
 */

void
dogechat_aspell_config_dict_change (void *data,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    dogechat_hashtable_remove_all (dogechat_aspell_speller_buffer);
    if (!dogechat_aspell_config_loading)
        dogechat_aspell_speller_remove_unused ();
}

/*
 * Callback called when an option is deleted in section "dict".
 */

int
dogechat_aspell_config_dict_delete_option (void *data,
                                          struct t_config_file *config_file,
                                          struct t_config_section *section,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    dogechat_config_option_free (option);

    dogechat_hashtable_remove_all (dogechat_aspell_speller_buffer);
    if (!dogechat_aspell_config_loading)
        dogechat_aspell_speller_remove_unused ();

    return DOGECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Creates an option in section "dict".
 */

int
dogechat_aspell_config_dict_create_option (void *data,
                                          struct t_config_file *config_file,
                                          struct t_config_section *section,
                                          const char *option_name,
                                          const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = DOGECHAT_CONFIG_OPTION_SET_ERROR;

    if (value && value[0])
        dogechat_aspell_speller_check_dictionaries (value);

    if (option_name)
    {
        ptr_option = dogechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = dogechat_config_option_set (ptr_option, value, 0);
            else
            {
                dogechat_config_option_free (ptr_option);
                rc = DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = dogechat_config_new_option (
                    config_file, section,
                    option_name, "string",
                    _("comma separated list of dictionaries to use on this buffer"),
                    NULL, 0, 0, "", value, 0,
                    NULL, NULL,
                    &dogechat_aspell_config_dict_change, NULL,
                    NULL, NULL);
                rc = (ptr_option) ?
                    DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : DOGECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (rc == DOGECHAT_CONFIG_OPTION_SET_ERROR)
    {
        dogechat_printf (NULL,
                        _("%s%s: error creating aspell dictionary \"%s\" => \"%s\""),
                        dogechat_prefix ("error"), ASPELL_PLUGIN_NAME,
                        option_name, value);
    }
    else
    {
        dogechat_hashtable_remove_all (dogechat_aspell_speller_buffer);
        if (!dogechat_aspell_config_loading)
            dogechat_aspell_speller_remove_unused ();
    }

    return rc;
}

/*
 * Callback for changes on an aspell option.
 */

void
dogechat_aspell_config_option_change (void *data,
                                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    dogechat_hashtable_remove_all (dogechat_aspell_speller_buffer);
    if (!dogechat_aspell_config_loading)
        dogechat_aspell_speller_remove_unused ();
}

/*
 * Callback called when an option is deleted in section "option".
 */

int
dogechat_aspell_config_option_delete_option (void *data,
                                            struct t_config_file *config_file,
                                            struct t_config_section *section,
                                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    dogechat_config_option_free (option);

    dogechat_hashtable_remove_all (dogechat_aspell_speller_buffer);
    if (!dogechat_aspell_config_loading)
        dogechat_aspell_speller_remove_unused ();

    return DOGECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Callback called when an option is created in section "option".
 */

int
dogechat_aspell_config_option_create_option (void *data,
                                            struct t_config_file *config_file,
                                            struct t_config_section *section,
                                            const char *option_name,
                                            const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = DOGECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = dogechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = dogechat_config_option_set (ptr_option, value, 1);
            else
            {
                dogechat_config_option_free (ptr_option);
                rc = DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = dogechat_config_new_option (
                    config_file, section,
                    option_name, "string",
                    _("option for aspell (for list of available options and "
                      "format, run command \"aspell config\" in a shell)"),
                    NULL, 0, 0, "", value, 0,
                    NULL, NULL,
                    &dogechat_aspell_config_option_change, NULL,
                    NULL, NULL);
                rc = (ptr_option) ?
                    DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : DOGECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (rc == DOGECHAT_CONFIG_OPTION_SET_ERROR)
    {
        dogechat_printf (NULL,
                        _("%s%s: error creating aspell option \"%s\" => \"%s\""),
                        dogechat_prefix ("error"), ASPELL_PLUGIN_NAME,
                        option_name, value);
    }
    else
    {
        dogechat_hashtable_remove_all (dogechat_aspell_speller_buffer);
        if (!dogechat_aspell_config_loading)
            dogechat_aspell_speller_remove_unused ();
    }

    return rc;
}

/*
 * Gets a list of dictionaries for a buffer.
 */

struct t_config_option *
dogechat_aspell_config_get_dict (const char *name)
{
    return dogechat_config_search_option (dogechat_aspell_config_file,
                                         dogechat_aspell_config_section_dict,
                                         name);
}

/*
 * Sets a list of dictionaries for a buffer.
 */

int
dogechat_aspell_config_set_dict (const char *name, const char *value)
{
    return dogechat_aspell_config_dict_create_option (NULL,
                                                     dogechat_aspell_config_file,
                                                     dogechat_aspell_config_section_dict,
                                                     name,
                                                     value);
}

/*
 * Initializes aspell configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dogechat_aspell_config_init ()
{
    struct t_config_section *ptr_section;

    dogechat_aspell_config_file = dogechat_config_new (ASPELL_CONFIG_NAME,
                                                     NULL, NULL);
    if (!dogechat_aspell_config_file)
        return 0;

    /* color */
    ptr_section = dogechat_config_new_section (dogechat_aspell_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        dogechat_config_free (dogechat_aspell_config_file);
        return 0;
    }

    dogechat_aspell_config_color_misspelled = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "misspelled", "color",
        N_("text color for misspelled words (input bar)"),
        NULL, 0, 0, "lightred", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    dogechat_aspell_config_color_suggestions = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "suggestions", "color",
        N_("text color for suggestions on a misspelled word (status bar)"),
        NULL, 0, 0, "default", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* check */
    ptr_section = dogechat_config_new_section (dogechat_aspell_config_file, "check",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        dogechat_config_free (dogechat_aspell_config_file);
        return 0;
    }

    dogechat_aspell_config_check_commands = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "commands", "string",
        N_("comma separated list of commands for which spell checking is "
           "enabled (spell checking is disabled for all other commands)"),
        NULL, 0, 0,
        "ame,amsg,away,command,cycle,kick,kickban,me,msg,notice,part,query,"
        "quit,topic", NULL, 0,
        NULL, NULL, &dogechat_aspell_config_change_commands, NULL, NULL, NULL);
    dogechat_aspell_config_check_default_dict = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "default_dict", "string",
        N_("default dictionary (or comma separated list of dictionaries) to "
           "use when buffer has no dictionary defined (leave blank to disable "
           "aspell on buffers for which you didn't explicitly enabled it)"),
        NULL, 0, 0, "", NULL, 0,
        NULL, NULL, &dogechat_aspell_config_change_default_dict, NULL, NULL, NULL);
    dogechat_aspell_config_check_during_search = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "during_search", "boolean",
        N_("check words during text search in buffer"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    dogechat_aspell_config_check_enabled = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "enabled", "boolean",
        N_("enable aspell check for command line"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, &dogechat_aspell_config_change_enabled, NULL, NULL, NULL);
    dogechat_aspell_config_check_real_time = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "real_time", "boolean",
        N_("real-time spell checking of words (slower, disabled by default: "
           "words are checked only if there's delimiter after)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    dogechat_aspell_config_check_suggestions = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "suggestions", "integer",
        N_("number of suggestions to display in bar item \"aspell_suggest\" "
           "for each dictionary set in buffer (-1 = disable suggestions, "
           "0 = display all possible suggestions in all languages)"),
        NULL, -1, INT_MAX, "-1", NULL, 0,
        NULL, NULL, &dogechat_aspell_config_change_suggestions, NULL, NULL, NULL);
    dogechat_aspell_config_check_word_min_length = dogechat_config_new_option (
        dogechat_aspell_config_file, ptr_section,
        "word_min_length", "integer",
        N_("minimum length for a word to be spell checked (use 0 to check all "
           "words)"),
        NULL, 0, INT_MAX, "2", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* dict */
    ptr_section = dogechat_config_new_section (dogechat_aspell_config_file, "dict",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &dogechat_aspell_config_dict_create_option, NULL,
                                              &dogechat_aspell_config_dict_delete_option, NULL);
    if (!ptr_section)
    {
        dogechat_config_free (dogechat_aspell_config_file);
        return 0;
    }

    dogechat_aspell_config_section_dict = ptr_section;

    /* option */
    ptr_section = dogechat_config_new_section (dogechat_aspell_config_file, "option",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &dogechat_aspell_config_option_create_option, NULL,
                                              &dogechat_aspell_config_option_delete_option, NULL);
    if (!ptr_section)
    {
        dogechat_config_free (dogechat_aspell_config_file);
        return 0;
    }

    return 1;
}

/*
 * Reads aspell configuration file.
 */

int
dogechat_aspell_config_read ()
{
    int rc;

    dogechat_aspell_config_loading = 1;
    rc = dogechat_config_read (dogechat_aspell_config_file);
    dogechat_aspell_config_loading = 0;
    if (rc == DOGECHAT_CONFIG_READ_OK)
    {
        dogechat_aspell_config_change_commands (NULL,
                                               dogechat_aspell_config_check_commands);
    }
    dogechat_aspell_speller_remove_unused ();

    return rc;
}

/*
 * Writes aspell configuration file.
 */

int
dogechat_aspell_config_write ()
{
    return dogechat_config_write (dogechat_aspell_config_file);
}

/*
 * Frees aspell configuration.
 */

void
dogechat_aspell_config_free ()
{
    dogechat_config_free (dogechat_aspell_config_file);

    if (dogechat_aspell_commands_to_check)
        dogechat_string_free_split (dogechat_aspell_commands_to_check);
    if (dogechat_aspell_length_commands_to_check)
        free (dogechat_aspell_length_commands_to_check);
}
