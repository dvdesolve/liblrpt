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

#include <complex.h>
#include <stdlib.h>

#include <check.h>

#include "lrpt.h"

/*************************************************************************************************/

#define NEL(x) (sizeof(x) / sizeof((x)[0]))

/*************************************************************************************************/

static int TEST_len = 50;
static complex double TEST_cdata[] = {
    1.0     - 2.0 * I,
    4.5     + 2.9 * I,
    -3.1    + 9.5 * I,
    102.4   - 0.04 * I,
    2.4     - 7.5 * I,
    0.75    + 1.25 * I
};
static double TEST_ddata[] = {
    1.0,    -2.0,
    4.5,    2.9,
    -3.1,   9.5,
    102.4,  -0.04,
    2.4,    -7.5,
    0.75,   1.25
};

/*************************************************************************************************/

START_TEST(test_alloc) {
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(0, NULL);

    ck_assert_ptr_nonnull(data);

    lrpt_iq_data_free(data);
}

START_TEST(test_length) {
    lrpt_iq_data_t *data1 = lrpt_iq_data_alloc(0, NULL);
    lrpt_iq_data_t *data2 = lrpt_iq_data_alloc(TEST_len, NULL);

    ck_assert_int_eq(lrpt_iq_data_length(data1), 0); /* empty object */
    ck_assert_int_eq(lrpt_iq_data_length(data2), TEST_len); /* object with length */
    ck_assert_int_eq(lrpt_iq_data_length(NULL), 0); /* NULL pointer */

    lrpt_iq_data_free(data1);
    lrpt_iq_data_free(data2);
}

START_TEST(test_resize) {
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(0, NULL);

    /* resize to length */
    ck_assert(lrpt_iq_data_resize(data, TEST_len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data), TEST_len);

    /* resize to zero length */
    ck_assert(lrpt_iq_data_resize(data, 0, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data), 0);

    lrpt_iq_data_free(data);
}

START_TEST(test_from_complex) {
    int len = NEL(TEST_cdata);

    lrpt_iq_data_t *data1 = lrpt_iq_data_alloc(0, NULL);
    lrpt_iq_data_t *data2 = NULL;

    /* assign to existing data object: full length */
    ck_assert(lrpt_iq_data_from_complex(data1, TEST_cdata, 0, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data1), len);

    /* assign to existing data object: length - 1, offset 1 */
    ck_assert(lrpt_iq_data_from_complex(data1, TEST_cdata, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data1), len - 1);

    lrpt_iq_data_free(data1);


    /* create new data object: full length */
    data2 = lrpt_iq_data_create_from_complex(TEST_cdata, 0, len, NULL);

    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len);

    lrpt_iq_data_free(data2);

    /* create new data object: length - 1, offset 1 */
    data2 = lrpt_iq_data_create_from_complex(TEST_cdata, 1, len - 1, NULL);

    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len - 1);

    lrpt_iq_data_free(data2);
}

START_TEST(test_from_doubles) {
    int len = NEL(TEST_ddata) / 2;

    lrpt_iq_data_t *data1 = lrpt_iq_data_alloc(0, NULL);
    lrpt_iq_data_t *data2 = NULL;

    /* assign to existing data object: full length */
    ck_assert(lrpt_iq_data_from_doubles(data1, TEST_ddata, 0, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data1), len);

    /* assign to existing data object: length - 1, offset 1 */
    ck_assert(lrpt_iq_data_from_doubles(data1, TEST_ddata, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data1), len - 1);

    lrpt_iq_data_free(data1);


    /* create new data object: full length */
    data2 = lrpt_iq_data_create_from_doubles(TEST_ddata, 0, len, NULL);

    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len);

    lrpt_iq_data_free(data2);

    /* create new data object: length - 1, offset 1 */
    data2 = lrpt_iq_data_create_from_doubles(TEST_ddata, 1, len - 1, NULL);

    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len - 1);

    lrpt_iq_data_free(data2);
}

START_TEST(test_to_complex) {
    int len = NEL(TEST_cdata);

    lrpt_iq_data_t *data = lrpt_iq_data_create_from_complex(TEST_cdata, 0, len, NULL);

    complex double rcdata1[len], rcdata2[len - 1];

    /* full length */
    ck_assert(lrpt_iq_data_to_complex(rcdata1, data, 0, len, NULL));

    for (int i = 0; i < len; i++) {
        ck_assert_double_eq(creal(TEST_cdata[i]), creal(rcdata1[i]));
        ck_assert_double_eq(cimag(TEST_cdata[i]), cimag(rcdata1[i]));
    }

    /* length - 1, offset 1 */
    ck_assert(lrpt_iq_data_to_complex(rcdata2, data, 1, len - 1, NULL));

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_double_eq(creal(TEST_cdata[1 + i]), creal(rcdata1[1 + i]));
        ck_assert_double_eq(cimag(TEST_cdata[1 + i]), cimag(rcdata1[1 + i]));
    }

    lrpt_iq_data_free(data);
}

