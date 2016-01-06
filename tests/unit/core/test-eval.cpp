/*
 * test-eval.cpp - test evaluation functions
 *
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

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "src/core/doge-eval.h"
#include "src/core/doge-config.h"
#include "src/core/doge-config-file.h"
#include "src/core/doge-hashtable.h"
#include "src/core/doge-string.h"
#include "src/core/doge-version.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
}

#define DOGE_CHECK_EVAL(__result, __expr)                                \
    value = eval_expression (__expr, pointers, extra_vars, options);    \
    STRCMP_EQUAL(__result, value);                                      \
    free (value);

TEST_GROUP(Eval)
{
};

/*
 * Tests functions:
 *   eval_is_true
 */

TEST(Eval, Boolean)
{
    /* false */
    LONGS_EQUAL(0, eval_is_true (NULL));
    LONGS_EQUAL(0, eval_is_true (""));
    LONGS_EQUAL(0, eval_is_true ("0"));

    /* true */
    LONGS_EQUAL(1, eval_is_true ("00"));
    LONGS_EQUAL(1, eval_is_true ("1"));
    LONGS_EQUAL(1, eval_is_true ("A"));
    LONGS_EQUAL(1, eval_is_true ("abcdef"));
}

/*
 * Tests functions:
 *   eval_expression (condition)
 */

TEST(Eval, EvalCondition)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    char *value;

    pointers = NULL;

    extra_vars = hashtable_new (32,
                                DOGECHAT_HASHTABLE_STRING,
                                DOGECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "test", "value");

    options = hashtable_new (32,
                             DOGECHAT_HASHTABLE_STRING,
                             DOGECHAT_HASHTABLE_STRING,
                             NULL, NULL);
    CHECK(options);
    hashtable_set (options, "type", "condition");

    POINTERS_EQUAL(NULL, eval_expression (NULL, NULL, NULL, options));

    /* conditions evaluated as false */
    DOGE_CHECK_EVAL("0", "");
    DOGE_CHECK_EVAL("0", "0");
    DOGE_CHECK_EVAL("0", "1 == 2");
    DOGE_CHECK_EVAL("0", "1 >= 2");
    DOGE_CHECK_EVAL("0", "2 <= 1");
    DOGE_CHECK_EVAL("0", "2 != 2");
    DOGE_CHECK_EVAL("0", "18 < 5");
    DOGE_CHECK_EVAL("0", "5 > 18");
    DOGE_CHECK_EVAL("0", "1 == 5 > 18");
    DOGE_CHECK_EVAL("0", "abc == def");
    DOGE_CHECK_EVAL("0", "()");
    DOGE_CHECK_EVAL("0", "(5 > 26)");
    DOGE_CHECK_EVAL("0", "((5 > 26))");
    DOGE_CHECK_EVAL("0", "(26 < 5)");
    DOGE_CHECK_EVAL("0", "abc > def");
    DOGE_CHECK_EVAL("0", "1 && 0");
    DOGE_CHECK_EVAL("0", "abc && 0");
    DOGE_CHECK_EVAL("0", "0 || 0");
    DOGE_CHECK_EVAL("0", "0 || 0 || 0");
    DOGE_CHECK_EVAL("0", "0 || 1 && 0");
    DOGE_CHECK_EVAL("0", "0 || (1 && 0)");
    DOGE_CHECK_EVAL("0", "0 || (0 || (1 && 0))");
    DOGE_CHECK_EVAL("0", "1 && (0 || 0)");
    DOGE_CHECK_EVAL("0", "(0 || 1) && 0");
    DOGE_CHECK_EVAL("0", "((0 || 1) && 1) && 0");
    DOGE_CHECK_EVAL("0", "abcd =~ (?-i)^ABC");
    DOGE_CHECK_EVAL("0", "abcd =~ \\(abcd\\)");
    DOGE_CHECK_EVAL("0", "(abcd) =~ \\(\\(abcd\\)\\)");
    DOGE_CHECK_EVAL("0", "${test} == test");
    DOGE_CHECK_EVAL("0", "${test2} == value2");
    DOGE_CHECK_EVAL("0", "${buffer.number} == 2");
    DOGE_CHECK_EVAL("0", "${window.buffer.number} == 2");

    /* conditions evaluated as true */
    DOGE_CHECK_EVAL("1", "1");
    DOGE_CHECK_EVAL("1", "123");
    DOGE_CHECK_EVAL("1", "abc");
    DOGE_CHECK_EVAL("1", "2 == 2");
    DOGE_CHECK_EVAL("1", "2 >= 1");
    DOGE_CHECK_EVAL("1", "1 <= 2");
    DOGE_CHECK_EVAL("1", "1 != 2");
    DOGE_CHECK_EVAL("1", "18 > 5");
    DOGE_CHECK_EVAL("1", "5 < 18");
    DOGE_CHECK_EVAL("1", "1 == 18 > 5");
    DOGE_CHECK_EVAL("1", "abc == abc");
    DOGE_CHECK_EVAL("1", "(26 > 5)");
    DOGE_CHECK_EVAL("1", "((26 > 5))");
    DOGE_CHECK_EVAL("1", "(5 < 26)");
    DOGE_CHECK_EVAL("1", "def > abc");
    DOGE_CHECK_EVAL("1", "1 && 1");
    DOGE_CHECK_EVAL("1", "abc && 1");
    DOGE_CHECK_EVAL("1", "0 || 1");
    DOGE_CHECK_EVAL("1", "0 || 0 || 1");
    DOGE_CHECK_EVAL("1", "1 || 1 && 0");
    DOGE_CHECK_EVAL("1", "0 || (1 && 1)");
    DOGE_CHECK_EVAL("1", "0 || (0 || (1 && 1))");
    DOGE_CHECK_EVAL("1", "1 && (0 || 1)");
    DOGE_CHECK_EVAL("1", "(0 || 1) && 1");
    DOGE_CHECK_EVAL("1", "((0 || 1) && 1) && 1");
    DOGE_CHECK_EVAL("1", "abcd =~ ^ABC");
    DOGE_CHECK_EVAL("1", "abcd =~ (?-i)^abc");
    DOGE_CHECK_EVAL("1", "(abcd) =~ (abcd)");
    DOGE_CHECK_EVAL("1", "(abcd) =~ \\(abcd\\)");
    DOGE_CHECK_EVAL("1", "((abcd)) =~ \\(\\(abcd\\)\\)");
    DOGE_CHECK_EVAL("1", "${test} == value");
    DOGE_CHECK_EVAL("1", "${test2} ==");
    DOGE_CHECK_EVAL("1", "${buffer.number} == 1");
    DOGE_CHECK_EVAL("1", "${window.buffer.number} == 1");

    hashtable_free (extra_vars);
    hashtable_free (options);
}

