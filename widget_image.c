/* $Id$
 * $URL$
 *
 * image widget handling
 *
 * Copyright (C) 2006 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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
 */

/*
 * exported functions:
 *
 * WIDGET_CLASS Widget_Image
 *   the image widget
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_GD_GD_H
#include <gd/gd.h>
#else
#ifdef HAVE_GD_H
#include <gd.h>
#else
#error "gd.h not found!"
#error "cannot compile image widget"
#endif
#endif

#if GD2_VERS != 2
#error "lcd4linux requires libgd version 2"
#error "cannot compile image widget"
#endif

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "property.h"
#include "timer_group.h"
#include "widget.h"
#include "widget_image.h"
#include "rgb.h"
#include "drv_generic.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static void widget_image_render(const char *Name, WIDGET_IMAGE * Image)
{
    int x, y;
    int inverted;
    gdImagePtr gdImage;
    int scale, _width, _height, center;

    /* clear bitmap */
    if (Image->bitmap) {
        Image->oldheight = Image->height;
        int i;
        for (i = 0; i < Image->height * Image->width; i++) {
            RGBA empty = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0x00 };
            Image->bitmap[i] = empty;
        }
    }

    /* reload image only on first call or on explicit reload request */
    if (Image->gdImage == NULL || P2N(&Image->reload)) {

        char *file;
        FILE *fd;

        /* free previous image */
        if (Image->gdImage) {
            gdImageDestroy(Image->gdImage);
            Image->gdImage = NULL;
        }

        file = P2S(&Image->file);
        if (file == NULL || file[0] == '\0') {
            error("Warning: Image %s has no file", Name);
            return;
        }

        fd = fopen(file, "rb");
        if (fd == NULL) {
            error("Warning: Image %s: fopen(%s) failed: %s", Name, file, strerror(errno));
            return;
        }
        Image->gdImage = gdImageCreateFromPng(fd);
        fclose(fd);

        if (Image->gdImage == NULL) {
            fd = fopen(file, "rb");
            if (fd == NULL) {
                error("Warning: Image %s: fopen(%s) failed: %s", Name, file, strerror(errno));
                return;
            }
            Image->gdImage = gdImageCreateFromJpeg(fd);
            fclose(fd);
        }

        if (Image->gdImage == NULL) {
            fd = fopen(file, "rb");
            if (fd == NULL) {
                error("Warning: Image %s: fopen(%s) failed: %s", Name, file, strerror(errno));
                return;
            }
            Image->gdImage = gdImageCreateFromGif(fd);
            fclose(fd);
        }

        if (Image->gdImage == NULL) {
            fd = fopen(file, "rb");
            if (fd == NULL) {
                error("Warning: Image %s: fopen(%s) failed: %s", Name, file, strerror(errno));
                return;
            }
            Image->gdImage = gdImageCreateFromBmp(fd);
            fclose(fd);
        }

        if (Image->gdImage == NULL) {
            error("Warning: Image %s: CreateFromPng/Jpeg/Gif/Bmp (%s) failed!", Name, file);
            return;
        }

    }

    _width = P2N(&Image->_width);
    _height = P2N(&Image->_height);
    scale = P2N(&Image->scale);
    center = P2N(&Image->center);

    if (((_width > 0) || (_height > 0)) && (scale == 100)) {
        gdImage = Image->gdImage;
        gdImagePtr scaled_image;
        int ox = gdImageSX(gdImage);
        int oy = gdImageSY(gdImage);
        int nx = ox;
        int ny = oy;
        float w_fac, h_fac;

        w_fac = ((float) _width / (float) ox);
        h_fac = ((float) _height / (float) oy);

        if (w_fac == 0)
            w_fac = h_fac + 1;
        if (h_fac == 0)
            h_fac = w_fac + 1;

        if (w_fac > h_fac) {
            nx = h_fac * ox;
            ny = _height;
        } else {
            nx = _width;
            ny = w_fac * oy;
        }

        scaled_image = gdImageCreateTrueColor(nx, ny);
        gdImageSaveAlpha(scaled_image, 1);
        gdImageFill(scaled_image, 0, 0, gdImageColorAllocateAlpha(scaled_image, 0, 0, 0, 127));
        gdImageCopyResized(scaled_image, Image->gdImage, 0, 0, 0, 0, nx, ny, ox, oy);
        gdImageDestroy(Image->gdImage);
        Image->gdImage = scaled_image;
    }

    /* Scale if needed */
    if ((scale != 100) && scale > 1) {
        gdImage = Image->gdImage;
        gdImagePtr scaled_image;
        int ox = gdImageSX(gdImage);
        int oy = gdImageSY(gdImage);
        int nx = ox * scale / 100;
        int ny = oy * scale / 100;
        scaled_image = gdImageCreateTrueColor(nx, ny);
        gdImageSaveAlpha(scaled_image, 1);
        gdImageFill(scaled_image, 0, 0, gdImageColorAllocateAlpha(scaled_image, 0, 0, 0, 127));
        gdImageCopyResized(scaled_image, Image->gdImage, 0, 0, 0, 0, nx, ny, ox, oy);
        gdImageDestroy(Image->gdImage);
        Image->gdImage = scaled_image;
    }

    if (center) {
        gdImage = Image->gdImage;
        gdImagePtr center_image;
        int ox = gdImageSX(gdImage);
        int oy = gdImageSY(gdImage);
        int cx = (DCOLS / 2) - (ox / 2);
        int cy = (oy < Image->oldheight) ? Image->oldheight : oy;
        center_image = gdImageCreateTrueColor(DCOLS, cy);
        if (cy > oy)
            cy = (cy / 2) - (oy / 2);
        else
            cy = 0;
        gdImageSaveAlpha(center_image, 1);
        gdImageFill(center_image, 0, 0, gdImageColorAllocateAlpha(center_image, 0, 0, 0, 127));
        gdImageCopyResized(center_image, Image->gdImage, cx, cy, 0, 0, ox, oy, ox, oy);
        gdImageDestroy(Image->gdImage);
        Image->gdImage = center_image;
    }


    /* maybe resize bitmap */
    gdImage = Image->gdImage;
    if (gdImage->sx > Image->width || P2N(&Image->center)) {
        Image->width = gdImage->sx;
        free(Image->bitmap);
        Image->bitmap = NULL;
    }
    if (gdImage->sy > Image->height || P2N(&Image->center)) {
        Image->height = gdImage->sy;
        free(Image->bitmap);
        Image->bitmap = NULL;
    }
    if (Image->bitmap == NULL && Image->width > 0 && Image->height > 0) {
        int i = Image->width * Image->height * sizeof(Image->bitmap[0]);
        Image->bitmap = malloc(i);
        if (Image->bitmap == NULL) {
            error("Warning: Image %s: malloc(%d) failed: %s", Name, i, strerror(errno));
            return;
        }
        for (i = 0; i < Image->height * Image->width; i++) {
            RGBA empty = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0x00 };
            Image->bitmap[i] = empty;
        }
    }


    /* finally really render it */
    inverted = P2N(&Image->inverted);
    if (P2N(&Image->visible)) {
        for (x = 0; x < gdImage->sx; x++) {
            for (y = 0; y < gdImage->sy; y++) {
                int p = gdImageGetTrueColorPixel(gdImage, x, y);
                int a = gdTrueColorGetAlpha(p);
                int i = y * Image->width + x;
                Image->bitmap[i].R = gdTrueColorGetRed(p);
                Image->bitmap[i].G = gdTrueColorGetGreen(p);
                Image->bitmap[i].B = gdTrueColorGetBlue(p);
                /* GD's alpha is 0 (opaque) to 127 (tranparanet) */
                /* our alpha is 0 (transparent) to 255 (opaque) */
                Image->bitmap[i].A = (a == 127) ? 0 : 255 - 2 * a;
                if (inverted) {
                    Image->bitmap[i].R = 255 - Image->bitmap[i].R;
                    Image->bitmap[i].G = 255 - Image->bitmap[i].G;
                    Image->bitmap[i].B = 255 - Image->bitmap[i].B;
                }
            }
        }
    }
}


