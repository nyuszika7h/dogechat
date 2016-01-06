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

if(ENABLE_MAN)
  # man page
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat.1
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dogechat.1.ru.asciidoc .
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.ru.asciidoc .
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='DogeChat ${VERSION}' -d manpage -f manpage -L dogechat.1.ru.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat.1.ru.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.ru.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat.1 (ru)"
  )
  add_custom_target(doc-man-ru ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat.1 DESTINATION ${MANDIR}/ru/man1)
endif()

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.ru.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_QUICKSTART} -o dogechat_quickstart.ru.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_quickstart.ru.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_quickstart.ru.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_quickstart.ru.html"
  )
  add_custom_target(doc-quickstart-ru ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.ru.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.ru.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
