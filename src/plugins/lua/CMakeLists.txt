#
# Copyright (C) 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
# Copyright (C) 2006-2016 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(lua MODULE dogechat-lua.c dogechat-lua.h dogechat-lua-api.c
dogechat-lua-api.h)

set_target_properties(lua PROPERTIES PREFIX "")

if(LUA_FOUND)
  include_directories(${LUA_INCLUDE_DIRS})
  target_link_libraries(lua ${LUA_LDFLAGS} dogechat_plugins_scripts)
endif()

install(TARGETS lua LIBRARY DESTINATION ${LIBDIR}/plugins)