static void widget_image_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_IMAGE *Image = W->data;

    /* process the parent only */
    if (W->parent == NULL) {

        /* evaluate properties */
        property_eval(&Image->file);
        property_eval(&Image->scale);
        property_eval(&Image->_width);
        property_eval(&Image->_height);
        property_eval(&Image->update);
        property_eval(&Image->reload);
        property_eval(&Image->visible);
        property_eval(&Image->inverted);
        property_eval(&Image->center);

        /* render image into bitmap */
        widget_image_render(W->name, Image);

    }

    /* finally, draw it! */
    if (W->class->draw)
        W->class->draw(W);

    /* add a new one-shot timer */
    if (P2N(&Image->update) > 0) {
        timer_add_widget(widget_image_update, Self, P2N(&Image->update), 1);
    }
}



int widget_image_init(WIDGET * Self)
{
    char *section;
    WIDGET_IMAGE *Image;

    /* re-use the parent if one exists */
    if (Self->parent == NULL) {

        /* prepare config section */
        /* strlen("Widget:")=7 */
        section = malloc(strlen(Self->name) + 8);
        strcpy(section, "Widget:");
        strcat(section, Self->name);

        Image = malloc(sizeof(WIDGET_IMAGE));
        memset(Image, 0, sizeof(WIDGET_IMAGE));

        /* initial size */
        Image->width = 0;
        Image->height = 0;
        Image->bitmap = NULL;

        /* load properties */
        property_load(section, "file", NULL, &Image->file);
        property_load(section, "scale", "100", &Image->scale);
        property_load(section, "width", "0", &Image->_width);
        property_load(section, "height", "0", &Image->_height);
        property_load(section, "update", "100", &Image->update);
        property_load(section, "reload", "0", &Image->reload);
        property_load(section, "visible", "1", &Image->visible);
        property_load(section, "inverted", "0", &Image->inverted);
        property_load(section, "center", "0", &Image->center);

        /* sanity checks */
        if (!property_valid(&Image->file)) {
            error("Warning: widget %s has no file", section);
        }

        free(section);
        Self->data = Image;
        Self->x2 = Self->col + Image->width;
        Self->y2 = Self->row + Image->height;

    } else {

        /* re-use the parent */
        Self->data = Self->parent->data;

    }

    /* just do it! */
    widget_image_update(Self);

    return 0;
}


int widget_image_quit(WIDGET * Self)
{
    if (Self) {
        /* do not deallocate child widget! */
        if (Self->parent == NULL) {
            if (Self->data) {
                WIDGET_IMAGE *Image = Self->data;
                if (Image->gdImage) {
                    gdImageDestroy(Image->gdImage);
                    Image->gdImage = NULL;
                }
                free(Image->bitmap);
                property_free(&Image->file);
                property_free(&Image->scale);
                property_free(&Image->_width);
                property_free(&Image->_height);
                property_free(&Image->update);
                property_free(&Image->reload);
                property_free(&Image->visible);
                property_free(&Image->inverted);
                property_free(&Image->center);
                free(Self->data);
                Self->data = NULL;
            }
        }
    }

    return 0;

}



WIDGET_CLASS Widget_Image = {
    .name = "image",
    .type = WIDGET_TYPE_XY,
    .init = widget_image_init,
    .draw = NULL,
    .quit = widget_image_quit,
};
