/*
 * fifo.c - fifo plugin for DogeChat: remote control with FIFO pipe
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

#include "../dogechat-plugin.h"
#include "fifo.h"
#include "fifo-command.h"
#include "fifo-info.h"


DOGECHAT_PLUGIN_NAME(FIFO_PLUGIN_NAME);
DOGECHAT_PLUGIN_DESCRIPTION(N_("FIFO pipe for remote control"));
DOGECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
DOGECHAT_PLUGIN_VERSION(DOGECHAT_VERSION);
DOGECHAT_PLUGIN_LICENSE(DOGECHAT_LICENSE);
DOGECHAT_PLUGIN_PRIORITY(7000);

#define FIFO_FILENAME_PREFIX "dogechat_fifo_"

struct t_dogechat_plugin *dogechat_fifo_plugin = NULL;
#define dogechat_plugin dogechat_fifo_plugin

int fifo_quiet = 0;
int fifo_fd = -1;
struct t_hook *fifo_fd_hook = NULL;
char *fifo_filename;
char *fifo_unterminated = NULL;


int fifo_read();


/*
 * Removes old FIFO pipes in directory.
 */

void
fifo_remove_old_pipes ()
{
    char *buf;
    int buf_len, prefix_len;
    const char *dogechat_home, *dir_separator;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    buf_len = PATH_MAX;
    buf = malloc (buf_len);
    if (!buf)
        return;

    dogechat_home = dogechat_info_get ("dogechat_dir", "");
    dir_separator = dogechat_info_get ("dir_separator", "");

    prefix_len = strlen (FIFO_FILENAME_PREFIX);

    dp = opendir (dogechat_home);
    if (dp != NULL)
    {
        while ((entry = readdir (dp)) != NULL)
        {
            if (strcmp (entry->d_name, ".") == 0 || strcmp (entry->d_name, "..") == 0)
                continue;

            if (strncmp (entry->d_name, FIFO_FILENAME_PREFIX, prefix_len) == 0)
            {
                snprintf (buf, buf_len, "%s%s%s",
                          dogechat_home, dir_separator, entry->d_name);
                if (stat (buf, &statbuf) != -1)
                {
                    dogechat_printf (NULL,
                                    _("%s: removing old fifo pipe \"%s\""),
                                    FIFO_PLUGIN_NAME, buf);
                    unlink (buf);
                }
            }
        }
        closedir (dp);
    }

    free (buf);
}

/*
 * Creates FIFO pipe for remote control.
 */

void
fifo_create ()
{
    int filename_length;
    const char *fifo_option, *dogechat_home;

    fifo_option = dogechat_config_get_plugin ("fifo");
    if (!fifo_option)
    {
        dogechat_config_set_plugin ("fifo", "on");
        fifo_option = dogechat_config_get_plugin ("fifo");
    }

    dogechat_home = dogechat_info_get ("dogechat_dir", "");

    if (fifo_option && dogechat_home)
    {
        fifo_remove_old_pipes ();

        if (dogechat_strcasecmp (fifo_option, "on") == 0)
        {
            /*
             * build FIFO filename:
             *   "<dogechat_home>/dogechat_fifo_" + process PID
             */
            if (!fifo_filename)
            {
                filename_length = strlen (dogechat_home) + 64;
                fifo_filename = malloc (filename_length);
                snprintf (fifo_filename, filename_length,
                          "%s/%s%d",
                          dogechat_home, FIFO_FILENAME_PREFIX, (int) getpid());
            }

            fifo_fd = -1;

            /* create FIFO pipe, writable for user only */
            if (mkfifo (fifo_filename, 0600) == 0)
            {
                /* open FIFO pipe in read-only, non-blocking mode */
                if ((fifo_fd = open (fifo_filename,
                                     O_RDONLY | O_NONBLOCK)) != -1)
                {
                    if ((dogechat_fifo_plugin->debug >= 1) || !fifo_quiet)
                    {
                        dogechat_printf (NULL,
                                        _("%s: pipe opened (file: %s)"),
                                        FIFO_PLUGIN_NAME,
                                        fifo_filename);
                    }
                    fifo_fd_hook = dogechat_hook_fd (fifo_fd, 1, 0, 0,
                                                    &fifo_read, NULL);
                }
                else
                    dogechat_printf (NULL,
                                    _("%s%s: unable to open pipe (%s) for "
                                      "reading"),
                                    dogechat_prefix ("error"), FIFO_PLUGIN_NAME,
                                    fifo_filename);
            }
            else
            {
                dogechat_printf (NULL,
                                _("%s%s: unable to create pipe for remote "
                                  "control (%s): error %d %s"),
                                dogechat_prefix ("error"), FIFO_PLUGIN_NAME,
                                fifo_filename, errno, strerror (errno));
            }
        }
    }
}

/*
 * Removes FIFO pipe.
 */

void
fifo_remove ()
{
    /* remove fd hook */
    if (fifo_fd_hook)
    {
        dogechat_unhook (fifo_fd_hook);
        fifo_fd_hook = NULL;
    }

    /* close FIFO pipe */
    if (fifo_fd != -1)
    {
        close (fifo_fd);
        fifo_fd = -1;
    }

    /* remove FIFO from disk */
    if (fifo_filename)
        unlink (fifo_filename);

    /* remove any unterminated message */
    if (fifo_unterminated)
    {
        free (fifo_unterminated);
        fifo_unterminated = NULL;
    }

    /* free filename */
    if (fifo_filename)
    {
        free (fifo_filename);
        fifo_filename = NULL;
    }

    dogechat_printf (NULL,
                    _("%s: pipe closed"),
                    FIFO_PLUGIN_NAME);
}

