/*
 * alias-config.c - alias configuration options (file alias.conf)
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

#include "../dogechat-plugin.h"
#include "alias.h"


struct t_config_file *alias_config_file = NULL;
struct t_config_section *alias_config_section_cmd = NULL;
struct t_config_section *alias_config_section_completion = NULL;

char *alias_default_cmd[][2] =
{ { "AAWAY",   "allserv /away"                       },
  { "AME",     "allchan /me"                         },
  { "AMSG",    "allchan /msg *"                      },
  { "ANICK",   "allserv /nick"                       },
  { "BEEP",    "print -beep"                         },
  { "BYE",     "quit"                                },
  { "C",       "buffer clear"                        },
  { "CL",      "buffer clear"                        },
  { "CLOSE",   "buffer close"                        },
  { "CHAT",    "dcc chat"                            },
  { "EXIT",    "quit"                                },
  { "IG",      "ignore"                              },
  { "J",       "join"                                },
  { "K",       "kick"                                },
  { "KB",      "kickban"                             },
  { "LEAVE",   "part"                                },
  { "M",       "msg"                                 },
  { "MUB",     "unban *"                             },
  { "MSGBUF", "command -buffer $1 * /input send $2-" },
  { "N",       "names"                               },
  { "Q",       "query"                               },
  { "REDRAW",  "window refresh"                      },
  { "SAY",     "msg *"                               },
  { "SIGNOFF", "quit"                                },
  { "T",       "topic"                               },
  { "UB",      "unban"                               },
  { "UMODE",   "mode $nick"                          },
  { "V",       "command core version"                },
  { "W",       "who"                                 },
  { "WC",      "window merge"                        },
  { "WI",      "whois"                               },
  { "WII",     "whois $1 $1"                         },
  { "WW",      "whowas"                              },
  { NULL,      NULL                                  },
};

char *alias_default_completion[][2] =
{ { "MSGBUF", "%(buffers_plugins_names)" },
  { NULL,     NULL                       },
};

/*
 * Callback for changes on options in section "cmd" (command).
 */

void
alias_config_cmd_change_cb (void *data, struct t_config_option *option)
{
    struct t_config_option *ptr_option_completion;

    /* make C compiler happy */
    (void) data;

    ptr_option_completion = dogechat_config_search_option (alias_config_file,
                                                          alias_config_section_completion,
                                                          dogechat_config_option_get_pointer (option, "name"));

    alias_new (dogechat_config_option_get_pointer (option, "name"),
               dogechat_config_option_get_pointer (option, "value"),
               (ptr_option_completion) ?
               dogechat_config_option_get_pointer (ptr_option_completion, "value") : NULL);
}

/*
 * Callback called when an option is deleted in section "cmd" (command).
 */

void
alias_config_cmd_delete_cb (void *data, struct t_config_option *option)
{
    struct t_config_option *ptr_option_completion;
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) data;

    ptr_option_completion = dogechat_config_search_option (alias_config_file,
                                                          alias_config_section_completion,
                                                          dogechat_config_option_get_pointer (option, "name"));

    ptr_alias = alias_search (dogechat_config_option_get_pointer (option, "name"));
    if (ptr_alias)
        alias_free (ptr_alias);

    if (ptr_option_completion)
        dogechat_config_option_free (ptr_option_completion);
}

/*
 * Callback for changes on options in section "completion".
 */

void
alias_config_completion_change_cb (void *data, struct t_config_option *option)
{
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) data;

    ptr_alias = alias_search (dogechat_config_option_get_pointer (option, "name"));
    if (ptr_alias && ptr_alias->hook)
    {
        alias_update_completion (ptr_alias,
                                 dogechat_config_option_get_pointer (option, "value"));
    }
}

/*
 * Callback called when an option is deleted in section "completion".
 */

void
alias_config_completion_delete_cb (void *data, struct t_config_option *option)
{
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) data;

    ptr_alias = alias_search (dogechat_config_option_get_pointer (option, "name"));
    if (ptr_alias && ptr_alias->hook && ptr_alias->completion)
    {
        alias_update_completion (ptr_alias, NULL);
    }
}

/*
 * Reloads alias configuration file.
 */

int
alias_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;

    dogechat_config_section_free_options (alias_config_section_cmd);
    dogechat_config_section_free_options (alias_config_section_completion);
    alias_free_all ();

    return dogechat_config_reload (config_file);
}

/*
 * Writes default aliases in configuration file in section "cmd" (command).
 */

