/*
 * SpanDSP - a series of DSP components for telephony
 *
 * generate_sized_pages.c - Create a series of TIFF files in the various page sizes
 *                        and resolutions.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2006 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: generate_sized_pages.c,v 1.4 2006/11/20 00:00:27 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE

#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <string.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <tiffio.h>

#define T4_X_RESOLUTION_R4          4019
#define T4_X_RESOLUTION_R8          8037
#define T4_X_RESOLUTION_R16         16074

#define T4_Y_RESOLUTION_STANDARD    3850
#define T4_Y_RESOLUTION_FINE        7700
#define T4_Y_RESOLUTION_SUPERFINE   15400

struct
{
    const char *name;
    int x_res;
    int y_res;
    int width;
    int length;
} sequence[] =
{
    {
        "R8_385_A4.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_STANDARD,
        1728,
        1100
    },
    {
        "R16_385_A4.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_STANDARD,
        3456,
        1100
    },
    {
        "R8_77_A4.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_FINE,
        1728,
        2200
    },
    {
        "R16_77_A4.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_FINE,
        3456,
        2200
    },
    {
        "R8_154_A4.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_SUPERFINE,
        1728,
        4400
    },
    {
        "R16_154_A4.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_SUPERFINE,
        3456,
        4400
    },
    {
        "R8_385_B4.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_STANDARD,
        2048,
        1200
    },
    {
        "R16_385_B4.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_STANDARD,
        4096,
        1200
    },
    {
        "R8_77_B4.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_FINE,
        2048,
        2400
    },
    {
        "R16_77_B4.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_FINE,
        4096,
        2400
    },
    {
        "R8_154_B4.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_SUPERFINE,
        2048,
        4800
    },
    {
        "R16_154_B4.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_SUPERFINE,
        4096,
        4800
    },
    {
        "R8_385_A3.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_STANDARD,
        2432,
        1556
    },
    {
        "R16_385_A3.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_STANDARD,
        4864,
        1556
    },
    {
        "R8_77_A3.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_FINE,
        2432,
        3111
    },
    {
        "R16_77_A3.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_FINE,
        4864,
        3111
    },
    {
        "R8_154_A3.tif",
        T4_X_RESOLUTION_R8,
        T4_Y_RESOLUTION_SUPERFINE,
        2432,
        6222
    },
    {
        "R16_154_A3.tif",
        T4_X_RESOLUTION_R16,
        T4_Y_RESOLUTION_SUPERFINE,
        4864,
        6222
    },
    {
        NULL,
        0,
        0,
        0,
        0
    },
};

int main(int argc, char *argv[])
{
    int row;
    uint8_t image_buffer[1024];
    TIFF *tiff_file;
    struct tm *tm;
    time_t now;
    char buf[133];
    float x_resolution;
    float y_resolution;
    int i;

    for (i = 0;  sequence[i].name;  i++)
    {
        if ((tiff_file = TIFFOpen(sequence[i].name, "w")) == NULL)
            exit(2);

        /* Prepare the directory entry fully before writing the image, or libtiff complains */
        TIFFSetField(tiff_file, TIFFTAG_COMPRESSION, COMPRESSION_CCITT_T6);
        TIFFSetField(tiff_file, TIFFTAG_IMAGEWIDTH, sequence[i].width);
        TIFFSetField(tiff_file, TIFFTAG_BITSPERSAMPLE, 1);
        TIFFSetField(tiff_file, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tiff_file, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tiff_file, TIFFTAG_ROWSPERSTRIP, -1L);
        TIFFSetField(tiff_file, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tiff_file, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
        TIFFSetField(tiff_file, TIFFTAG_FILLORDER, FILLORDER_LSB2MSB);
    
        x_resolution = sequence[i].x_res/100.0f;
        y_resolution = sequence[i].y_res/100.0f;
        TIFFSetField(tiff_file, TIFFTAG_XRESOLUTION, floorf(x_resolution*2.54f + 0.5f));
        TIFFSetField(tiff_file, TIFFTAG_YRESOLUTION, floorf(y_resolution*2.54f + 0.5f));
        TIFFSetField(tiff_file, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    
        TIFFSetField(tiff_file, TIFFTAG_SOFTWARE, "spandsp");
        if (gethostname(buf, sizeof(buf)) == 0)
            TIFFSetField(tiff_file, TIFFTAG_HOSTCOMPUTER, buf);
    
        TIFFSetField(tiff_file, TIFFTAG_IMAGEDESCRIPTION, "Blank test image");
        TIFFSetField(tiff_file, TIFFTAG_MAKE, "soft-switch.org");
        TIFFSetField(tiff_file, TIFFTAG_MODEL, "test data");

        time(&now);
        tm = localtime(&now);
        sprintf(buf,
    	        "%4d/%02d/%02d %02d:%02d:%02d",
                tm->tm_year + 1900,
                tm->tm_mon + 1,
                tm->tm_mday,
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec);
        TIFFSetField(tiff_file, TIFFTAG_DATETIME, buf);
    
        TIFFSetField(tiff_file, TIFFTAG_IMAGELENGTH, sequence[i].length);
        TIFFSetField(tiff_file, TIFFTAG_PAGENUMBER, 0, 1);
        TIFFSetField(tiff_file, TIFFTAG_CLEANFAXDATA, CLEANFAXDATA_CLEAN);
        TIFFSetField(tiff_file, TIFFTAG_IMAGEWIDTH, sequence[i].width);

        /* Write the image first.... */
        for (row = 0;  row < sequence[i].length;  row++)
        {
            memset(image_buffer, 0, sequence[i].width/8 + 1);
            if (TIFFWriteScanline(tiff_file, image_buffer, row, 0) < 0)
            {
                printf("Write error at row %d.\n", row);
                exit(2);
            }
        }
        /* ....then the directory entry, and libtiff is happy. */
        TIFFWriteDirectory(tiff_file);
        TIFFClose(tiff_file);
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
