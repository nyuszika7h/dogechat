/*
 * ##########################################################################
 * ##          ___       __         ______________        _____            ##
 * ##          __ |     / /___________  ____/__  /_______ __  /_           ##
 * ##          __ | /| / /_  _ \  _ \  /    __  __ \  __ `/  __/           ##
 * ##          __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_             ##
 * ##          ____/|__/  \___/\___/\____/  /_/ /_/\__,_/ \__/             ##
 * ##                                                                      ##
 * ##             DogeChat - Doge Enhanced Environment for Chat              ##
 * ##                 Fast, light, extensible chat client                  ##
 * ##                                                                      ##
 * ##             By Sébastien Helleu <flashcode@flashtux.org>             ##
 * ##                                                                      ##
 * ##                         https://dogechat.org/                         ##
 * ##                                                                      ##
 * ##########################################################################
 *
 * dogechat.c - DogeChat main functions
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "dogechat.h"
#include "doge-command.h"
#include "doge-completion.h"
#include "doge-config.h"
#include "doge-debug.h"
#include "doge-eval.h"
#include "doge-hdata.h"
#include "doge-hook.h"
#include "doge-log.h"
#include "doge-network.h"
#include "doge-proxy.h"
#include "doge-secure.h"
#include "doge-string.h"
#include "doge-upgrade.h"
#include "doge-utf8.h"
#include "doge-util.h"
#include "doge-version.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-main.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-api.h"


int dogechat_debug_core = 0;            /* debug level for core              */
char *dogechat_argv0 = NULL;            /* DogeChat binary file name (argv[0])*/
int dogechat_upgrading = 0;             /* =1 if DogeChat is upgrading        */
int dogechat_first_start = 0;           /* first start of DogeChat?           */
time_t dogechat_first_start_time = 0;   /* start time (used by /uptime cmd)  */
int dogechat_upgrade_count = 0;         /* number of /upgrade done           */
struct timeval dogechat_current_start_timeval; /* start time used to display */
                                              /* duration of /upgrade       */
int dogechat_quit = 0;                  /* = 1 if quit request from user     */
int dogechat_sigsegv = 0;               /* SIGSEGV received?                 */
char *dogechat_home = NULL;             /* home dir. (default: ~/.dogechat)   */
int dogechat_locale_ok = 0;             /* is locale OK?                     */
char *dogechat_local_charset = NULL;    /* example: ISO-8859-1, UTF-8        */
int dogechat_server_cmd_line = 0;       /* at least 1 server on cmd line     */
int dogechat_auto_load_plugins = 1;     /* auto load plugins                 */
int dogechat_plugin_no_dlclose = 0;     /* remove calls to dlclose for libs  */
                                       /* (useful with valgrind)            */
int dogechat_no_gnutls = 0;             /* remove init/deinit of gnutls      */
                                       /* (useful with valgrind/electric-f.)*/
int dogechat_no_gcrypt = 0;             /* remove init/deinit of gcrypt      */
                                       /* (useful with valgrind)            */
char *dogechat_startup_commands = NULL; /* startup commands (-r flag)        */


/*
 * Displays DogeChat copyright on standard output.
 */

void
dogechat_display_copyright ()
{
    string_iconv_fprintf (stdout, "\n");
    string_iconv_fprintf (
        stdout,
        /* TRANSLATORS: "%s %s" after "compiled on" is date and time */
        _("DogeChat %s Copyright %s, compiled on %s %s\n"
          "Developed by Sébastien Helleu <flashcode@flashtux.org> "
          "- %s"),
        version_get_version_with_git (),
        DOGECHAT_COPYRIGHT_DATE,
        version_get_compilation_date (),
        version_get_compilation_time (),
        DOGECHAT_WEBSITE);
    string_iconv_fprintf (stdout, "\n");
}

/*
 * Displays DogeChat usage on standard output.
 */

