#
# Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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

add_library(aspell MODULE
dogechat-aspell.c dogechat-aspell.h
dogechat-aspell-bar-item.c dogechat-aspell-bar-item.h
dogechat-aspell-command.c dogechat-aspell-command.h
dogechat-aspell-completion.c dogechat-aspell-completion.h
dogechat-aspell-config.c dogechat-aspell-config.h
dogechat-aspell-info.c dogechat-aspell-info.h
dogechat-aspell-speller.c dogechat-aspell-speller.h)
set_target_properties(aspell PROPERTIES PREFIX "")

if(ENCHANT_FOUND)
  include_directories(${ENCHANT_INCLUDE_DIR})
  target_link_libraries(aspell ${ENCHANT_LIBRARIES})
  add_definitions(-DUSE_ENCHANT)
else()
  if(ASPELL_FOUND)
    include_directories(${ASPELL_INCLUDE_PATH})
    target_link_libraries(aspell ${ASPELL_LIBRARY})
  endif()
endif()

install(TARGETS aspell LIBRARY DESTINATION ${LIBDIR}/plugins)