START_TEST(test_to_doubles) {
    int len = NEL(TEST_ddata) / 2;

    lrpt_iq_data_t *data = lrpt_iq_data_create_from_doubles(TEST_ddata, 0, len, NULL);

    double rddata1[2 * len], rddata2[2 * (len - 1)];

    /* full length */
    ck_assert(lrpt_iq_data_to_doubles(rddata1, data, 0, len, NULL));

    for (int i = 0; i < len; i++) {
        ck_assert_double_eq(TEST_ddata[2 * i], rddata1[2 * i]);
        ck_assert_double_eq(TEST_ddata[2 * i + 1], rddata1[2 * i + 1]);
    }

    /* length - 1, offset 1 */
    ck_assert(lrpt_iq_data_to_doubles(rddata2, data, 1, len - 1, NULL));

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_double_eq(TEST_ddata[2 + 2 * i], rddata1[2 + 2 * i]);
        ck_assert_double_eq(TEST_ddata[2 + 2 * i + 1], rddata1[2 + 2 * i + 1]);
    }

    lrpt_iq_data_free(data);
}

START_TEST(test_from_iq) {
    int len = NEL(TEST_cdata);

    lrpt_iq_data_t *data = lrpt_iq_data_create_from_complex(TEST_cdata, 0, len, NULL);
    lrpt_iq_data_t *rdata1 = lrpt_iq_data_alloc(0, NULL);
    lrpt_iq_data_t *rdata2 = NULL;
    complex double rcdata1[len], rcdata2[len - 1];

    /* assign to existing data object: full length */
    ck_assert(lrpt_iq_data_from_iq(rdata1, data, 0, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(rdata1), len);

    lrpt_iq_data_to_complex(rcdata1, rdata1, 0, len, NULL);

    for (int i = 0; i < len; i++) {
        ck_assert_double_eq(creal(TEST_cdata[i]), creal(rcdata1[i]));
        ck_assert_double_eq(cimag(TEST_cdata[i]), cimag(rcdata1[i]));
    }

    /* assign to existing data object: length - 1, offset 1 */
    ck_assert(lrpt_iq_data_from_iq(rdata1, data, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(rdata1), len - 1);

    lrpt_iq_data_to_complex(rcdata2, rdata1, 0, len - 1, NULL);

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_double_eq(creal(TEST_cdata[1 + i]), creal(rcdata2[i]));
        ck_assert_double_eq(cimag(TEST_cdata[1 + i]), cimag(rcdata2[i]));
    }

    lrpt_iq_data_free(rdata1);


    /* create new data object: full length */
    rdata2 = lrpt_iq_data_create_from_iq(data, 0, len, NULL);

    ck_assert_ptr_nonnull(rdata2);
    ck_assert_int_eq(lrpt_iq_data_length(rdata2), len);

    lrpt_iq_data_to_complex(rcdata1, rdata2, 0, len, NULL);

    for (int i = 0; i < len; i++) {
        ck_assert_double_eq(creal(TEST_cdata[i]), creal(rcdata1[i]));
        ck_assert_double_eq(cimag(TEST_cdata[i]), cimag(rcdata1[i]));
    }

    lrpt_iq_data_free(rdata2);

    /* create new data object: length - 1, offset 1 */
    rdata2 = lrpt_iq_data_create_from_iq(data, 1, len - 1, NULL);

    ck_assert_ptr_nonnull(rdata2);
    ck_assert_int_eq(lrpt_iq_data_length(rdata2), len - 1);

    lrpt_iq_data_to_complex(rcdata2, rdata2, 0, len - 1, NULL);

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_double_eq(creal(TEST_cdata[1 + i]), creal(rcdata2[i]));
        ck_assert_double_eq(cimag(TEST_cdata[1 + i]), cimag(rcdata2[i]));
    }

    lrpt_iq_data_free(rdata2);

    lrpt_iq_data_free(data);
}

START_TEST(test_append) {
    int len = NEL(TEST_cdata);

    lrpt_iq_data_t *data1 = lrpt_iq_data_create_from_complex(TEST_cdata, 0, len, NULL);
    lrpt_iq_data_t *data2 = lrpt_iq_data_create_from_complex(TEST_cdata, 0, len, NULL);

    /* full length addition */
    ck_assert(lrpt_iq_data_append(data2, data1, 0, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data2), 2 * len);

    /* partial addition: length - 1, offset 1 */
    ck_assert(lrpt_iq_data_append(data2, data1, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data2), 3 * len - 1);

    lrpt_iq_data_free(data1);
    lrpt_iq_data_free(data2);
}

Suite *iq_data_suite(void) {
    Suite *s;
    TCase *tc_alloc, *tc_length, *tc_construct, *tc_convert;

    s = suite_create("I/Q data");
    tc_alloc = tcase_create("allocation/freeing");
    tc_length = tcase_create("length manipulation");
    tc_construct = tcase_create("constructors");
    tc_convert = tcase_create("conversions");

    tcase_add_test(tc_alloc, test_alloc);
    tcase_add_test(tc_length, test_length);
    tcase_add_test(tc_length, test_resize);
    tcase_add_test(tc_construct, test_from_complex);
    tcase_add_test(tc_construct, test_from_doubles);
    tcase_add_test(tc_convert, test_to_complex);
    tcase_add_test(tc_convert, test_to_doubles);
    tcase_add_test(tc_construct, test_from_iq);
    tcase_add_test(tc_length, test_append);

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

    s = iq_data_suite();
    sr = srunner_create(s);

    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    num_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