void
dogechat_display_usage (char *exec_name)
{
    dogechat_display_copyright ();
    string_iconv_fprintf (stdout, "\n");
    string_iconv_fprintf (stdout,
                          _("Usage: %s [option...] [plugin:option...]\n"),
                          exec_name, exec_name);
    string_iconv_fprintf (stdout, "\n");
    string_iconv_fprintf (
        stdout,
        _("  -a, --no-connect         disable auto-connect to servers at "
          "startup\n"
          "  -c, --colors             display default colors in terminal\n"
          "  -d, --dir <path>         set DogeChat home directory "
          "(default: ~/.dogechat)\n"
          "                           (environment variable DOGECHAT_HOME is "
          "read if this option is not given)\n"
          "  -h, --help               display this help\n"
          "  -l, --license            display DogeChat license\n"
          "  -p, --no-plugin          don't load any plugin at startup\n"
          "  -r, --run-command <cmd>  run command(s) after startup\n"
          "                           (many commands can be separated by "
          "semicolons)\n"
          "  -s, --no-script          don't load any script at startup\n"
          "      --upgrade            upgrade DogeChat using session files "
          "(see /help upgrade in DogeChat)\n"
          "  -v, --version            display DogeChat version\n"
          "  plugin:option            option for plugin (see man dogechat)\n"));
    string_iconv_fprintf(stdout, "\n");
}

/*
 * Parses command line arguments.
 *
 * Arguments argc and argv come from main() function.
 */

