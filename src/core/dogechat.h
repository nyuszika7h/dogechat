/*
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

#ifndef DOGECHAT_H
#define DOGECHAT_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/time.h>

#include <locale.h>

#if defined(ENABLE_NLS) && !defined(_)
    #ifdef HAVE_LIBINTL_H
        #include <libintl.h>
    #else
        #include "../../intl/libintl.h"
    #endif /* HAVE_LIBINTL_H */
    #define _(string) gettext(string)
    #define NG_(single,plural,number) ngettext(single,plural,number)
    #ifdef gettext_noop
        #define N_(string) gettext_noop(string)
    #else
        #define N_(string) (string)
    #endif /* gettext_noop */
#endif /* defined(ENABLE_NLS) && !defined(_) */
#if !defined(_)
    #define _(string) (string)
    #define NG_(single,plural,number) (plural)
    #define N_(string) (string)
#endif /* !defined(_) */


#define DOGECHAT_COPYRIGHT_DATE   "(C) 2003-2016"
#define DOGECHAT_WEBSITE          "https://dogechat.org/"
#define DOGECHAT_WEBSITE_DOWNLOAD "https://dogechat.org/download"

/* log file */
#define DOGECHAT_LOG_NAME "dogechat.log"

/* license */
#define DOGECHAT_LICENSE_TEXT \
    "DogeChat is free software; you can redistribute it and/or modify\n" \
    "it under the terms of the GNU General Public License as published by\n" \
    "the Free Software Foundation; either version 3 of the License, or\n" \
    "(at your option) any later version.\n" \
    "\n", \
    \
    "DogeChat is distributed in the hope that it will be useful,\n" \
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
    "GNU General Public License for more details.\n" \
    "\n" \
    "You should have received a copy of the GNU General Public License\n" \
    "along with DogeChat.  If not, see <http://www.gnu.org/licenses/>.\n\n"

/* directory separator, depending on OS */
#ifdef _WIN32
    #define DIR_SEPARATOR       "\\"
    #define DIR_SEPARATOR_CHAR  '\\'
#else
    #define DIR_SEPARATOR       "/"
    #define DIR_SEPARATOR_CHAR  '/'
#endif /* _WIN32 */

/* some systems like GNU/Hurd do not define PATH_MAX */
#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif /* PATH_MAX */

/* internal charset */
#define DOGECHAT_INTERNAL_CHARSET "UTF-8"

/* global variables and functions */
extern int dogechat_debug_core;
extern char *dogechat_argv0;
extern int dogechat_upgrading;
extern int dogechat_first_start;
extern time_t dogechat_first_start_time;
extern struct timeval dogechat_current_start_timeval;
extern int dogechat_upgrade_count;
extern int dogechat_quit;
extern char *dogechat_home;
extern char *dogechat_local_charset;
extern int dogechat_plugin_no_dlclose;
extern int dogechat_no_gnutls;
extern int dogechat_no_gcrypt;
extern char *dogechat_startup_commands;

extern void dogechat_term_check ();
extern void dogechat_shutdown (int return_code, int crash);
extern void dogechat_init (int argc, char *argv[], void (*gui_init_cb)());
extern void dogechat_end (void (*gui_end_cb)(int clean_exit));

#endif /* DOGECHAT_H */
