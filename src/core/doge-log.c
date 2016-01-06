/*
 * doge-log.c - DogeChat log file (dogechat.log)
 *
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#ifdef HAVE_FLOCK
#include <sys/file.h>
#endif

#include <sys/types.h>
#include <time.h>

#include "dogechat.h"
#include "doge-log.h"
#include "doge-debug.h"
#include "doge-string.h"
#include "doge-version.h"
#include "../plugins/plugin.h"


char *dogechat_log_filename = NULL; /* log name (~/.dogechat/dogechat.log)     */
FILE *dogechat_log_file = NULL;     /* DogeChat log file                      */
int dogechat_log_use_time = 1;      /* 0 to temporary disable time in log,   */
                                   /* for example when dumping data         */


/*
 * Opens the DogeChat log file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
log_open (const char *filename, const char *mode)
{
    int filename_length;

    /* exit if log already opened */
    if (dogechat_log_file)
        return 0;

    if (filename)
        dogechat_log_filename = strdup (filename);
    else
    {
        filename_length = strlen (dogechat_home) + 64;
        dogechat_log_filename = malloc (filename_length);
        snprintf (dogechat_log_filename, filename_length,
                  "%s/%s", dogechat_home, DOGECHAT_LOG_NAME);
    }

    dogechat_log_file = fopen (dogechat_log_filename, mode);
    if (!dogechat_log_file)
    {
        free (dogechat_log_filename);
        dogechat_log_filename = NULL;
        return 0;
    }

#ifdef HAVE_FLOCK
    if ((flock (fileno (dogechat_log_file), LOCK_EX | LOCK_NB) != 0))
    {
        if (errno == EWOULDBLOCK)
        {
            fclose (dogechat_log_file);
            dogechat_log_file = NULL;
            free (dogechat_log_filename);
            dogechat_log_filename = NULL;
            return 0;
        }
    }
#endif /* HAVE_FLOCK */

    return 1;
}

/*
 * Initializes the DogeChat log file.
 */

void
log_init ()
{
    if (!log_open (NULL, "w"))
    {
        string_iconv_fprintf (stderr,
                              _("Error: unable to create/append to log file (dogechat.log)\n"
                                "If another DogeChat process is using this file, try to run DogeChat\n"
                                "with another home using the \"--dir\" command line option.\n"));
        exit (1);
    }
    log_printf ("DogeChat %s (%s %s %s)",
                version_get_version_with_git (),
                _("compiled on"),
                version_get_compilation_date (),
                version_get_compilation_time ());
}

/*
 * Writes a message in DogeChat log file.
 */

void
log_printf (const char *message, ...)
{
    char *ptr_buffer;
    static time_t seconds;
    struct tm *date_tmp;

    if (!dogechat_log_file)
        return;

    dogechat_va_format (message);
    if (vbuffer)
    {
        /* keep only valid chars */
        ptr_buffer = vbuffer;
        while (ptr_buffer[0])
        {
            if ((ptr_buffer[0] != '\n')
                && (ptr_buffer[0] != '\r')
                && ((unsigned char)(ptr_buffer[0]) < 32))
                ptr_buffer[0] = '.';
            ptr_buffer++;
        }

        if (dogechat_log_use_time)
        {
            seconds = time (NULL);
            date_tmp = localtime (&seconds);
            if (date_tmp)
            {
                string_iconv_fprintf (dogechat_log_file,
                                      "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
                                      date_tmp->tm_year + 1900, date_tmp->tm_mon + 1,
                                      date_tmp->tm_mday, date_tmp->tm_hour,
                                      date_tmp->tm_min, date_tmp->tm_sec,
                                      vbuffer);
            }
            else
                string_iconv_fprintf (dogechat_log_file, "%s\n", vbuffer);
        }
        else
            string_iconv_fprintf (dogechat_log_file, "%s\n", vbuffer);

        fflush (dogechat_log_file);

        free (vbuffer);
    }
}

/*
 * Dumps a string as hexa data in DogeChat log file.
 */

void
log_printf_hexa (const char *spaces, const char *string)
{
    int msg_pos, hexa_pos, ascii_pos;
    char hexa[(16 * 3) + 1], ascii[(16 * 2) + 1];

    msg_pos = 0;
    hexa_pos = 0;
    ascii_pos = 0;
    while (string[msg_pos])
    {
        snprintf (hexa + hexa_pos, 4, "%02X ", (unsigned char)(string[msg_pos]));
        hexa_pos += 3;
        snprintf (ascii + ascii_pos, 3, "%c ",
                  ((((unsigned char)string[msg_pos]) < 32)
                   || (((unsigned char)string[msg_pos]) > 127)) ?
                  '.' : (unsigned char)(string[msg_pos]));
        ascii_pos += 2;
        if (ascii_pos == 32)
        {
            log_printf ("%s%-48s  %s", spaces, hexa, ascii);
            hexa_pos = 0;
            ascii_pos = 0;
        }
        msg_pos++;
    }
    if (ascii_pos > 0)
        log_printf ("%s%-48s  %s", spaces, hexa, ascii);
}

/*
 * Closes the DogeChat log file.
 */

void
log_close ()
{
    /* close log file */
    if (dogechat_log_file)
    {
#ifdef HAVE_FLOCK
        flock (fileno (dogechat_log_file), LOCK_UN);
#endif /* HAVE_FLOCK */
        fclose (dogechat_log_file);
        dogechat_log_file = NULL;
    }

    /* free filename */
    if (dogechat_log_filename)
    {
        free (dogechat_log_filename);
        dogechat_log_filename = NULL;
    }
}

/*
 * Renames the DogeChat log file (when crashing).
 *
 * The file "dogechat.log" is renamed to "dogechat_crash_YYYYMMDD_NNNN.log",
 * where YYYYMMDD is the current date and NNNN the PID of DogeChat process.
 */

int
log_crash_rename ()
{
    char *old_name, *new_name;
    int length;
    time_t time_now;
    struct tm *local_time;

    if (!dogechat_log_filename)
        return 0;

    old_name = strdup (dogechat_log_filename);
    if (!old_name)
        return 0;

    log_close ();

    length = strlen (dogechat_home) + 128;
    new_name = malloc (length);
    if (new_name)
    {
        time_now = time (NULL);
        local_time = localtime (&time_now);
        snprintf (new_name, length,
                  "%s/dogechat_crash_%04d%02d%02d_%d.log",
                  dogechat_home,
                  local_time->tm_year + 1900,
                  local_time->tm_mon + 1,
                  local_time->tm_mday,
                  getpid());
        if (rename (old_name, new_name) == 0)
        {
            string_iconv_fprintf (stderr, "*** Full crash dump was saved to %s file.\n",
                                  new_name);
            log_open (new_name, "a");
            free (old_name);
            free (new_name);
            return 1;
        }
        free (new_name);
    }

    free (old_name);
    log_open (NULL, "a");
    return 0;
}