void
dogechat_parse_args (int argc, char *argv[])
{
    int i;

    dogechat_argv0 = (argv && argv[0]) ? strdup (argv[0]) : NULL;
    dogechat_upgrading = 0;
    dogechat_home = NULL;
    dogechat_server_cmd_line = 0;
    dogechat_auto_load_plugins = 1;
    dogechat_plugin_no_dlclose = 0;

    for (i = 1; i < argc; i++)
    {
        if ((strcmp (argv[i], "-c") == 0)
            || (strcmp (argv[i], "--colors") == 0))
        {
            gui_color_display_terminal_colors ();
            dogechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-d") == 0)
            || (strcmp (argv[i], "--dir") == 0))
        {
            if (i + 1 < argc)
            {
                if (dogechat_home)
                    free (dogechat_home);
                dogechat_home = strdup (argv[++i]);
            }
            else
            {
                string_iconv_fprintf (stderr,
                                      _("Error: missing argument for \"%s\" "
                                        "option\n"),
                                      argv[i]);
                dogechat_shutdown (EXIT_FAILURE, 0);
            }
        }
        else if ((strcmp (argv[i], "-h") == 0)
                || (strcmp (argv[i], "--help") == 0))
        {
            dogechat_display_usage (argv[0]);
            dogechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-l") == 0)
                 || (strcmp (argv[i], "--license") == 0))
        {
            dogechat_display_copyright ();
            string_iconv_fprintf (stdout, "\n");
            string_iconv_fprintf (stdout, "%s%s", DOGECHAT_LICENSE_TEXT);
            dogechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if (strcmp (argv[i], "--no-dlclose") == 0)
        {
            /*
             * Valgrind works better when dlclose() is not done after plugins
             * are unloaded, it can display stack for plugins,* otherwise
             * you'll see "???" in stack for functions of unloaded plugins.
             * This option disables the call to dlclose(),
             * it must NOT be used for other purposes!
             */
            dogechat_plugin_no_dlclose = 1;
        }
        else if (strcmp (argv[i], "--no-gnutls") == 0)
        {
            /*
             * Electric-fence is not working fine when gnutls loads
             * certificates and Valgrind reports many memory errors with
             * gnutls.
             * This option disables the init/deinit of gnutls,
             * it must NOT be used for other purposes!
             */
            dogechat_no_gnutls = 1;
        }
        else if (strcmp (argv[i], "--no-gcrypt") == 0)
        {
            /*
             * Valgrind reports many memory errors with gcrypt.
             * This option disables the init/deinit of gcrypt,
             * it must NOT be used for other purposes!
             */
            dogechat_no_gcrypt = 1;
        }
        else if ((strcmp (argv[i], "-p") == 0)
                 || (strcmp (argv[i], "--no-plugin") == 0))
        {
            dogechat_auto_load_plugins = 0;
        }
        else if ((strcmp (argv[i], "-r") == 0)
                 || (strcmp (argv[i], "--run-command") == 0))
        {
            if (i + 1 < argc)
            {
                if (dogechat_startup_commands)
                    free (dogechat_startup_commands);
                dogechat_startup_commands = strdup (argv[++i]);
            }
            else
            {
                string_iconv_fprintf (stderr,
                                      _("Error: missing argument for \"%s\" "
                                        "option\n"),
                                      argv[i]);
                dogechat_shutdown (EXIT_FAILURE, 0);
            }
        }
        else if (strcmp (argv[i], "--upgrade") == 0)
        {
            dogechat_upgrading = 1;
        }
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
        {
            string_iconv_fprintf (stdout, version_get_version ());
            fprintf (stdout, "\n");
            dogechat_shutdown (EXIT_SUCCESS, 0);
        }
    }
}

/*
 * Expands and assigns given path to "dogechat_home".
 */

void
dogechat_set_home_path (char *home_path)
{
    char *ptr_home;
    int dir_length;

    if (home_path[0] == '~')
    {
        /* replace leading '~' by $HOME */
        ptr_home = getenv ("HOME");
        if (!ptr_home)
        {
            string_iconv_fprintf (stderr,
                                  _("Error: unable to get HOME directory\n"));
            dogechat_shutdown (EXIT_FAILURE, 0);
            /* make C static analyzer happy (never executed) */
            return;
        }
        dir_length = strlen (ptr_home) + strlen (home_path + 1) + 1;
        dogechat_home = malloc (dir_length);
        if (dogechat_home)
        {
            snprintf (dogechat_home, dir_length,
                      "%s%s", ptr_home, home_path + 1);
        }
    }
    else
    {
        dogechat_home = strdup (home_path);
    }

    if (!dogechat_home)
    {
        string_iconv_fprintf (stderr,
                              _("Error: not enough memory for home "
                                "directory\n"));
        dogechat_shutdown (EXIT_FAILURE, 0);
        /* make C static analyzer happy (never executed) */
        return;
    }
}

/*
 * Creates DogeChat home directory (by default ~/.dogechat).
 *
 * Any error in this function is fatal: DogeChat can not run without a home
 * directory.
 */

void
dogechat_create_home_dir ()
{
    char *ptr_dogechat_home, *config_dogechat_home;
    struct stat statinfo;

    /*
     * dogechat_home is not set yet: look for environment variable
     * "DOGECHAT_HOME"
     */
    if (!dogechat_home)
    {
        ptr_dogechat_home = getenv ("DOGECHAT_HOME");
        if (ptr_dogechat_home && ptr_dogechat_home[0])
            dogechat_set_home_path (ptr_dogechat_home);
    }

    /* dogechat_home is still not set: try to use compile time default */
    if (!dogechat_home)
    {
        config_dogechat_home = DOGECHAT_HOME;
        if (!config_dogechat_home[0])
        {
            string_iconv_fprintf (stderr,
                                  _("Error: DOGECHAT_HOME is undefined, check "
                                    "build options\n"));
            dogechat_shutdown (EXIT_FAILURE, 0);
            /* make C static analyzer happy (never executed) */
            return;
        }
        dogechat_set_home_path (config_dogechat_home);
    }

    /* if home already exists, it has to be a directory */
    if (stat (dogechat_home, &statinfo) == 0)
    {
        if (!S_ISDIR (statinfo.st_mode))
        {
            string_iconv_fprintf (stderr,
                                  _("Error: home (%s) is not a directory\n"),
                                  dogechat_home);
            dogechat_shutdown (EXIT_FAILURE, 0);
            /* make C static analyzer happy (never executed) */
            return;
        }
    }

    /* create home directory; error is fatal */
    if (!util_mkdir (dogechat_home, 0755))
    {
        string_iconv_fprintf (stderr,
                              _("Error: cannot create directory \"%s\"\n"),
                              dogechat_home);
        dogechat_shutdown (EXIT_FAILURE, 0);
        /* make C static analyzer happy (never executed) */
        return;
    }
}

/*
 * Displays DogeChat startup message.
 */

void
dogechat_startup_message ()
{
    if (CONFIG_BOOLEAN(config_startup_display_logo))
    {

        gui_chat_printf (
            NULL,
            "%s      ____                   ________          __\n"
            "%s     / __ \\____  ____ ____  / ____/ /_  ____ _/ /_\n"
            "%s    / / / / __ \\/ __ `/ _ \\/ /   / __ \\/ __ `/ __/\n"
            "%s   / /_/ / /_/ / /_/ /  __/ /___/ / / / /_/ / /_\n"
            "%s  /_____/\\____/\\__, /\\___/\\____/_/ /_/\\__,_/\\__/\n"
            "%s              /____/\n",
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK));
    }
    if (CONFIG_BOOLEAN(config_startup_display_version))
    {
        command_version_display (NULL, 0, 0, 0);
    }
    if (CONFIG_BOOLEAN(config_startup_display_logo) ||
        CONFIG_BOOLEAN(config_startup_display_version))
    {
        gui_chat_printf (
            NULL,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    }

    if (dogechat_first_start)
    {
        /* message on first run (when dogechat.conf is created) */
        gui_chat_printf (NULL, "");
        gui_chat_printf (
            NULL,
            _("Welcome to DogeChat!\n"
              "\n"
              "If you are discovering DogeChat, it is recommended to read at "
              "least the quickstart guide, and the user's guide if you have "
              "some time; they explain main DogeChat concepts.\n"
              "All DogeChat docs are available at: https://dogechat.org/doc\n"
              "\n"
              "Moreover, there is inline help with /help on all commands and "
              "options (use Tab key to complete the name).\n"
              "The command /iset (script iset.pl) can help to customize "
              "DogeChat: /script install iset.pl\n"
              "\n"
              "You can add and connect to an IRC server with /server and "
              "/connect commands (see /help server)."));
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, "---");
        gui_chat_printf (NULL, "");
    }
}

