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

START_TEST(test_alloc) {
    lrpt_iq_data_t *data = lrpt_iq_data_alloc(0, NULL);

    ck_assert_ptr_nonnull(data);

    lrpt_iq_data_free(data);
}

START_TEST(test_length) {
    int len = 50;

    lrpt_iq_data_t *data1 = lrpt_iq_data_alloc(0, NULL);
    lrpt_iq_data_t *data2 = lrpt_iq_data_alloc(len, NULL);

    ck_assert_int_eq(lrpt_iq_data_length(data1), 0);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len);

    lrpt_iq_data_free(data1);
    lrpt_iq_data_free(data2);
}

START_TEST(test_resize) {
    int len = 50;

    lrpt_iq_data_t *data = lrpt_iq_data_alloc(0, NULL);

    ck_assert(lrpt_iq_data_resize(data, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data), len);

    ck_assert(lrpt_iq_data_resize(data, 0, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data), 0);

    lrpt_iq_data_free(data);
}

START_TEST(test_from_complex) {
    complex double cdata[] = {
        1.0 - 2.0 * I,
        4.5 + 2.9 * I,
        -3.1 + 9.5 * I,
        102.4 - 0.04 * I,
        2.4 - 7.5 * I,
        0.75 + 1.25 * I
    };
    int len = NEL(cdata);

    lrpt_iq_data_t *data1 = lrpt_iq_data_alloc(0, NULL);
    lrpt_iq_data_t *data2 = NULL;

    /* full length */
    ck_assert(lrpt_iq_data_from_complex(data1, cdata, 0, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data1), len);

    /* length - 1, offset 1 */
    ck_assert(lrpt_iq_data_from_complex(data1, cdata, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data1), len - 1);

    lrpt_iq_data_free(data1);

    /* full length */
    data2 = lrpt_iq_data_create_from_complex(cdata, 0, len, NULL);
    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len);
    lrpt_iq_data_free(data2);

    /* length - 1, offset 1 */
    data2 = lrpt_iq_data_create_from_complex(cdata, 1, len - 1, NULL);
    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len - 1);
    lrpt_iq_data_free(data2);
}

START_TEST(test_from_doubles) {
    double ddata[] = {
        1.0,   -2.0,
        4.5,    2.9,
       -3.1,    9.5,
        102.4, -0.04,
        2.4,   -7.5,
        0.75,   1.25
    };
    int len = NEL(ddata) / 2;

    lrpt_iq_data_t *data1 = lrpt_iq_data_alloc(0, NULL);
    lrpt_iq_data_t *data2 = NULL;

    /* full length */
    ck_assert(lrpt_iq_data_from_doubles(data1, ddata, 0, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data1), len);

    /* length - 1, offset 1 */
    ck_assert(lrpt_iq_data_from_doubles(data1, ddata, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data1), len - 1);

    lrpt_iq_data_free(data1);

    /* full length */
    data2 = lrpt_iq_data_create_from_doubles(ddata, 0, len, NULL);
    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len);
    lrpt_iq_data_free(data2);

    /* length - 1, offset 1 */
    data2 = lrpt_iq_data_create_from_doubles(ddata, 1, len - 1, NULL);
    ck_assert_ptr_nonnull(data2);
    ck_assert_int_eq(lrpt_iq_data_length(data2), len - 1);
    lrpt_iq_data_free(data2);
}

START_TEST(test_to_complex) {
    complex double cdata[] = {
        1.0 - 2.0 * I,
        4.5 + 2.9 * I,
        -3.1 + 9.5 * I,
        102.4 - 0.04 * I,
        2.4 - 7.5 * I,
        0.75 + 1.25 * I
    };
    int len = NEL(cdata);

    lrpt_iq_data_t *data = lrpt_iq_data_create_from_complex(cdata, 0, len, NULL);
    ck_assert_ptr_nonnull(data);
    ck_assert_int_eq(lrpt_iq_data_length(data), len);

    complex double rcdata1[len], rcdata2[len - 1];

    /* full length */
    ck_assert(lrpt_iq_data_to_complex(rcdata1, data, 0, len, NULL));

    /* length - 1, offset 1 */
    ck_assert(lrpt_iq_data_to_complex(rcdata2, data, 1, len - 1, NULL));

    for (int i = 0; i < len; i++) {
        ck_assert_double_eq(creal(cdata[i]), creal(rcdata1[i]));
        ck_assert_double_eq(cimag(cdata[i]), cimag(rcdata1[i]));
    }

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_double_eq(creal(cdata[1 + i]), creal(rcdata1[1 + i]));
        ck_assert_double_eq(cimag(cdata[1 + i]), cimag(rcdata1[1 + i]));
    }

    lrpt_iq_data_free(data);
}

