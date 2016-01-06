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
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dogechat.1.pl.asciidoc ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.pl.asciidoc ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='DogeChat ${VERSION}' -d manpage -f manpage -L ${CMAKE_CURRENT_BINARY_DIR}/dogechat.1.pl.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat.1.pl.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.pl.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat.1 (pl)"
  )
  add_custom_target(doc-man-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat.1 DESTINATION ${MANDIR}/pl/man1)
endif()

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_user.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_USER} -o dogechat_user.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_user.pl.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_user.pl.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.pl.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_user.pl.html"
  )
  add_custom_target(doc-user-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_user.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_user.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_scripting.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_SCRIPTING} -o dogechat_scripting.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_scripting.pl.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_scripting.pl.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_scripting.pl.html"
  )
  add_custom_target(doc-scripting-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_scripting.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_scripting.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_faq.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_FAQ} -o dogechat_faq.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_faq.pl.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_faq.pl.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_faq.pl.html"
  )
  add_custom_target(doc-faq-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_faq.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_faq.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_QUICKSTART} -o dogechat_quickstart.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_quickstart.pl.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_quickstart.pl.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_quickstart.pl.html"
  )
  add_custom_target(doc-quickstart-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_quickstart.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dogechat_tester.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_TESTER} -o dogechat_tester.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_tester.pl.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dogechat_tester.pl.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building dogechat_tester.pl.html"
  )
  add_custom_target(doc-tester-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dogechat_tester.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dogechat_tester.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
