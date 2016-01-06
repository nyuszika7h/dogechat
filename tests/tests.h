/*
 * Copyright (C) 2014-2016 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of DogeChat, the extensible chat client.
 *
 * DogeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * DogeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DogeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOGECHAT_TESTS_H
#define DOGECHAT_TESTS_H 1

#define DOGE_TEST_STR(__result, __test)                                  \
    str = __test;                                                       \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, str);                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, str);                                    \
    }                                                                   \
    free (str);

#endif /* DOGECHAT_TESTS_H */
