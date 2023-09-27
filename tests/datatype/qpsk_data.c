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

#include <stdint.h>
#include <stdlib.h>

#include <check.h>

#include "lrpt.h"

/*************************************************************************************************/

#define NEL(x) (sizeof(x) / sizeof((x)[0]))

/*************************************************************************************************/

static int TEST_len = 50;
static int8_t TEST_sdata[] = {
    57,     56,
    -90,    -35,
    68,     78,
    -22,    -94,
    79,     -14,
    80,     60,
    114,    -93,
    36,     -83,
    -1,     104,
    -109,   20,
    108,    -98,
    -16,    85,
    -127,   -34,
    -79,    85,
    117,    -56,
    125,    100
};
static unsigned char TEST_hdata[] = {
    0xCC,
    0xBA,
    0x59,
    0x1B
};
static size_t TEST_hpart_offset = 2;
static size_t TEST_hpart_n = 7;
static int8_t TEST_hpart[] = {
    127,    127,
    -127,   -127,
    127,    -127,
    127,    127,
    127,    -127,
    127,    -127,
    -127,   127
};

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

START_TEST(test_from_soft) {
    int len = NEL(TEST_sdata) / 2;

    lrpt_qpsk_data_t *data1 = lrpt_qpsk_data_alloc(0, NULL);
    lrpt_qpsk_data_t *data2 = NULL;

    /* assign to existing data object: full length */
    ck_assert(lrpt_qpsk_data_from_soft(data1, TEST_sdata, 0, len, NULL));
    ck_assert_int_eq(lrpt_qpsk_data_length(data1), len);

    /* assign to existing data object: length - 1, offset 1 */
    ck_assert(lrpt_qpsk_data_from_soft(data1, TEST_sdata, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_qpsk_data_length(data1), len - 1);

    lrpt_qpsk_data_free(data1);


    /* create new data object: full length */
    data2 = lrpt_qpsk_data_create_from_soft(TEST_sdata, 0, len, NULL);

    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_qpsk_data_length(data2), len);

    lrpt_qpsk_data_free(data2);

    /* create new data object: length - 1, offset 1 */
    data2 = lrpt_qpsk_data_create_from_soft(TEST_sdata, 1, len - 1, NULL);

    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_qpsk_data_length(data2), len - 1);

    lrpt_qpsk_data_free(data2);
}

START_TEST(test_from_hard) {
    int len = NEL(TEST_hdata) * 4;

    lrpt_qpsk_data_t *data1 = lrpt_qpsk_data_alloc(0, NULL);
    lrpt_qpsk_data_t *data2 = NULL;

    /* assign to existing data object: full length */
    ck_assert(lrpt_qpsk_data_from_hard(data1, TEST_hdata, 0, len, NULL));
    ck_assert_int_eq(lrpt_qpsk_data_length(data1), len);

    /* assign to existing data object: length - 1, offset 1 */
    ck_assert(lrpt_qpsk_data_from_hard(data1, TEST_hdata, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_qpsk_data_length(data1), len - 1);

    lrpt_qpsk_data_free(data1);


    /* create new data object: full length */
    data2 = lrpt_qpsk_data_create_from_hard(TEST_hdata, 0, len, NULL);

    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_qpsk_data_length(data2), len);

    lrpt_qpsk_data_free(data2);

    /* create new data object: length - 1, offset 1 */
    data2 = lrpt_qpsk_data_create_from_hard(TEST_hdata, 1, len - 1, NULL);

    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_qpsk_data_length(data2), len - 1);

    lrpt_qpsk_data_free(data2);
}

START_TEST(test_to_soft) {
    int len = NEL(TEST_sdata) / 2;

    lrpt_qpsk_data_t *data = lrpt_qpsk_data_create_from_soft(TEST_sdata, 0, len, NULL);

    int8_t rsdata1[2 * len], rsdata2[2 * (len - 1)];

    /* full length */
    ck_assert(lrpt_qpsk_data_to_soft(rsdata1, data, 0, len, NULL));

    for (int i = 0; i < len; i++) {
        ck_assert_int_eq(TEST_sdata[2 * i], rsdata1[2 * i]);
        ck_assert_int_eq(TEST_sdata[2 * i + 1], rsdata1[2 * i + 1]);
    }

    /* length - 1, offset 1 */
    ck_assert(lrpt_qpsk_data_to_soft(rsdata2, data, 1, len - 1, NULL));

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_int_eq(TEST_sdata[2 + 2 * i], rsdata1[2 + 2 * i]);
        ck_assert_int_eq(TEST_sdata[2 + 2 * i + 1], rsdata1[2 + 2 * i + 1]);
    }

    lrpt_qpsk_data_free(data);
}

START_TEST(test_to_hard) {
    int len = NEL(TEST_hdata) * 4;

    lrpt_qpsk_data_t *data = lrpt_qpsk_data_create_from_hard(TEST_hdata, 0, len, NULL);

    int8_t rsdata[2 * TEST_hpart_n];

    /* partial length - this is much more difficult than *_to_soft so that only test is enough */
    ck_assert(lrpt_qpsk_data_to_soft(rsdata, data, TEST_hpart_offset, TEST_hpart_n, NULL));

    for (int i = 0; i < TEST_hpart_n; i++) {
        ck_assert_int_eq(TEST_hpart[2 * i], rsdata[2 * i]);
        ck_assert_int_eq(TEST_hpart[2 * i + 1], rsdata[2 * i + 1]);
    }

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
    tcase_add_test(tc_construct, test_from_soft);
    tcase_add_test(tc_construct, test_from_hard);
    tcase_add_test(tc_convert, test_to_soft);
    tcase_add_test(tc_convert, test_to_hard);

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
