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
 * Author: Karel Zuiderveld
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Image normalization routines.
 */

/*************************************************************************************************/

#include "normalize.h"

#include "../../include/lrpt.h"
#include "../liblrpt/error.h"
#include "../liblrpt/image.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/

/* Classic normalization settings */
static const uint8_t NORM_HIST_MIN_BLACK = 2; /* Needed to avoid black bands in source images */
static const uint8_t NORM_HIST_CUTOFF_BLACK = 1;
static const uint8_t NORM_HIST_CUTOFF_WHITE = 1;

/* CLAHE settings */
static const uint8_t NORM_CLAHE_N_CONREG_X = 8;
static const uint8_t NORM_CLAHE_N_CONREG_Y = 8;
static const uint8_t NORM_CLAHE_N_CONREG_X_MAX = 16; /**< Max number of context. regions along X */
static const uint8_t NORM_CLAHE_N_CONREG_Y_MAX = 16; /**< Max number of context. regions along Y */
static const uint16_t NORM_CLAHE_N_BINS = 256;
static const uint16_t NORM_CLAHE_N_GREYS = 256;
static const double NORM_CLAHE_CLIPLIMIT = 3.0;

/*************************************************************************************************/

/** Perform contrast limited adaptive histogram equalization.
 *
 * Selecting small number of bins for histogram (\p n_bins) will accelerate processing but at the
 * expense of quality. Clip limit (\p limit) lesser than 1 produces standard AHE; greater values
 * will give more contrast.
 *
 * \param image Pointer to the pixel data (by-row).
 * \param width Width of the image.
 * \param height Height of the image.
 * \param grey_min Minimum grey value of input image.
 * \param grey_max Maximum grey value of input image.
 * \param n_crx Number of contextual regions in the X direction (should be between 2 and
 * #NORM_CLAHE_N_CONREG_X_MAX).
 * \param n_cry Number of contextual regions in the Y direction (should be between 2 and
 * #NORM_CLAHE_N_CONREG_Y_MAX).
 * \param n_bins Number of bins for histogram (defines dynamic range).
 * \param limit Normalized clip limit value.
 */
static void do_clahe(
        uint8_t *image,
        size_t width,
        size_t height,
        uint8_t grey_min,
        uint8_t grey_max,
        uint8_t n_crx,
        uint8_t n_cry,
        uint16_t n_bins,
        double limit);

/** Make histogram from array map.
 *
 * \param image Pointer to the image data.
 * \param width Width of the image.
 * \param sz_crx Size of contextual region in X dimension.
 * \param sz_cry Size of contextual region in Y dimension.
 * \param hist Pointer to the histogram.
 * \param n_greys Number of grey levels.
 * \param lut Lookup table used for scaling.
 */
static void make_histogram(
        const uint8_t *image,
        size_t width,
        size_t sz_crx,
        size_t sz_cry,
        size_t *hist,
        uint16_t n_greys,
        uint8_t *lut);

/** Perform histogram clipping.
 *
 * This function performs clipping of the histogram and bins redistribution. Excess pixels are
 * counted after clipping and then equally redistributed across the whole histogram.
 *
 * \param hist Pointer to the histogram.
 * \param n_greys Number of grey levels.
 * \param clip_limit Clip limit.
 */
static void clip_histogram(
        size_t *hist,
        uint16_t n_greys,
        size_t clip_limit);

/** Perform histogram mapping.
 *
 * Calculate the equalized lookup table (mapping table) by summing the input histogram. Lookup
 * table will be rescaled in range [\p grey_min; \p grey_max].
 *
 * \param hist Pointer to the histogram.
 * \param grey_min Minimum grey value.
 * \param grey_max Maximum grey value.
 * \param n_greys Number of grey levels.
 * \param n_pixels Number of pixels.
 */
static void map_histogram(
        size_t *hist,
        uint8_t grey_min,
        uint8_t grey_max,
        uint16_t n_greys,
        size_t n_pixels);

/** Calculate new grey level for pixels within submatrix of size \p sz_x per \p sz_y by the use
 * of bilinear interpolation.
 *
 * \param image Pointer to the image data.
 * \param width Image width.
 * \param map_lu Mapping for left-upper grey levels from histogram.
 * \param map_ru Mapping for right-upper grey levels from histogram.
 * \param map_lb Mapping for left-bottom grey levels from histogram.
 * \param map_rb Mapping for right-bottom grey levels from histogram.
 * \param sz_x X size of image submatrix.
 * \param sz_y Y size of image submatrix.
 * \param lut Lookup table with grey values to bins mapping.
 */
