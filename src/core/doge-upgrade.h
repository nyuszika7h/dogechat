/*
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

#ifndef DOGECHAT_UPGRADE_H
#define DOGECHAT_UPGRADE_H 1

#include "doge-upgrade-file.h"

#define DOGECHAT_UPGRADE_FILENAME "dogechat"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_upgrade_dogechat_type
{
    UPGRADE_DOGECHAT_TYPE_HISTORY = 0,
    UPGRADE_DOGECHAT_TYPE_BUFFER,
    UPGRADE_DOGECHAT_TYPE_NICKLIST,
    UPGRADE_DOGECHAT_TYPE_BUFFER_LINE,
    UPGRADE_DOGECHAT_TYPE_MISC,
    UPGRADE_DOGECHAT_TYPE_HOTLIST,
    UPGRADE_DOGECHAT_TYPE_LAYOUT_WINDOW,
};

int upgrade_dogechat_save ();
int upgrade_dogechat_load ();
void upgrade_dogechat_end ();

#endif /* DOGECHAT_UPGRADE_H */
