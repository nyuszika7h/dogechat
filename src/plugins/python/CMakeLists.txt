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

add_library(python MODULE dogechat-python.c dogechat-python.h
dogechat-python-api.c dogechat-python-api.h)

set_target_properties(python PROPERTIES PREFIX "")

if(PYTHON_FOUND)
  include_directories(${PYTHON_INCLUDE_PATH})
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PYTHON_LFLAGS}")
  target_link_libraries(python ${PYTHON_LIBRARY} dogechat_plugins_scripts)
endif()

install(TARGETS python LIBRARY DESTINATION ${LIBDIR}/plugins)