/*
 * Tests functions:
 *   eval_expression (expression)
 */

TEST(Eval, EvalExpression)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    struct t_config_option *ptr_option;
    char *value, str_value[256];

    pointers = NULL;

    extra_vars = hashtable_new (32,
                                DOGECHAT_HASHTABLE_STRING,
                                DOGECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "test", "value");

    options = NULL;

    POINTERS_EQUAL(NULL, eval_expression (NULL, NULL, NULL, NULL));

    /* test with simple strings */
    DOGE_CHECK_EVAL("", "");
    DOGE_CHECK_EVAL("a b c", "a b c");
    DOGE_CHECK_EVAL("$", "$");
    DOGE_CHECK_EVAL("", "${");
    DOGE_CHECK_EVAL("}", "}");
    DOGE_CHECK_EVAL("", "${}");
    DOGE_CHECK_EVAL("", "${xyz}");

    /* test eval of substring */
    DOGE_CHECK_EVAL("\t", "${eval:${\\t}}");

    /* test value from extra_vars */
    DOGE_CHECK_EVAL("value", "${test}");

    /* test escaped chars */
    DOGE_CHECK_EVAL("\t", "${\\t}");
    DOGE_CHECK_EVAL("\t", "${esc:\t}");

    /* test hidden chars */
    DOGE_CHECK_EVAL("********", "${hide:*,password}");
    DOGE_CHECK_EVAL("\u2603\u2603\u2603", "${hide:${esc:\u2603},abc}");

    /* test color */
    DOGE_CHECK_EVAL(gui_color_get_custom ("green"), "${color:green}");
    DOGE_CHECK_EVAL(gui_color_get_custom ("*214"), "${color:*214}");
    snprintf (str_value, sizeof (str_value),
              "%s-test-",
              gui_color_from_option (config_color_chat_delimiters));
    DOGE_CHECK_EVAL(str_value, "${color:chat_delimiters}-test-");
    config_file_search_with_string ("irc.color.message_join", NULL, NULL,
                                    &ptr_option, NULL);
    if (!ptr_option)
    {
        FAIL("ERROR: option irc.color.message_join not found.");
    }
    snprintf (str_value, sizeof (str_value),
              "%s-test-", gui_color_from_option (ptr_option));
    DOGE_CHECK_EVAL(str_value, "${color:irc.color.message_join}-test-");
    DOGE_CHECK_EVAL("test", "${option.not.found}test");

    /* test info */
    DOGE_CHECK_EVAL(version_get_version (), "${info:version}");

    /* test date */
    value = eval_expression ("${date}", pointers, extra_vars, options);
    LONGS_EQUAL(19, strlen (value));
    free (value);
    value = eval_expression ("${date:%H:%M:%S}",
                             pointers, extra_vars, options);
    LONGS_EQUAL(8, strlen (value));
    free (value);

    /* test option */
    snprintf (str_value, sizeof (str_value),
              "%d", CONFIG_INTEGER(config_look_scroll_amount));
    DOGE_CHECK_EVAL(str_value, "${dogechat.look.scroll_amount}");
    DOGE_CHECK_EVAL(str_value, "${${window.buffer.name}.look.scroll_amount}");

    /* test hdata */
    DOGE_CHECK_EVAL("x", "x${buffer.number");
    DOGE_CHECK_EVAL("x${buffer.number}1",
                   "x\\${buffer.number}${buffer.number}");
    DOGE_CHECK_EVAL("1", "${buffer.number}");
    DOGE_CHECK_EVAL("1", "${window.buffer.number}");
    DOGE_CHECK_EVAL("core.dogechat", "${buffer.full_name}");
    DOGE_CHECK_EVAL("core.dogechat", "${window.buffer.full_name}");

    hashtable_free (extra_vars);
}