static void interpolate(
        uint8_t *image,
        size_t width,
        size_t *map_lu,
        size_t *map_ru,
        size_t *map_lb,
        size_t *map_rb,
        size_t sz_x,
        size_t sz_y,
        uint8_t *lut);

/*************************************************************************************************/

/* make_histogram() */
static void make_histogram(
        const uint8_t *image,
        size_t width,
        size_t sz_crx,
        size_t sz_cry,
        size_t *hist,
        uint16_t n_greys,
        uint8_t *lut) {
    /* Clear histogram */
    memset(hist, 0, sizeof(size_t) * n_greys);

    for (size_t i = 0; i < sz_cry; i++) {
        const uint8_t *img_ptr = (image + sz_crx);

        while (image < img_ptr)
            hist[lut[*image++]]++;

        img_ptr += width;

        /* Go to the beginning of next row */
        image = (img_ptr - sz_crx);
    }
}

/*************************************************************************************************/

/* clip_histogram() */
static void clip_histogram(
        size_t *hist,
        uint16_t n_greys,
        size_t clip_limit) {
    size_t n_excess = 0;

    for (uint16_t i = 0; i < n_greys; i++) {
        /* Calculate total number of excess pixels */
        if (hist[i] > clip_limit)
            n_excess += (hist[i] - clip_limit);
    }

    /* Clip histogram and redistribute excess pixels in each bin */
    /* Average bin increment */
    size_t bin_inc = (n_excess / n_greys);

    /* Bins that are larger than upper bound will be set to clip limit */
    size_t upper_bnd = (clip_limit - bin_inc);

    for (uint16_t i = 0; i < n_greys; i++) {
        if (hist[i] > clip_limit)
            hist[i] = clip_limit;
        else {
            if (hist[i] > upper_bnd) {
                /* High bin count */
                n_excess -= (hist[i] - upper_bnd);
                hist[i] = clip_limit;
            }
            else {
                /* Low bin count */
                n_excess -= bin_inc;
                hist[i] += bin_inc;
            }
        }
    }

    while (n_excess) {
        /* Redistribute remaining excess */
        size_t *end_ptr = (hist + n_greys);
        size_t *tmp_hist = hist;

        while (n_excess && (tmp_hist < end_ptr)) {
            size_t step_size = (n_greys / n_excess);

            if (step_size < 1)
                step_size = 1; /* Step size should be at least 1 */

            for (
                    size_t *bin_ptr = tmp_hist;
                    (bin_ptr < end_ptr) && n_excess;
                    bin_ptr += step_size) {
                if (*bin_ptr < clip_limit) {
                    (*bin_ptr)++;
                    n_excess--; /* Reduce excess */
                }
            }

            /* Restart redistribution on other bin location */
            tmp_hist++;
        }
    }
}

/*************************************************************************************************/

/* map_histogram() */
static void map_histogram(
        size_t *hist,
        uint8_t grey_min,
        uint8_t grey_max,
        uint16_t n_greys,
        size_t n_pixels) {
    const double scale = ((double)(grey_max - grey_min) / n_pixels);
    size_t sum = 0;

    for (uint16_t i = 0; i < n_greys; i++) {
        sum += hist[i];
        hist[i] = (grey_min + sum * scale);

        if (hist[i] > grey_max)
            hist[i] = grey_max;
    }
}

/*************************************************************************************************/