/*
 * Displays warnings about $TERM if it is detected as wrong.
 *
 * If $TERM is different from "screen" or "screen-256color" and that $STY is
 * set (GNU screen) or $TMUX is set (tmux), then a warning is displayed.
 */

void
dogechat_term_check ()
{
    char *term, *sty, *tmux;
    const char *screen_terms = "screen-256color, screen";
    const char *tmux_terms = "tmux-256color, tmux, screen-256color, screen";
    const char *ptr_terms;
    int is_term_ok, is_screen, is_tmux;

    term = getenv ("TERM");
    sty = getenv ("STY");
    tmux = getenv ("TMUX");

    is_screen = (sty && sty[0]);
    is_tmux = (tmux && tmux[0]);

    if (is_screen || is_tmux)
    {
        /* check if $TERM is OK (according to screen/tmux) */
        is_term_ok = 0;
        ptr_terms = NULL;
        if (is_screen)
        {
            is_term_ok = (term && (strncmp (term, "screen", 6) == 0));
            ptr_terms = screen_terms;
        }
        else if (is_tmux)
        {
            is_term_ok = (term
                          && ((strncmp (term, "screen", 6) == 0)
                              || (strncmp (term, "tmux", 4) == 0)));
            ptr_terms = tmux_terms;
        }

        /* display a warning if $TERM is NOT OK */
        if (!is_term_ok)
        {
            gui_chat_printf_date_tags (
                NULL,
                0,
                "term_warning",
                /* TRANSLATORS: the "under %s" can be "under screen" or "under tmux" */
                _("%sWarning: DogeChat is running under %s and $TERM is \"%s\", "
                  "which can cause display bugs; $TERM should be set to one "
                  "of these values: %s"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                (is_screen) ? "screen" : "tmux",
                (term) ? term : "",
                ptr_terms);
            gui_chat_printf_date_tags (
                NULL,
                0,
                "term_warning",
                _("%sYou should add this line in the file %s:  %s"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                (is_screen) ? "~/.screenrc" : "~/.tmux.conf",
                (is_screen) ?
                "term screen-256color" :
                "set -g default-terminal \"tmux-256color\"");
        }
    }
}

/*
 * Displays warning about wrong locale ($LANG and $LC_*) if they are detected
 * as wrong.
 */

void
dogechat_locale_check ()
{
    if (!dogechat_locale_ok)
    {
        gui_chat_printf (
            NULL,
            _("%sWarning: cannot set the locale; make sure $LANG and $LC_* "
              "variables are correct"),
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    }
}

/*
 * Callback for system signal SIGHUP: quits DogeChat.
 */

void
dogechat_sighup ()
{
    int rc;

    rc = hook_signal_send ("signal_sighup",
                           DOGECHAT_HOOK_SIGNAL_STRING, NULL);
    if (rc != DOGECHAT_RC_OK_EAT)
    {
        log_printf (_("Signal %s received, exiting DogeChat..."), "SIGHUP");
        (void) hook_signal_send ("quit", DOGECHAT_HOOK_SIGNAL_STRING, NULL);
        dogechat_quit = 1;
    }
}

/*
 * Callback for system signal SIGQUIT: quits DogeChat.
 */

void
dogechat_sigquit ()
{
    int rc;

    rc = hook_signal_send ("signal_sigquit",
                           DOGECHAT_HOOK_SIGNAL_STRING, NULL);
    if (rc != DOGECHAT_RC_OK_EAT)
    {
        log_printf (_("Signal %s received, exiting DogeChat..."), "SIGQUIT");
        (void) hook_signal_send ("quit", DOGECHAT_HOOK_SIGNAL_STRING, NULL);
        dogechat_quit = 1;
    }
}

/*
 * Callback for system signal SIGTERM: quits DogeChat.
 */

void
dogechat_sigterm ()
{
    int rc;

    rc = hook_signal_send ("signal_sigterm",
                           DOGECHAT_HOOK_SIGNAL_STRING, NULL);
    if (rc != DOGECHAT_RC_OK_EAT)
    {
        log_printf (_("Signal %s received, exiting DogeChat..."), "SIGTERM");
        (void) hook_signal_send ("quit", DOGECHAT_HOOK_SIGNAL_STRING, NULL);
        dogechat_quit = 1;
    }
}

/*
 * Shutdowns DogeChat.
 */

void
dogechat_shutdown (int return_code, int crash)
{
    gui_chat_print_lines_waiting_buffer (stderr);

    log_close ();
    network_end ();
    debug_end ();

    if (dogechat_argv0)
        free (dogechat_argv0);
    if (dogechat_home)
        free (dogechat_home);
    if (dogechat_local_charset)
        free (dogechat_local_charset);

    if (crash)
        abort();
    else if (return_code >= 0)
        exit (return_code);
}

/*
 * Initializes DogeChat.
 */

void
dogechat_init (int argc, char *argv[], void (*gui_init_cb)())
{
    dogechat_first_start_time = time (NULL); /* initialize start time        */
    gettimeofday (&dogechat_current_start_timeval, NULL);

    dogechat_locale_ok = (setlocale (LC_ALL, "") != NULL);   /* init gettext */
#ifdef ENABLE_NLS
    bindtextdomain (PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain (PACKAGE);
#endif /* ENABLE_NLS */

#ifdef HAVE_LANGINFO_CODESET
    dogechat_local_charset = strdup (nl_langinfo (CODESET));
#else
    dogechat_local_charset = strdup ("");
#endif /* HAVE_LANGINFO_CODESET */
    utf8_init ();

    /* catch signals */
    util_catch_signal (SIGINT, SIG_IGN);           /* signal ignored        */
    util_catch_signal (SIGQUIT, SIG_IGN);          /* signal ignored        */
    util_catch_signal (SIGPIPE, SIG_IGN);          /* signal ignored        */
    util_catch_signal (SIGSEGV, &debug_sigsegv);   /* crash dump            */
    util_catch_signal (SIGHUP, &dogechat_sighup);   /* exit DogeChat          */
    util_catch_signal (SIGQUIT, &dogechat_sigquit); /* exit DogeChat          */
    util_catch_signal (SIGTERM, &dogechat_sigterm); /* exit DogeChat          */

    hdata_init ();                      /* initialize hdata                 */
    hook_init ();                       /* initialize hooks                 */
    debug_init ();                      /* hook signals for debug           */
    gui_color_init ();                  /* initialize colors                */
    gui_chat_init ();                   /* initialize chat                  */
    command_init ();                    /* initialize DogeChat commands      */
    completion_init ();                 /* add core completion hooks        */
    gui_key_init ();                    /* init keys                        */
    network_init_gcrypt ();             /* init gcrypt                      */
    if (!secure_init ())                /* init secured data options (sec.*)*/
        dogechat_shutdown (EXIT_FAILURE, 0);
    if (!config_dogechat_init ())        /* init DogeChat options (dogechat.*) */
        dogechat_shutdown (EXIT_FAILURE, 0);
    dogechat_parse_args (argc, argv);    /* parse command line args          */
    dogechat_create_home_dir ();         /* create DogeChat home directory    */
    log_init ();                        /* init log file                    */
    plugin_api_init ();                 /* create some hooks (info,hdata,..)*/
    secure_read ();                     /* read secured data options        */
    config_dogechat_read ();             /* read DogeChat options             */
    network_init_gnutls ();             /* init GnuTLS                      */

    if (gui_init_cb)
        (*gui_init_cb) ();              /* init DogeChat interface           */

    if (dogechat_upgrading)
    {
        upgrade_dogechat_load ();        /* upgrade with session file        */
        dogechat_upgrade_count++;        /* increase /upgrade count          */
    }
    dogechat_startup_message ();         /* display DogeChat startup message  */
    gui_chat_print_lines_waiting_buffer (NULL); /* display lines waiting    */
    dogechat_term_check ();              /* warning about wrong $TERM        */
    dogechat_locale_check ();            /* warning about wrong locale       */
    command_startup (0);                /* command executed before plugins  */
    plugin_init (dogechat_auto_load_plugins, /* init plugin interface(s)     */
                 argc, argv);
    command_startup (1);                /* commands executed after plugins  */
    if (!dogechat_upgrading)
        gui_layout_window_apply (gui_layout_current, -1);
    if (dogechat_upgrading)
        upgrade_dogechat_end ();         /* remove .upgrade files + signal   */
}

/*
 * Ends DogeChat.
 */

void
dogechat_end (void (*gui_end_cb)(int clean_exit))
{
    gui_layout_store_on_exit ();        /* store layout                     */
    plugin_end ();                      /* end plugin interface(s)          */
    if (CONFIG_BOOLEAN(config_look_save_config_on_exit))
        (void) config_dogechat_write (); /* save DogeChat config file         */
    (void) secure_write ();             /* save secured data                */

    if (gui_end_cb)
        (*gui_end_cb) (1);              /* shut down DogeChat GUI            */

    proxy_free_all ();                  /* free all proxies                 */
    config_dogechat_free ();             /* free DogeChat options             */
    secure_free ();                     /* free secured data options        */
    config_file_free_all ();            /* free all configuration files     */
    gui_key_end ();                     /* remove all keys                  */
    unhook_all ();                      /* remove all hooks                 */
    hdata_end ();                       /* end hdata                        */
    secure_end ();                      /* end secured data                 */
    string_end ();                      /* end string                       */
    dogechat_shutdown (-1, 0);           /* end other things                 */
}
