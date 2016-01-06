#!/bin/sh
#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
# Build tarballs (.tar.gz and .tar.bz2) for DogeChat using git-archive.
#
# Syntax:
#    makedist.sh <version> <tree-ish> [<path>]
#
#      version : DogeChat version, for example 0.3.9 or 0.4.0-dev
#      tree-ish: git tree-ish (optional, defaults to HEAD), example: v0.3.9
#      path    : where to put packages (optional, default is current directory)
#

if [ $# -lt 2 ]; then
    echo "Syntax: $0 <version> <tree-ish> [<path>]"
    exit 1
fi

VERSION=$1
TREEISH=${2:-HEAD}
OUTPATH="."

if [ $# -ge 3 ]; then
    OUTPATH=$3
fi

PREFIX="dogechat-${VERSION}/"
FILE="dogechat-${VERSION}.tar"

echo "Building file ${FILE}.bz2"
git archive --prefix=${PREFIX} ${TREEISH} | bzip2 -c >${OUTPATH}/${FILE}.bz2

echo "Building file ${FILE}.gz"
git archive --prefix=${PREFIX} ${TREEISH} | gzip -c >${OUTPATH}/${FILE}.gz