/* interpolate() */
static void interpolate(
        uint8_t *image,
        size_t width,
        size_t *map_lu,
        size_t *map_ru,
        size_t *map_lb,
        size_t *map_rb,
        size_t sz_x,
        size_t sz_y,
        uint8_t *lut) {
    /* Pointer increment after processing row */
    const size_t inc = (width - sz_x);

    /* Normalization factor */
    size_t factor = (sz_x * sz_y);

    /* If normalization factor is not a power of 2, use division instead */
    if (factor & (factor - 1)) {
        for (
                size_t y_coeff = 0, y_inv_coeff = sz_y;
                y_coeff < sz_y;
                y_coeff++, y_inv_coeff--, image += inc) {
            for (
                    size_t x_coeff = 0, x_inv_coeff = sz_x;
                    x_coeff < sz_x;
                    x_coeff++, x_inv_coeff--) {
                uint8_t grey_val = lut[*image]; /* Get histogram bin value */

                *image++ = (uint8_t)(
                        (
                         y_inv_coeff * (x_inv_coeff * map_lu[grey_val] + x_coeff * map_ru[grey_val]) +
                         y_coeff * (x_inv_coeff * map_lb[grey_val] + x_coeff * map_rb[grey_val])
                        ) / factor);
            }
        }
    }
    else {
        uint8_t shft = 0;

        /* Avoid the division and use a right shift instead */
        while (factor >>= 1)
            shft++; /* Calculate log2 of normalization factor */

        for (
                size_t y_coeff = 0, y_inv_coeff = sz_y;
                y_coeff < sz_y;
                y_coeff++, y_inv_coeff--, image += inc) {
            for (
                    size_t x_coeff = 0, x_inv_coeff = sz_x;
                    x_coeff < sz_x;
                    x_coeff++, x_inv_coeff--) {
                uint8_t grey_val = lut[*image]; /* Get histogram bin value */

                *image++ = (uint8_t)(
                        (
                         y_inv_coeff * (x_inv_coeff * map_lu[grey_val] + x_coeff * map_ru[grey_val]) +
                         y_coeff * (x_inv_coeff * map_lb[grey_val] + x_coeff * map_rb[grey_val])
                        ) >> shft);
            }
        }
    }
}

/*************************************************************************************************/

/* TODO return bool */
/* do_clahe() */
static void do_clahe(
        uint8_t *image,
        size_t width,
        size_t height,
        uint8_t grey_min,
        uint8_t grey_max,
        uint8_t n_crx,
        uint8_t n_cry,
        uint16_t n_bins,
        double limit) {
    /* Original code was written by Karel Zuiderveld. For more information please see
     * "Contrast Limited Adaptive Histogram Equalization", "Graphics Gems IV",
     * Academic Press, 1994.
     */

    /* In case of errors just silently return without touching the image */
    if (
            ((n_crx < 2) || (n_crx > NORM_CLAHE_N_CONREG_X_MAX)) ||
            ((n_cry < 2) || (n_cry > NORM_CLAHE_N_CONREG_Y_MAX)) ||
            (((width % n_crx) != 0) || ((height % n_cry) != 0)) ||
            (grey_min >= grey_max) ||
            (n_bins == 0) ||
            ((limit == 1.0) || (limit <= 0)))
        return;

    /* Allocate map array */
    size_t *map = calloc(n_crx * n_cry * n_bins, sizeof(size_t));

    if (!map)
        return;

    /* Calculate actual sizes of contextual regions */
    const size_t sz_crx = (width / n_crx);
    const size_t sz_cry = (height / n_cry);
    const size_t n_cr_pxls = (sz_crx * sz_cry);

    /* Calculate integer clip limit */
    size_t clip_limit = (limit * n_cr_pxls / n_bins);

    if (clip_limit < 1)
        clip_limit = 1;

    /* Create and populate lookup table for image scaling */
    uint8_t lut[NORM_CLAHE_N_GREYS];
    const uint8_t sz_bin = (1 + (grey_max - grey_min) / n_bins);

    for (uint8_t i = grey_min; i <= grey_max; i++) {
        lut[i] = ((i - grey_min) / sz_bin);

        if (i == grey_max)
            break;
    }

    /* Calculate grey level mappings for each contextual region */
    uint8_t *img_ptr = image;

    for (uint8_t j = 0; j < n_cry; j++) {
        for (uint8_t i = 0; i < n_crx; i++, img_ptr += sz_crx) {
            size_t *hist = (map + n_bins * (j * n_crx + i));

            make_histogram(img_ptr, width, sz_crx, sz_cry, hist, n_bins, lut);
            clip_histogram(hist, n_bins, clip_limit);
            map_histogram(hist, grey_min, grey_max, n_bins, n_cr_pxls);
        }

        /* Skip lines, set pointer */
        img_ptr += ((sz_cry - 1) * width);
    }

    /* Interpolate grey level mappings to get CLAHE image */
    img_ptr = image;

    for (uint8_t j = 0; j <= n_cry; j++) {
        size_t sub_y, y_u, y_b;

        if (j == 0) {
            /* Top row */
            sub_y = (sz_cry >> 1);
            y_u = 0;
            y_b = 0;
        }
        else if (j == n_cry) {
            /* Bottom row */
            sub_y = ((sz_cry + 1 ) >> 1);
            y_u = (n_cry - 1);
            y_b = y_u;
        }
        else {
            /* Somewhere in the middle */
            sub_y = sz_cry;
            y_u = (j - 1);
            y_b = (y_u + 1);
        }

        for (uint8_t i = 0; i <= n_crx; i++) {
            size_t sub_x, x_l, x_r;

            if (i == 0) {
                /* Left column */
                sub_x = (sz_crx >> 1);
                x_l = 0;
                x_r = 0;
            }
            else if (i == n_crx) {
                /* Right column */
                sub_x = ((sz_crx + 1 ) >> 1);
                x_l = (n_crx - 1);
                x_r = x_l;
            }
            else {
                /* Somewhere in the middle */
                sub_x = sz_crx;
                x_l = (i - 1);
                x_r = (x_l + 1);
            }

            size_t *lu_ptr = (map + n_bins * (y_u * n_crx + x_l));
            size_t *ru_ptr = (map + n_bins * (y_u * n_crx + x_r));
            size_t *lb_ptr = (map + n_bins * (y_b * n_crx + x_l));
            size_t *rb_ptr = (map + n_bins * (y_b * n_crx + x_r));

            interpolate(img_ptr, width, lu_ptr, ru_ptr, lb_ptr, rb_ptr, sub_x, sub_y, lut);

            /* Set pointer to the next matrix */
            img_ptr += sub_x;
        }

        img_ptr += ((sub_y - 1) * width);
    }

    /* Free resources */
    free(map);
}

