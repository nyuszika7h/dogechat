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
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dogechat.1.de.asciidoc .
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.de.asciidoc .
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='DogeChat ${VERSION}' -d manpage -f manpage -L dogechat.1.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat.1.de.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.de.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat.1 (de)"
  )
  add_custom_target(doc-man-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat.1 DESTINATION ${MANDIR}/de/man1)
endif()

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_user.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_USER} -o dogechat_user.de.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_user.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_user.de.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.de.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_user.de.html"
  )
  add_custom_target(doc-user-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_user.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_user.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_scripting.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_SCRIPTING} -o dogechat_scripting.de.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_scripting.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_scripting.de.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_scripting.de.html"
  )
  add_custom_target(doc-scripting-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_scripting.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_scripting.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_faq.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_FAQ} -o dogechat_faq.de.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_faq.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_faq.de.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_faq.de.html"
  )
  add_custom_target(doc-faq-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_faq.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_faq.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_QUICKSTART} -o dogechat_quickstart.de.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_quickstart.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_quickstart.de.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_quickstart.de.html"
  )
  add_custom_target(doc-quickstart-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_tester.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_TESTER} -o dogechat_tester.de.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_tester.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_tester.de.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_tester.de.html"
  )
  add_custom_target(doc-tester-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_tester.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_tester.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
