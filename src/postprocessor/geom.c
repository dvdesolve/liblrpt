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
 * Author: Rich Griffiths
 * Author: Neoklis Kyriazis
 * Author: Viktor Drobot
 */

/** \cond INTERNAL_API_DOCS */

/** \file
 *
 * Geometric image manipulation routines.
 */

/*************************************************************************************************/

#include "geom.h"

#include "../../include/lrpt.h"
#include "../liblrpt/error.h"
#include "../liblrpt/image.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************************************************************/

/* Half of the max scan angle, in radians */
static double RECT_PHI_MAX = 0.9425;

/* Average Earth radius */
static double RECT_EARTH_R = 6370.0;

/*************************************************************************************************/

/** Calculate beta - the angle between observed pixel and pixel at nadir.
 *
 * Angle is calculated from the Earth's center to the observed pixels. Newton-Raphson method is
 * used to numerically solve the following equation:
 * R * sin(beta) - (A + R * (1 - cos(beta))) * tan(phi) = 0,
 * where A = satellite altitude, R = Earth radius, phi = scanner angle.
 *
 * \param beta0 Initial beta approximation (0.1 is a good starting value).
 * \param phi Scanner angle.
 * \param altitude Satellite altitude, in km.
 *
 * \return Beta angle.
 */
static double calc_beta(
        double beta0,
        double phi,
        double altitude);

/** Rectify LRPT image without pixel interpolation.
 *
 * Original algorithm by Rich Griffiths.
 *
 * \param image Pointer to the image object.
 * \param altitude Satellite altitude, in km.
 *
 * \return Pointer to the rectified image object or \c NULL in case of error.
 */
static lrpt_image_t *rectify_w2rg(
        lrpt_image_t *image,
        double altitude);

/** Rectify LRPT image with pixel interpolation.
 *
 * Modified algorithm by Neoklis Kyriazis.
 *
 * \param image Pointer to the image object.
 * \param altitude Satellite altitude, in km.
 *
 * \return Pointer to the rectified image object or \c NULL in case of error.
 */
static lrpt_image_t *rectify_5b4az(
        lrpt_image_t *image,
        double altitude);

/*************************************************************************************************/

/* calc_beta() */
static double calc_beta(
        double beta0,
        double phi,
        double altitude) {
    const double tan_phi = tan(phi); /* Tangent of scanner's angle */
    const double aRp1 = (1.0 + altitude / RECT_EARTH_R) * tan_phi; /* tan(phi) * (1 + A / R) */

    double beta_np1 = 0.0; /* Successive beta value approximation */
    double delta = 1.0; /* Used by Newton-Raphson algorithm, convergence criterion */

    /* Loop over Newton-Raphson iterations */
    while (fabs(delta) > 1.0e-5) {
        const double sin_b = sin(beta0);
        const double cos_b = cos(beta0);
        const double f_beta = (sin_b + cos_b * tan_phi - aRp1);
        const double df_beta = (cos_b - sin_b * tan_phi);

        /* New improved estimate of beta */
        beta_np1 = (beta0 - f_beta / df_beta);
        delta = (beta_np1 - beta0) / beta_np1;
        beta0 = beta_np1;
    }

    return beta_np1;
}

/*************************************************************************************************/

