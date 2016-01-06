#!/bin/sh
#
# Copyright (C) 2015-2016 Sébastien Helleu <flashcode@flashtux.org>
#
# This file is part of DogeChat, the extensible chat client.
#
# DogeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# DogeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with DogeChat.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Returns current stable or devel version of DogeChat.
#
# Syntax:
#   version.sh stable|devel|devel-full|devel-major|devel-minor|devel-patch
#
#     stable       the current stable (e.g. 1.3)
#     devel        the current devel (e.g. 1.4)
#     devel-full   the full name of devel (e.g. 1.4-dev or 1.4-rc1)
#     devel-major  the major version of devel (e.g. 1)
#     devel-minor  the minor version of devel (e.g. 4-dev)
#     devel-patch  the patch version of devel (e.g. 2 for version 1.4.2)
#

DOGECHAT_STABLE=1.3
DOGECHAT_DEVEL=1.4
DOGECHAT_DEVEL_FULL=1.4-rc1

if [ $# -lt 1 ]; then
    echo >&2 "Syntax: $0 stable|devel|devel-full|devel-major|devel-minor|devel-patch"
    exit 1
fi

case $1 in
    stable ) echo $DOGECHAT_STABLE ;;
    devel ) echo $DOGECHAT_DEVEL ;;
    devel-full ) echo $DOGECHAT_DEVEL_FULL ;;
    devel-major ) echo $DOGECHAT_DEVEL_FULL | cut -d'.' -f1 ;;
    devel-minor ) echo $DOGECHAT_DEVEL_FULL | cut -d'.' -f2 ;;
    devel-patch ) echo $DOGECHAT_DEVEL_FULL | cut -d'.' -f3- ;;
    * ) echo >&2 "ERROR: unknown version."
        exit 1 ;;
esac