/*
 * Executes a command/text received in FIFO pipe.
 */

void
fifo_exec (const char *text)
{
    char *text2, *pos_msg;
    struct t_gui_buffer *ptr_buffer;

    text2 = strdup (text);
    if (!text2)
        return;

    pos_msg = NULL;
    ptr_buffer = NULL;

    /*
     * look for plugin + buffer name at beginning of text
     * text may be: "plugin.buffer *text" or "*text"
     */
    if (text2[0] == '*')
    {
        pos_msg = text2 + 1;
        ptr_buffer = dogechat_current_buffer ();
    }
    else
    {
        pos_msg = strstr (text2, " *");
        if (!pos_msg)
        {
            dogechat_printf (NULL,
                            _("%s%s: invalid text received in pipe"),
                            dogechat_prefix ("error"), FIFO_PLUGIN_NAME);
            free (text2);
            return;
        }
        pos_msg[0] = '\0';
        pos_msg += 2;
        ptr_buffer = dogechat_buffer_search ("==", text2);
        if (!ptr_buffer)
        {
            dogechat_printf (NULL,
                            _("%s%s: buffer \"%s\" not found"),
                            dogechat_prefix ("error"), FIFO_PLUGIN_NAME,
                            text2);
            free (text2);
            return;
        }
    }

    dogechat_command (ptr_buffer, pos_msg);

    free (text2);
}

/*
 * Reads data in FIFO pipe.
 */

int
fifo_read (void *data, int fd)
{
    static char buffer[4096 + 2];
    char *buf2, *pos, *ptr_buf, *next_ptr_buf;
    int num_read;

    /* make C compiler happy */
    (void) data;
    (void) fd;

    num_read = read (fifo_fd, buffer, sizeof (buffer) - 2);
    if (num_read > 0)
    {
        buffer[num_read] = '\0';

        buf2 = NULL;
        ptr_buf = buffer;
        if (fifo_unterminated)
        {
            buf2 = malloc (strlen (fifo_unterminated) +
                           strlen (buffer) + 1);
            if (buf2)
            {
                strcpy (buf2, fifo_unterminated);
                strcat (buf2, buffer);
            }
            ptr_buf = buf2;
            free (fifo_unterminated);
            fifo_unterminated = NULL;
        }

        while (ptr_buf && ptr_buf[0])
        {
            next_ptr_buf = NULL;
            pos = strstr (ptr_buf, "\r\n");
            if (pos)
            {
                pos[0] = '\0';
                next_ptr_buf = pos + 2;
            }
            else
            {
                pos = strstr (ptr_buf, "\n");
                if (pos)
                {
                    pos[0] = '\0';
                    next_ptr_buf = pos + 1;
                }
                else
                {
                    fifo_unterminated = strdup (ptr_buf);
                    ptr_buf = NULL;
                    next_ptr_buf = NULL;
                }
            }

            if (ptr_buf)
                fifo_exec (ptr_buf);

            ptr_buf = next_ptr_buf;
        }

        if (buf2)
            free (buf2);
    }
    else
    {
        if (num_read < 0)
        {
#ifdef __CYGWIN__
            if ((errno == EAGAIN) || (errno == ECOMM))
#else
            if (errno == EAGAIN)
#endif /* __CYGWIN__ */
                return DOGECHAT_RC_OK;

            dogechat_printf (NULL,
                            _("%s%s: error reading pipe (%d %s), closing it"),
                            dogechat_prefix ("error"), FIFO_PLUGIN_NAME,
                            errno, strerror (errno));
            fifo_remove ();
        }
        else
        {
            dogechat_unhook (fifo_fd_hook);
            fifo_fd_hook = NULL;
            close (fifo_fd);
            fifo_fd = open (fifo_filename, O_RDONLY | O_NONBLOCK);
            if (fifo_fd < 0)
            {
                dogechat_printf (NULL,
                                _("%s%s: error opening file, closing it"),
                                dogechat_prefix ("error"), FIFO_PLUGIN_NAME);
                fifo_remove ();
            }
            else
                fifo_fd_hook = dogechat_hook_fd (fifo_fd, 1, 0, 0,
                                                &fifo_read, NULL);
        }
    }

    return DOGECHAT_RC_OK;
}

/*
 * Callback for changes on option "plugins.var.fifo.fifo".
 */

int
fifo_config_cb (void *data, const char *option, const char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (dogechat_strcasecmp (value, "on") == 0)
    {
        if (fifo_fd < 0)
            fifo_create ();
    }
    else
    {
        if (fifo_fd >= 0)
            fifo_remove ();
    }

    return DOGECHAT_RC_OK;
}

/*
 * Initializes fifo plugin.
 */

int
dogechat_plugin_init (struct t_dogechat_plugin *plugin, int argc, char *argv[])
{
    char str_option[256];

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    dogechat_plugin = plugin;

    fifo_quiet = 1;

    fifo_create ();

    snprintf (str_option, sizeof (str_option),
              "plugins.var.fifo.%s", FIFO_OPTION_NAME);
    dogechat_hook_config (str_option, &fifo_config_cb, NULL);

    fifo_command_init ();
    fifo_info_init ();

    fifo_quiet = 0;

    return DOGECHAT_RC_OK;
}

/*
 * Ends fifo plugin.
 */

int
dogechat_plugin_end (struct t_dogechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    fifo_remove ();

    return DOGECHAT_RC_OK;
}