/* rectify_w2rg() */
static lrpt_image_t *rectify_w2rg(
        lrpt_image_t *image,
        double altitude) {
    /* New positions of rectified pixels */
    const size_t w2 = (image->width / 2);
    double *newpos = calloc(w2, sizeof(double));

    if (!newpos)
        return NULL;

    /* Gaps between pixels */
    uint8_t *gaps = calloc(w2 - 1, sizeof(uint8_t));

    if (!gaps) {
        free(newpos);

        return NULL;
    }

    /* Pixel-to-pixel stride of the scanner, in radians */
    const double dphi = (2.0 * RECT_PHI_MAX / (image->width - 1));

    /* Max beta angle, corresponding to max phi */
    const double beta_max = calc_beta(0.1, RECT_PHI_MAX, altitude);

    /* The size of first sub-satellite pixel */
    double resolution = (2.0 * calc_beta(beta_max, dphi / 2.0, altitude));
    double beta0 = (resolution / 2.0); /* Needed for later estimation of beta */

    /* Width of the rectified image, as float */
    const double width_f = ((2.0 * beta_max) / resolution);

    /* Width of the rectified image, in pixels */
    size_t rect_width = width_f;

    /* Round to the nearest multiple of 8 because of the MCU size */
    rect_width -= (rect_width % 8);

    const size_t rw2 = (rect_width / 2);

    /* Reference size of pixels in rectified image */
    resolution = ((2.0 * beta_max) / (rect_width - 1));

    /* Calculate the correct position of each pixel */
    for (size_t i = 0; i < w2; i++) {
        const double phi = (i + 0.5) * dphi; /* Instantaneous scan angle */
        const double beta = calc_beta(beta0, phi, altitude); /* Corresponding beta angle */

        beta0 = beta;

        newpos[i] = (beta / resolution);
    }

    /* Unfilled gaps between true pixel locations */
    double unusedspace = 0.0;

    /* Calculate number of gaps between rectified pixels */
    for (size_t i = 0; i < (w2 - 1); i++) {
        unusedspace += (newpos[i + 1] - newpos[i] - 1.0);

        if (unusedspace >= 4.0) {
            gaps[i] = 4;
            unusedspace -= 4.0;
        }
        else if (unusedspace >= 3.0) {
            gaps[i] = 3;
            unusedspace -= 3.0;
        }
        else if (unusedspace >= 2.0) {
            gaps[i] = 2;
            unusedspace -= 2.0;
        }
        else if (unusedspace >= 1.0) {
            gaps[i] = 1;
            unusedspace -= 1.0;
        }
        else
            gaps[i] = 0;
    }

    lrpt_image_t *new_img = lrpt_image_alloc(rect_width, image->height, NULL);

    if (!new_img) {
        free(gaps);
        free(newpos);

        return NULL;
    }

    for (uint8_t i = 0; i < 6; i++) {
        /* Rectify image buffer line by line */
        for (size_t j = 0; j < image->height; j++) {
            /* Middle of each line in the rectified image */
            size_t rect_buff_right = (j * rect_width + rw2);
            size_t rect_buff_left = (rect_buff_right - 1);

            /* Middle of each line in the original image */
            size_t orig_buff_right = (j * image->width + w2);
            size_t orig_buff_left = (orig_buff_right - 1);

            /* Put middle pixel right from input to the rectified line */
            uint8_t byteA_right = image->channels[i][orig_buff_right++];
            new_img->channels[i][rect_buff_right++] = byteA_right;

            /* Put middle pixel left from input to the rectified line */
            uint8_t byteA_left = image->channels[i][orig_buff_left--];
            new_img->channels[i][rect_buff_left--] = byteA_left;

            uint8_t byteB_right = 0, byteB_left = 0;

            /* Traverse the rest of the line */
            for (size_t k = 0; k < (w2 - 1); k++) {
                /* Get the next pixel */
                byteB_right = image->channels[i][orig_buff_right++];
                byteB_left = image->channels[i][orig_buff_left--];

                /* Fill the gap between the two pixels */
                switch (gaps[k]) {
                    case 0:
                        break;

                    case 1:
                        new_img->channels[i][rect_buff_right++] = (byteA_right + byteB_right) / 2;
                        new_img->channels[i][rect_buff_left--] = (byteA_left + byteB_left) / 2;

                        break;

                    case 2:
                        new_img->channels[i][rect_buff_right++] = (2 * byteA_right + byteB_right) / 3;
                        new_img->channels[i][rect_buff_right++] = (byteA_right + 2 * byteB_right) / 3;
                        new_img->channels[i][rect_buff_left--] = (2 * byteA_left + byteB_left) / 3;
                        new_img->channels[i][rect_buff_left--] = (byteA_left + 2 * byteB_left) / 3;

                        break;

                    case 3:
                        new_img->channels[i][rect_buff_right++] = (2 * byteA_right + byteB_right) / 3;
                        new_img->channels[i][rect_buff_right++] = (byteA_right + byteB_right) / 2;
                        new_img->channels[i][rect_buff_right++] = (byteA_right + 2 * byteB_right) / 3;
                        new_img->channels[i][rect_buff_left--] = (2 * byteA_left + byteB_left) / 3;
                        new_img->channels[i][rect_buff_left--] = (byteA_left + byteB_left) / 2;
                        new_img->channels[i][rect_buff_left--] = (byteA_left + 2 * byteB_left) / 3;

                        break;

                    case 4:
                        new_img->channels[i][rect_buff_right++] = (3 * byteA_right + byteB_right) / 4;
                        new_img->channels[i][rect_buff_right++] = (2 * byteA_right + byteB_right) / 3;
                        new_img->channels[i][rect_buff_right++] = (byteA_right + 2 * byteB_right) / 3;
                        new_img->channels[i][rect_buff_right++] = (byteA_right + 3 * byteB_right) / 4;
                        new_img->channels[i][rect_buff_left--] = (3 * byteA_left + byteB_left) / 4;
                        new_img->channels[i][rect_buff_left--] = (2 * byteA_left + byteB_left) / 3;
                        new_img->channels[i][rect_buff_left--] = (byteA_left + 2 * byteB_left) / 3;
                        new_img->channels[i][rect_buff_left--] = (byteA_left + 3 * byteB_left) / 4;

                        break;

                    default:
                        break;
                }

                /* Write the pixel */
                new_img->channels[i][rect_buff_right++] = byteB_right;
                byteA_right = byteB_right;
                new_img->channels[i][rect_buff_left--] = byteB_left;
                byteA_left = byteB_left;
            }
        }
    }

    free(gaps);
    free(newpos);

    lrpt_image_free(image);

    return new_img;
}

