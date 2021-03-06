#
# Copyright (C) 2012-2016 Sébastien Helleu <flashcode@flashtux.org>
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

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.es.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_QUICKSTART} -o dogechat_quickstart.es.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_quickstart.es.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_quickstart.es.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_quickstart.es.html"
  )
  add_custom_target(doc-quickstart-es ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.es.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.es.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
