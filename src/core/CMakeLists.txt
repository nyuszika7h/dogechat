#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2008 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2009 Emmanuel Bouthenot <kolter@openics.org>
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

set(LIB_CORE_SRC
dogechat.c dogechat.h
doge-arraylist.c doge-arraylist.h
doge-backtrace.c doge-backtrace.h
doge-command.c doge-command.h
doge-completion.c doge-completion.h
doge-config.c doge-config.h
doge-config-file.c doge-config-file.h
doge-debug.c doge-debug.h
doge-eval.c doge-eval.h
doge-hashtable.c doge-hashtable.h
doge-hdata.c doge-hdata.h
doge-hook.c doge-hook.h
doge-infolist.c doge-infolist.h
doge-input.c doge-input.h
doge-list.c doge-list.h
doge-log.c doge-log.h
doge-network.c doge-network.h
doge-proxy.c doge-proxy.h
doge-secure.c doge-secure.h
doge-string.c doge-string.h
doge-upgrade.c doge-upgrade.h
doge-upgrade-file.c doge-upgrade-file.h
doge-url.c doge-url.h
doge-utf8.c doge-utf8.h
doge-util.c doge-util.h
doge-version.c doge-version.h)

# Check for flock support
include(CheckSymbolExists)
check_symbol_exists(flock "sys/file.h" HAVE_FLOCK)

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  find_library(EXECINFO_LIB_PATH execinfo /usr/local/lib)
  set(CMAKE_REQUIRED_LIBRARIES "${EXECINFO_LIB_PATH}")
  check_function_exists(backtrace HAVE_BACKTRACE)
else()
  check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)
endif()

if(GNUTLS_FOUND)
  include_directories(${GNUTLS_INCLUDE_PATH})
endif()

include_directories(${CURL_INCLUDE_DIRS})

include_directories(${CMAKE_BINARY_DIR})
add_library(dogechat_core STATIC ${LIB_CORE_SRC})
add_dependencies(dogechat_core version_git)