/*************************************************************************************************/

/* lrpt_postproc_image_normalize() */
bool lrpt_postproc_image_normalize(
        lrpt_image_t *image,
        bool clahe,
        lrpt_error_t *err) {
    bool good = true;

    if (image && (image->height > 0)) {
        for (uint8_t i = 0; i < 6; i++)
            if (!image->channels[i]) {
                good = false;

                break;
            }
    }

    if (!image || !good) {
        if (err)
            lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_PARAM,
                    "LRPT image object is NULL or corrupted");

        return false;
    }

    if ((image->width * image->height) == 0)
        return true;

    size_t hist[256]; /* Intensity histogram */

    for (uint8_t i = 0; i < 6; i++) {
        /* Zero out histogram */
        memset(hist, 0, sizeof(size_t) * 256);

        /* Count up intensities */
        for (size_t j = 0; j < (image->width * image->height); j++)
            hist[image->channels[i][j]]++;

        /* Determine black/white cut-off counts */
        size_t black_cutoff = (image->width * image->height * NORM_HIST_CUTOFF_BLACK) / 100;
        size_t white_cutoff = (image->width * image->height * NORM_HIST_CUTOFF_WHITE) / 100;

        /* Find black cutoff intensity */
        size_t cnt = 0;
        uint8_t black_cutval = 0;

        for (black_cutval = NORM_HIST_MIN_BLACK; black_cutval < 255; black_cutval++) {
            cnt += hist[black_cutval];

            if (cnt >= black_cutoff)
                break;
        }

        /* Find white cutoff intensity */
        cnt = 0;
        uint8_t white_cutval = 0;

        for (white_cutval = 255; white_cutval > 0; white_cutval--) {
            cnt += hist[white_cutval];

            if (cnt >= white_cutoff)
                break;
        }

        /* Rescale pixels in image for required intensity range */
        if (white_cutval <= black_cutval)
            continue;

        uint8_t rng_in = (white_cutval - black_cutval);

        /* Perform histogram normalization on image */
        for (size_t j = 0; j < (image->width * image->height); j++) {
            /* Input image pixel values are relative to input black cut off. Clamp pixel values
             * within black and white cut off values.
             */
            uint8_t val = image->channels[i][j];

            if (val > white_cutval)
                val = white_cutval;
            else if (val < black_cutval)
                val = black_cutval;

            val -= black_cutval;

            /* Normalized pixel values are scaled according to the ratio of required pixel value
             * range to input pixel value range */
            image->channels[i][j] = (0 + (val * 255) / rng_in);
        }

        if (clahe)
            do_clahe(
                    image->channels[i],
                    image->width,
                    image->height,
                    0,
                    255,
                    NORM_CLAHE_N_CONREG_X,
                    NORM_CLAHE_N_CONREG_Y,
                    NORM_CLAHE_N_BINS,
                    NORM_CLAHE_CLIPLIMIT);
    }

    return true;
}

/*************************************************************************************************/

/** \endcond */
