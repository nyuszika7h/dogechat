/*
 * main.c - entry point for Curses GUI
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

#include <stdlib.h>

#include "../../core/dogechat.h"
#include "../gui-main.h"
#include "gui-curses.h"


/*
 * Entry point for DogeChat (Curses GUI).
 */

int
main (int argc, char *argv[])
{
    dogechat_init (argc, argv, &gui_main_init);
    gui_main_loop ();
    dogechat_end (&gui_main_end);

    return EXIT_SUCCESS;
}
