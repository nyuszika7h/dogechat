/*
 * test-list.cpp - test list functions
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
#include "src/core/doge-list.h"
#include "src/plugins/plugin.h"
}

#define LIST_VALUE_TEST "test"
#define LIST_VALUE_XYZ  "xyz"
#define LIST_VALUE_ZZZ  "zzz"

TEST_GROUP(List)
{
};

/*
 * Creates a list for tests.
 */

struct t_dogelist *
test_list_new ()
{
    struct t_dogelist *list;

    list = dogelist_new ();

    dogelist_add (list, LIST_VALUE_ZZZ, DOGECHAT_LIST_POS_END, NULL);
    dogelist_add (list, LIST_VALUE_TEST, DOGECHAT_LIST_POS_BEGINNING, NULL);
    dogelist_add (list, LIST_VALUE_XYZ, DOGECHAT_LIST_POS_SORT, NULL);

    return list;
}

/*
 * Tests functions:
 *   dogelist_new
 */

TEST(List, New)
{
    struct t_dogelist *list;

    list = dogelist_new();
    CHECK(list);

    /* check initial values */
    POINTERS_EQUAL(NULL, list->items);
    POINTERS_EQUAL(NULL, list->last_item);
    LONGS_EQUAL(0, list->size);

    /* free list */
    dogelist_free (list);
}

/*
 * Tests functions:
 *   dogelist_add
 *   dogelist_free
 */

TEST(List, Add)
{
    struct t_dogelist *list;
    struct t_dogelist_item *item1, *item2, *item3;
    const char *str_user_data = "some user data";

    list = dogelist_new();

    POINTERS_EQUAL(NULL, dogelist_add (NULL, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, dogelist_add (list, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, dogelist_add (NULL, LIST_VALUE_TEST, NULL, NULL));
    POINTERS_EQUAL(NULL, dogelist_add (NULL, NULL, DOGECHAT_LIST_POS_END, NULL));
    POINTERS_EQUAL(NULL, dogelist_add (list, LIST_VALUE_TEST, NULL, NULL));
    POINTERS_EQUAL(NULL, dogelist_add (list, NULL, DOGECHAT_LIST_POS_END, NULL));
    POINTERS_EQUAL(NULL, dogelist_add (NULL, LIST_VALUE_TEST,
                                      DOGECHAT_LIST_POS_END, NULL));

    /* add an element at the end */
    item1 = dogelist_add (list, LIST_VALUE_ZZZ, DOGECHAT_LIST_POS_END,
                         (void *)str_user_data);
    CHECK(item1);
    CHECK(item1->data);
    CHECK(item1->user_data);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, item1->data);
    POINTERS_EQUAL(str_user_data, item1->user_data);
    LONGS_EQUAL(1, list->size);  /* list is now: ["zzz"] */
    POINTERS_EQUAL(item1, list->items);

    /* add an element at the beginning */
    item2 = dogelist_add (list, LIST_VALUE_TEST, DOGECHAT_LIST_POS_BEGINNING,
                         (void *)str_user_data);
    CHECK(item2);
    CHECK(item2->data);
    CHECK(item2->user_data);
    STRCMP_EQUAL(LIST_VALUE_TEST, item2->data);
    POINTERS_EQUAL(str_user_data, item2->user_data);
    LONGS_EQUAL(2, list->size);  /* list is now: ["test", "zzz"] */
    POINTERS_EQUAL(item2, list->items);
    POINTERS_EQUAL(item1, list->items->next_item);

    /* add an element, using sort */
    item3 = dogelist_add (list, LIST_VALUE_XYZ, DOGECHAT_LIST_POS_SORT,
                         (void *)str_user_data);
    CHECK(item3);
    CHECK(item3->data);
    CHECK(item3->user_data);
    STRCMP_EQUAL(LIST_VALUE_XYZ, item3->data);
    POINTERS_EQUAL(str_user_data, item3->user_data);
    LONGS_EQUAL(3, list->size);  /* list is now: ["test", "xyz", "zzz"] */
    POINTERS_EQUAL(item2, list->items);
    POINTERS_EQUAL(item3, list->items->next_item);
    POINTERS_EQUAL(item1, list->items->next_item->next_item);

    /* free list */
    dogelist_free (list);
}

/*
 * Tests functions:
 *   dogelist_search
 *   dogelist_search_pos
 *   dogelist_casesearch
 *   dogelist_casesearch_pos
 */

TEST(List, Search)
{
    struct t_dogelist *list;
    struct t_dogelist_item *ptr_item;

    list = test_list_new ();

    /* search an element */

    POINTERS_EQUAL(NULL, dogelist_search (NULL, NULL));
    POINTERS_EQUAL(NULL, dogelist_search (list, NULL));
    POINTERS_EQUAL(NULL, dogelist_search (NULL, LIST_VALUE_TEST));

    POINTERS_EQUAL(NULL, dogelist_search (list, "not found"));
    POINTERS_EQUAL(NULL, dogelist_search (list, "TEST"));

    ptr_item = dogelist_search (list, LIST_VALUE_TEST);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    ptr_item = dogelist_search (list, LIST_VALUE_XYZ);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);

    ptr_item = dogelist_search (list, LIST_VALUE_ZZZ);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);

    /* search the position of an element */

    LONGS_EQUAL(-1, dogelist_search_pos (NULL, NULL));
    LONGS_EQUAL(-1, dogelist_search_pos (list, NULL));
    LONGS_EQUAL(-1, dogelist_search_pos (NULL, LIST_VALUE_TEST));

    LONGS_EQUAL(-1, dogelist_search_pos (list, "not found"));
    LONGS_EQUAL(-1, dogelist_search_pos (list, "TEST"));

    LONGS_EQUAL(0, dogelist_search_pos (list, LIST_VALUE_TEST));
    LONGS_EQUAL(1, dogelist_search_pos (list, LIST_VALUE_XYZ));
    LONGS_EQUAL(2, dogelist_search_pos (list, LIST_VALUE_ZZZ));

    /* case-insensitive search of an element */

    POINTERS_EQUAL(NULL, dogelist_casesearch (NULL, NULL));
    POINTERS_EQUAL(NULL, dogelist_casesearch (list, NULL));
    POINTERS_EQUAL(NULL, dogelist_casesearch (NULL, LIST_VALUE_TEST));

    POINTERS_EQUAL(NULL, dogelist_casesearch (list, "not found"));

    ptr_item = dogelist_casesearch (list, "TEST");
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    ptr_item = dogelist_casesearch (list, LIST_VALUE_TEST);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    ptr_item = dogelist_casesearch (list, LIST_VALUE_XYZ);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);

    ptr_item = dogelist_casesearch (list, LIST_VALUE_ZZZ);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);

    /* case-insensitive search of an element position */

    LONGS_EQUAL(-1, dogelist_casesearch_pos (NULL, NULL));
    LONGS_EQUAL(-1, dogelist_casesearch_pos (list, NULL));
    LONGS_EQUAL(-1, dogelist_casesearch_pos (NULL, LIST_VALUE_TEST));

    LONGS_EQUAL(-1, dogelist_casesearch_pos (list, "not found"));

    LONGS_EQUAL(0, dogelist_casesearch_pos (list, "TEST"));
    LONGS_EQUAL(0, dogelist_casesearch_pos (list, LIST_VALUE_TEST));
    LONGS_EQUAL(1, dogelist_casesearch_pos (list, LIST_VALUE_XYZ));
    LONGS_EQUAL(2, dogelist_casesearch_pos (list, LIST_VALUE_ZZZ));

    /* free list */
    dogelist_free (list);
}