START_TEST(test_to_doubles) {
    double ddata[] = {
        1.0,   -2.0,
        4.5,    2.9,
       -3.1,    9.5,
        102.4, -0.04,
        2.4,   -7.5,
        0.75,   1.25
    };
    int len = NEL(ddata) / 2;

    lrpt_iq_data_t *data = lrpt_iq_data_create_from_doubles(ddata, 0, len, NULL);
    ck_assert_ptr_nonnull(data);
    ck_assert_int_eq(lrpt_iq_data_length(data), len);

    double rddata1[2 * len], rddata2[2 * (len - 1)];

    /* full length */
    ck_assert(lrpt_iq_data_to_doubles(rddata1, data, 0, len, NULL));

    /* length - 1, offset 1 */
    ck_assert(lrpt_iq_data_to_doubles(rddata2, data, 1, len - 1, NULL));

    for (int i = 0; i < len; i++) {
        ck_assert_double_eq(ddata[2 * i], rddata1[2 * i]);
        ck_assert_double_eq(ddata[2 * i + 1], rddata1[2 * i + 1]);
    }

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_double_eq(ddata[2 + 2 * i], rddata1[2 + 2 * i]);
        ck_assert_double_eq(ddata[2 + 2 * i + 1], rddata1[2 + 2 * i + 1]);
    }

    lrpt_iq_data_free(data);
}

START_TEST(test_from_iq) {
    complex double cdata[] = {
        1.0 - 2.0 * I,
        4.5 + 2.9 * I,
        -3.1 + 9.5 * I,
        102.4 - 0.04 * I,
        2.4 - 7.5 * I,
        0.75 + 1.25 * I
    };
    int len = NEL(cdata);

    lrpt_iq_data_t *data = lrpt_iq_data_create_from_complex(cdata, 0, len, NULL);
    lrpt_iq_data_t *rdata1 = lrpt_iq_data_alloc(0, NULL);
    lrpt_iq_data_t *rdata2 = NULL;
    complex double rcdata1[len];
    complex double rcdata2[len - 1];

    /* full length */
    ck_assert(lrpt_iq_data_from_iq(rdata1, data, 0, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(rdata1), len);

    lrpt_iq_data_to_complex(rcdata1, rdata1, 0, len, NULL);

    for (int i = 0; i < len; i++) {
        ck_assert_double_eq(creal(cdata[i]), creal(rcdata1[i]));
        ck_assert_double_eq(cimag(cdata[i]), cimag(rcdata1[i]));
    }

    /* length - 1, offset 1 */
    ck_assert(lrpt_iq_data_from_iq(rdata1, data, 1, len - 1, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(rdata1), len - 1);

    lrpt_iq_data_to_complex(rcdata2, rdata1, 0, len - 1, NULL);

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_double_eq(creal(cdata[1 + i]), creal(rcdata2[i]));
        ck_assert_double_eq(cimag(cdata[1 + i]), cimag(rcdata2[i]));
    }

    lrpt_iq_data_free(rdata1);

    /* full length */
    rdata2 = lrpt_iq_data_create_from_iq(data, 0, len, NULL);
    ck_assert_ptr_nonnull(rdata2);
    ck_assert_int_eq(lrpt_iq_data_length(rdata2), len);

    lrpt_iq_data_to_complex(rcdata1, rdata2, 0, len, NULL);

    for (int i = 0; i < len; i++) {
        ck_assert_double_eq(creal(cdata[i]), creal(rcdata1[i]));
        ck_assert_double_eq(cimag(cdata[i]), cimag(rcdata1[i]));
    }

    lrpt_iq_data_free(rdata2);

    /* length - 1, offset 1 */
    rdata2 = lrpt_iq_data_create_from_iq(data, 1, len - 1, NULL);
    ck_assert_ptr_nonnull(rdata2);
    ck_assert_int_eq(lrpt_iq_data_length(rdata2), len - 1);

    lrpt_iq_data_to_complex(rcdata2, rdata2, 0, len - 1, NULL);

    for (int i = 0; i < (len - 1); i++) {
        ck_assert_double_eq(creal(cdata[1 + i]), creal(rcdata2[i]));
        ck_assert_double_eq(cimag(cdata[1 + i]), cimag(rcdata2[i]));
    }

    lrpt_iq_data_free(rdata2);

    lrpt_iq_data_free(data);
}

START_TEST(test_append) {
    complex double cdata[] = {
        1.0 - 2.0 * I,
        4.5 + 2.9 * I,
        -3.1 + 9.5 * I,
        102.4 - 0.04 * I,
        2.4 - 7.5 * I,
        0.75 + 1.25 * I
    };
    int len = NEL(cdata);

    lrpt_iq_data_t *data1 = lrpt_iq_data_create_from_complex(cdata, 0, len, NULL);
    lrpt_iq_data_t *data2 = lrpt_iq_data_create_from_complex(cdata, 0, len, NULL);

    /* full length */
    ck_assert(lrpt_iq_data_append(data2, data1, 0, len, NULL));
    ck_assert_int_eq(lrpt_iq_data_length(data2), 2 * len);

    /* length - 1, offset 1 */
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

    srunner_run_all(sr, CK_NORMAL);
    num_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
