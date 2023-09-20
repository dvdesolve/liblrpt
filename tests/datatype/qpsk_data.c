/*
 * This file is part of liblrpt.
 *
 * liblrpt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liblrpt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with liblrpt. If not, see https://www.gnu.org/licenses/
 *
 * Author: Viktor Drobot
 */

/*************************************************************************************************/

#include <stdlib.h>

#include <check.h>

#include "lrpt.h"

/*************************************************************************************************/

#define NEL(x) (sizeof(x) / sizeof((x)[0]))

/*************************************************************************************************/

static int TEST_len = 50;

/*************************************************************************************************/

START_TEST(test_alloc) {
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(0, NULL);

    ck_assert_ptr_nonnull(data);

    lrpt_qpsk_data_free(data);
}

START_TEST(test_length) {
    lrpt_qpsk_data_t *data1 = lrpt_qpsk_data_alloc(0, NULL);
    lrpt_qpsk_data_t *data2 = lrpt_qpsk_data_alloc(TEST_len, NULL);

    ck_assert_int_eq(lrpt_qpsk_data_length(data1), 0); /* empty object */
    ck_assert_int_eq(lrpt_qpsk_data_length(data2), TEST_len); /* object with length */
    ck_assert_int_eq(lrpt_qpsk_data_length(NULL), 0); /* NULL pointer */

    lrpt_qpsk_data_free(data1);
    lrpt_qpsk_data_free(data2);
}

START_TEST(test_resize) {
    lrpt_qpsk_data_t *data = lrpt_qpsk_data_alloc(0, NULL);

    /* resize to length */
    ck_assert(lrpt_qpsk_data_resize(data, TEST_len, NULL));
    ck_assert_int_eq(lrpt_qpsk_data_length(data), TEST_len);

    /* resize to zero length */
    ck_assert(lrpt_qpsk_data_resize(data, 0, NULL));
    ck_assert_int_eq(lrpt_qpsk_data_length(data), 0);

    lrpt_qpsk_data_free(data);
}

Suite *qpsk_data_suite(void) {
    Suite *s;
    TCase *tc_alloc, *tc_length, *tc_construct, *tc_convert;

    s = suite_create("QPSK data");
    tc_alloc = tcase_create("allocation/freeing");
    tc_length = tcase_create("length manipulation");
    tc_construct = tcase_create("constructors");
    tc_convert = tcase_create("conversions");

    tcase_add_test(tc_alloc, test_alloc);
    tcase_add_test(tc_length, test_length);
    tcase_add_test(tc_length, test_resize);

    suite_add_tcase(s, tc_alloc);
    suite_add_tcase(s, tc_length);
    suite_add_tcase(s, tc_construct);
    suite_add_tcase(s, tc_convert);

    return s;
}

int main(void) {
    int num_failed;
    Suite *s;
    SRunner *sr;

    s = qpsk_data_suite();
    sr = srunner_create(s);

    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    num_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