int
alias_config_cmd_write_default_cb (void *data,
                                   struct t_config_file *config_file,
                                   const char *section_name)
{
    int i;

    /* make C compiler happy */
    (void) data;

    if (!dogechat_config_write_line (config_file, section_name, NULL))
        return DOGECHAT_CONFIG_WRITE_ERROR;

    for (i = 0; alias_default_cmd[i][0]; i++)
    {
        if (!dogechat_config_write_line (config_file,
                                        alias_default_cmd[i][0],
                                        "\"%s\"", alias_default_cmd[i][1]))
            return DOGECHAT_CONFIG_WRITE_ERROR;
    }

    return DOGECHAT_CONFIG_WRITE_OK;
}

/*
 * Creates a new option in section "cmd" (command).
 */

void
alias_config_cmd_new_option (const char *name, const char *command)
{
    dogechat_config_new_option (alias_config_file, alias_config_section_cmd,
                               name, "string", NULL,
                               NULL, 0, 0, NULL, command, 0,
                               NULL, NULL,
                               &alias_config_cmd_change_cb, NULL,
                               &alias_config_cmd_delete_cb, NULL);
}

/*
 * Callback called when an option is created in section "cmd" (command).
 */

int
alias_config_cmd_create_option_cb (void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name, const char *value)
{
    struct t_alias *ptr_alias;
    int rc;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    /* create configuration option */
    alias_config_cmd_new_option (option_name, value);

    /* create alias */
    ptr_alias = alias_search (option_name);
    if (ptr_alias)
        alias_free (ptr_alias);
    if (value && value[0])
        rc = (alias_new (option_name, value, NULL)) ?
            DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : DOGECHAT_CONFIG_OPTION_SET_ERROR;
    else
        rc = DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    if (rc == DOGECHAT_CONFIG_OPTION_SET_ERROR)
    {
        dogechat_printf (NULL,
                        _("%s%s: error creating alias \"%s\" => \"%s\""),
                        dogechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        option_name, value);
    }

    return rc;
}

/*
 * Writes default completions in configuration file in section "completion".
 */

int
alias_config_completion_write_default_cb (void *data,
                                          struct t_config_file *config_file,
                                          const char *section_name)
{
    int i;

    /* make C compiler happy */
    (void) data;

    if (!dogechat_config_write_line (config_file, section_name, NULL))
        return DOGECHAT_CONFIG_WRITE_ERROR;

    for (i = 0; alias_default_completion[i][0]; i++)
    {
        if (!dogechat_config_write_line (config_file,
                                        alias_default_completion[i][0],
                                        "\"%s\"", alias_default_completion[i][1]))
            return DOGECHAT_CONFIG_WRITE_ERROR;
    }

    return DOGECHAT_CONFIG_WRITE_OK;
}

/*
 * Creates a new option in section "completion".
 */

void
alias_config_completion_new_option (const char *name, const char *completion)
{
    dogechat_config_new_option (alias_config_file,
                               alias_config_section_completion,
                               name, "string", NULL,
                               NULL, 0, 0, NULL, completion, 0,
                               NULL, NULL,
                               &alias_config_completion_change_cb, NULL,
                               &alias_config_completion_delete_cb, NULL);
}

/*
 * Callback called when an option is created in section "completion".
 */

int
alias_config_completion_create_option_cb (void *data,
                                          struct t_config_file *config_file,
                                          struct t_config_section *section,
                                          const char *option_name,
                                          const char *value)
{
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    ptr_alias = alias_search (option_name);
    if (!ptr_alias)
    {
        dogechat_printf (NULL,
                        _("%s%s: error creating completion for alias \"%s\": "
                          "alias not found"),
                        dogechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        option_name);
        return DOGECHAT_CONFIG_OPTION_SET_ERROR;
    }

    /* create configuration option */
    alias_config_completion_new_option (option_name, value);

    /* create/update completion in alias */
    alias_update_completion (ptr_alias, value);

    return DOGECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Initializes alias configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
alias_config_init ()
{
    struct t_config_section *ptr_section;

    alias_config_file = dogechat_config_new (ALIAS_CONFIG_NAME,
                                            &alias_config_reload, NULL);
    if (!alias_config_file)
        return 0;

    /* cmd */
    ptr_section = dogechat_config_new_section (alias_config_file, "cmd",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &alias_config_cmd_write_default_cb, NULL,
                                              &alias_config_cmd_create_option_cb, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        dogechat_config_free (alias_config_file);
        return 0;
    }
    alias_config_section_cmd = ptr_section;

    /* completion */
    ptr_section = dogechat_config_new_section (alias_config_file, "completion",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &alias_config_completion_write_default_cb, NULL,
                                              &alias_config_completion_create_option_cb, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        dogechat_config_free (alias_config_file);
        return 0;
    }
    alias_config_section_completion = ptr_section;

    return 1;
}

/*
 * Reads alias configuration file.
 */

int
alias_config_read ()
{
    return dogechat_config_read (alias_config_file);
}

/*
 * Writes alias configuration file.
 */

int
alias_config_write ()
{
    return dogechat_config_write (alias_config_file);
}