/*
 * Tests functions:
 *   dogelist_get
 *   dogelist_string
 */

TEST(List, Get)
{
    struct t_dogelist *list;
    struct t_dogelist_item *ptr_item;

    list = test_list_new ();

    /* get an element by position */

    POINTERS_EQUAL(NULL, dogelist_get (NULL, -1));
    POINTERS_EQUAL(NULL, dogelist_get (list, -1));
    POINTERS_EQUAL(NULL, dogelist_get (NULL, 0));

    POINTERS_EQUAL(NULL, dogelist_get (list, 50));

    ptr_item = dogelist_get (list, 0);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    ptr_item = dogelist_get (list, 1);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);

    ptr_item = dogelist_get (list, 2);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);

    /* get string value of an element */

    POINTERS_EQUAL(NULL, dogelist_string (NULL));

    ptr_item = dogelist_get(list, 0);
    STRCMP_EQUAL(LIST_VALUE_TEST, dogelist_string (ptr_item));

    ptr_item = dogelist_get(list, 1);
    STRCMP_EQUAL(LIST_VALUE_XYZ, dogelist_string (ptr_item));

    ptr_item = dogelist_get(list, 2);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, dogelist_string (ptr_item));

    /* free list */
    dogelist_free (list);
}

/*
 * Tests functions:
 *   dogelist_set
 */

TEST(List, Set)
{
    struct t_dogelist *list;
    struct t_dogelist_item *ptr_item;
    const char *another_test = "another test";

    list = test_list_new ();

    ptr_item = dogelist_get (list, 0);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    dogelist_set (NULL, NULL);
    dogelist_set (ptr_item, NULL);
    dogelist_set (NULL, another_test);

    dogelist_set (ptr_item, another_test);
    STRCMP_EQUAL(another_test, ptr_item->data);

    /* free list */
    dogelist_free (list);
}

/*
 * Tests functions:
 *   dogelist_next
 *   dogelist_prev
 */

TEST(List, Move)
{
    struct t_dogelist *list;
    struct t_dogelist_item *ptr_item;

    list = test_list_new ();

    /* get next item */

    ptr_item = dogelist_get (list, 0);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);
    ptr_item = dogelist_next (ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);
    ptr_item = dogelist_next (ptr_item);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);
    ptr_item = dogelist_next (ptr_item);
    POINTERS_EQUAL(NULL, ptr_item);

    /* get previous item */

    ptr_item = dogelist_get(list, 2);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);
    ptr_item = dogelist_prev (ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);
    ptr_item = dogelist_prev (ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    /* free list */
    dogelist_free (list);
}

/*
 * Tests functions:
 *   dogelist_remove
 *   dogelist_remove_all
 */

TEST(List, Free)
{
    struct t_dogelist *list;
    struct t_dogelist_item *ptr_item;

    list = test_list_new ();

    /* remove one element */

    ptr_item = dogelist_get(list, 1);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);

    dogelist_remove (NULL, NULL);
    dogelist_remove (list, NULL);
    dogelist_remove (NULL, ptr_item);

    dogelist_remove (list, ptr_item);

    ptr_item = dogelist_get(list, 1);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);
    ptr_item = dogelist_get (list, 2);
    POINTERS_EQUAL(NULL, ptr_item);

    /* remove all elements */

    dogelist_remove_all (list);
    LONGS_EQUAL(0, list->size);
    POINTERS_EQUAL(NULL, list->items);
    POINTERS_EQUAL(NULL, list->last_item);

    /* free list */
    dogelist_free (list);
}

/*
 * Tests functions:
 *   dogelist_print_log
 */

TEST(List, PrintLog)
{
    /* TODO: write tests */
}