/*
 * Tests functions:
 *   eval_expression (replace with regex)
 */

TEST(Eval, EvalReplaceRegex)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    char *value;
    regex_t regex;

    pointers = hashtable_new (32,
                              DOGECHAT_HASHTABLE_STRING,
                              DOGECHAT_HASHTABLE_POINTER,
                              NULL, NULL);
    CHECK(pointers);

    extra_vars = hashtable_new (32,
                                DOGECHAT_HASHTABLE_STRING,
                                DOGECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "test", "value");

    options = hashtable_new (32,
                             DOGECHAT_HASHTABLE_STRING,
                             DOGECHAT_HASHTABLE_STRING,
                             NULL, NULL);
    CHECK(options);

    /* replace regex by empty string */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", ".*");
    hashtable_set (options, "regex_replace", "");
    DOGE_CHECK_EVAL("", "test");

    /* replace empty regex */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "");
    hashtable_set (options, "regex_replace", "abc");
    DOGE_CHECK_EVAL("test", "test");

    /* replace empty regex by empty string */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "");
    hashtable_set (options, "regex_replace", "");
    DOGE_CHECK_EVAL("test", "test");

    /* add brackets around URLs (regex as string) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "\\w+://\\S+");
    hashtable_set (options, "regex_replace", "[ ${re:0} ]");
    DOGE_CHECK_EVAL("test: [ https://dogechat.org ]",
                   "test: https://dogechat.org");

    /* add brackets around URLs (compiled regex) */
    LONGS_EQUAL(0, string_regcomp (&regex, "\\w+://\\S+",
                                   REG_EXTENDED | REG_ICASE));
    hashtable_set (pointers, "regex", &regex);
    hashtable_remove (options, "regex");
    hashtable_set (options, "regex_replace", "[ ${re:0} ]");
    DOGE_CHECK_EVAL("test: [ https://dogechat.org ]",
                   "test: https://dogechat.org");
    regfree (&regex);

    /* hide passwords (regex as string) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "(password=)(\\S+)");
    hashtable_set (options, "regex_replace", "${re:1}${hide:*,${re:2}}");
    DOGE_CHECK_EVAL("password=*** password=***",
                   "password=abc password=def");

    /* hide passwords (compiled regex) */
    LONGS_EQUAL(0, string_regcomp (&regex, "(password=)(\\S+)",
                                   REG_EXTENDED | REG_ICASE));
    hashtable_set (pointers, "regex", &regex);
    hashtable_remove (options, "regex");
    hashtable_set (options, "regex_replace", "${re:1}${hide:*,${re:2}}");
    DOGE_CHECK_EVAL("password=*** password=***",
                   "password=abc password=def");
    regfree (&regex);

    hashtable_free (pointers);
    hashtable_free (extra_vars);
    hashtable_free (options);
}