/*************************************************************************************************/

/* rectify_5b4az() */
static lrpt_image_t *rectify_5b4az(
        lrpt_image_t *image,
        double altitude) {
    /* Pixel-to-pixel stride of the scanner, in radians */
    const double dphi = (2.0 * RECT_PHI_MAX / (image->width - 1));

    /* Max beta angle, corresponding to max phi */
    const double beta_max = calc_beta(0.1, RECT_PHI_MAX, altitude);

    /* Width of the rectified image, as float */
    const double width_f = (beta_max / calc_beta(beta_max, dphi / 2.0, altitude));
    double beta0 = (beta_max / width_f); /* Needed for later estimation of beta */

    /* Width of the rectified image, in pixels */
    size_t rect_width = width_f;

    /* Round to the nearest multiple of 8 because of the MCU size */
    rect_width -= (rect_width % 8);

    const size_t rw2 = (rect_width / 2);

    /* The center pixel (first to right of the center) of input image */
    const size_t w2 = (image->width / 2);

    /* Prepare indices buffer for pixel values extrapolation during rectification */
    size_t *indices = calloc(rw2, sizeof(size_t));

    if (!indices)
        return NULL;

    /* Prepare extrapolation factors buffer */
    double *factors = calloc(rw2, sizeof(double));

    if (!factors) {
        free(indices);

        return NULL;
    }

    /* Distance between centers of adjacent rectified image pixels */
    const double delta_center = ((2.0 * beta_max) / (rect_width - 1));

    double prev_center = -calc_beta(beta0, dphi / 2.0, altitude);

    size_t rect_idx = 0, orig_idx = 0;

    /* Repeat for all pixels in rectified image buffer */
    while (rect_idx < rw2) {
        const double phi = (orig_idx + 0.5) * dphi; /* Instantaneous scan angle */
        const double beta = calc_beta(beta0, phi, altitude); /* Corresponding beta angle */

        beta0 = beta;

        /* The center position of original image when projected onto the surface of the Earth */
        const double orig_pixel_center = beta;

        /* The center position of the rectified image when projected onto the surface of the Earth */
        const double rect_pixel_center = (rect_idx + 0.5) * delta_center;

        /* If rectified pixel overtakes an original pixel, then advance the original pixel
         * index and save its center position as the previous center
         */
        if (rect_pixel_center > orig_pixel_center) {
            orig_idx++;
            prev_center = orig_pixel_center;

            continue;
        }

        /* If rectified pixel center position is less than the original pixel position,
         * save the original pixel index in the reference indices buffer */
        indices[rect_idx] = orig_idx;

        /* The extrapolation factor is the distance of the rectified pixel center
         * from the reference pixel center, divided by the distance between the centers
         * of the reference pixel and the previous one */
        factors[rect_idx] =
            ((rect_pixel_center - orig_pixel_center) / (prev_center - orig_pixel_center));

        rect_idx++;
    }

    lrpt_image_t *new_img = lrpt_image_alloc(rect_width, image->height, NULL);

    if (!new_img) {
        free(factors);
        free(indices);

        return NULL;
    }

    for (uint8_t i = 0; i < 6; i++) {
        /* TODO that should be rechecked for possible arithmetic over/underflows while calculating diffs etc */
        /* Rectify image buffer line by line */
        for (size_t j = 0; j < image->height; j++) {
            /* Indices to input and output image buffers */
            size_t rect_buff_idx = (j * rect_width + rw2);
            const size_t stride = (j * image->width + w2);;

            /* Extrapolate values of each pixel in output buffer. This is from the center right pixel
             * to the right edge.
             */
            for (size_t k = 0; k < rw2; k++) {
                /* This index points to the pixel in input buffer that is to be used as the
                 * reference for extrapolating pixel value
                 */
                const size_t in_buff_idx = (indices[k] + stride);

                /* This is the diff in values of the reference pixel and the one before it,
                 * and it is to be used for linear extrapolation of the value of the current pixel
                 * of the output buffer
                 */
                const int16_t pxval_diff =
                    (image->channels[i][in_buff_idx - 1] - image->channels[i][in_buff_idx]);

                /* Pixel value is the reference input pixel value plus a proportion of the value diff
                 * above, according to the extrapolation factor
                 */
                new_img->channels[i][rect_buff_idx] =
                    (image->channels[i][in_buff_idx] + pxval_diff * factors[k]);

                rect_buff_idx++;
            }

            /* Extrapolate values of each pixel in output buffer as above. This is from the
             * center left pixel to the left edge
             */
            rect_buff_idx = (j * rect_width + rw2);

            for (size_t k = 0; k < rw2; k++) {
                const size_t in_buff_idx = (stride - indices[k]);

                const int16_t pxval_diff =
                    (image->channels[i][in_buff_idx - 1] - image->channels[i][in_buff_idx]);

                rect_buff_idx--;

                new_img->channels[i][rect_buff_idx] =
                    (image->channels[i][in_buff_idx - 1] - pxval_diff * factors[k]);
            }
        }
    }

    free(factors);
    free(indices);

    lrpt_image_free(image);

    return new_img;
}

/*************************************************************************************************/

/* lrpt_postproc_image_flip() */
bool lrpt_image_flip(
        lrpt_image_t *image,
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

    const size_t iw = image->width;
    const size_t ih = image->height;

    for (uint8_t i = 0; i < 6; i++) {
        for (size_t y = 0; y < (ih / 2 + ih % 2); y++) {
            const size_t border = (y == (ih / 2)) ? (iw / 2) : iw;

            for (size_t x = 0; x < border; x++) {
                const uint8_t tmp = image->channels[i][y * iw + x];

                image->channels[i][y * iw + x] = image->channels[i][(ih - 1 - y) * iw + (iw - 1 - x)];
                image->channels[i][(ih - 1 - y) * iw + (iw - 1 - x)] = tmp;
            }
        }
    }

    return true;
}

/*************************************************************************************************/

/* lrpt_image_rectify() */
lrpt_image_t *lrpt_image_rectify(
        lrpt_image_t *image,
        double altitude,
        bool interpolate,
        lrpt_error_t *err) {
    /* Original algorithm was implemented by Rich Griffiths. It compensates tangential scale
     * distortion and distortion caused by the curvature of Earth.
     */
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

        return NULL;
    }

    if ((image->width * image->height) == 0)
        return NULL;

    lrpt_image_t *result = NULL;

    if (!interpolate) {
        result = rectify_w2rg(image, altitude);

        if (!result) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                        "Can't rectify image with plain algorithm (W2RG)");

            return NULL;
        }
    }
    else {
        result = rectify_5b4az(image, altitude);

        if (!result) {
            if (err)
                lrpt_error_set(err, LRPT_ERR_LVL_ERROR, LRPT_ERR_CODE_DATAPROC,
                        "Can't rectify image with interpolation algorithm (5B4AZ)");

            return NULL;
        }
    }

    return result;
}

/*************************************************************************************************/

/** \endcond */
