/* truetype widget handling
 *
 * Copyright (C) 2016 TangoCash <eric@loxat.de>
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
 * WIDGET_CLASS Widget_ttf
 *   the truetype widget
 *
 */

/* brect[8]
* 0	lower left corner, X position
* 1	lower left corner, Y position
* 2	lower right corner, X position
* 3	lower right corner, Y position
* 4	upper right corner, X position
* 5	upper right corner, Y position
* 6	upper left corner, X position
* 7	upper left corner, Y position
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
#include "widget_ttf.h"
#include "rgb.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static void widget_ttf_render(const char *Name, WIDGET_TTF * Image)
{
    int x, y;
    int inverted;
    gdImagePtr gdImage;

    int color;
    int debugborder;
    int trans;
    int brect[8];
    int mrect[8];
    int _width, _height;
    char *e;
    unsigned long l;
    unsigned char r, g, b, a;
    char *fcolor;
    char *text;
    char *dborder;
    double size;
    char *font, *err, *align;
    char *background;
    int backgroundcl;

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

        /* free previous image */
        if (Image->gdImage) {
            gdImageDestroy(Image->gdImage);
            Image->gdImage = NULL;
        }
        //replace with expression and use as text
        text = P2S(&Image->value);
        font = P2S(&Image->font);
        size = P2N(&Image->size);
        _width = P2N(&Image->_width);
        _height = P2N(&Image->_height);
        align = P2S(&Image->align);

        if (((_width > 0) && (_height > 0)) && (size == 0)) {
            size = _height;
            do {
                size--;
                err = gdImageStringFT(NULL, &mrect[0], 0, font, size, 0., 0, 0, "[Äp}§|");
                err = gdImageStringFT(NULL, &brect[0], 0, font, size, 0., 0, 0, text);
            }
            while ((brect[2] - brect[6] > _width) || (mrect[3] - mrect[7] > _height));
            x = _width;
            y = _height;
        } else {
            err = gdImageStringFT(NULL, &mrect[0], 0, font, size, 0., 0, 0, "[Äp}§|");
            err = gdImageStringFT(NULL, &brect[0], 0, font, size, 0., 0, 0, text);

            if ((_width > 0) && (_height > 0)) {
                x = _width;
                y = _height;
            } else {
                x = brect[2] - brect[6];
                y = mrect[3] - mrect[7];
            }
        }

        Image->gdImage = gdImageCreateTrueColor(x, y);
        gdImageSaveAlpha(Image->gdImage, 1);

        if (Image->gdImage == NULL) {
            error("Warning: Image %s: Create failed!", Name);
            return;
        }
        trans = gdImageColorAllocateAlpha(Image->gdImage, 0, 0, 0, 127);
        gdImageFill(Image->gdImage, 0, 0, trans);

        fcolor = P2S(&Image->fcolor);

        if (strlen(fcolor) == 8) {
            l = strtoul(fcolor, &e, 16);
            r = (l >> 24) & 0xff;
            g = (l >> 16) & 0xff;
            b = (l >> 8) & 0xff;
            a = (l & 0xff) / 2;

            color = gdImageColorAllocateAlpha(Image->gdImage, r, g, b, a);
        } else {
            l = strtoul(fcolor, &e, 16);
            r = (l >> 16) & 0xff;
            g = (l >> 8) & 0xff;
            b = l & 0xff;

            color = gdImageColorAllocate(Image->gdImage, r, g, b);
        }

        if ((_width > 0) && (_height > 0))
            switch (toupper(align[0])) {
            case 'R':
                x = _width - brect[2] - brect[6];
                break;
            case 'L':
                x = brect[6];
                break;
            case 'C':
            case 'M':
                x = (_width - brect[2] - brect[6]) / 2 - brect[6];
                break;
            default:
                x = (_width - brect[2] - brect[6]) / 2 - brect[6];
                break;
        } else
            x = brect[6];

        y = (_height - mrect[1] - mrect[7]) / 2;

        if (property_valid(&Image->background)) {
            background = P2S(&Image->background);
            if (strlen(background) == 8) {
                l = strtoul(background, &e, 16);
                r = (l >> 24) & 0xff;
                g = (l >> 16) & 0xff;
                b = (l >> 8) & 0xff;
                a = (l & 0xff) / 2;

                backgroundcl = gdImageColorAllocateAlpha(Image->gdImage, r, g, b, a);
            } else {
                l = strtoul(background, &e, 16);
                r = (l >> 16) & 0xff;
                g = (l >> 8) & 0xff;
                b = l & 0xff;

                backgroundcl = gdImageColorAllocate(Image->gdImage, r, g, b);
            }

            if (backgroundcl)
                gdImageFilledRectangle(Image->gdImage, 1, 1, _width - 1, _height - 1, backgroundcl);
        }

        dborder = P2S(&Image->debugborder);

        if (strlen(dborder) == 8) {
            l = strtoul(dborder, &e, 16);
            r = (l >> 24) & 0xff;
            g = (l >> 16) & 0xff;
            b = (l >> 8) & 0xff;
            a = (l & 0xff) / 2;

            debugborder = gdImageColorAllocateAlpha(Image->gdImage, r, g, b, a);
        } else {
            l = strtoul(dborder, &e, 16);
            r = (l >> 16) & 0xff;
            g = (l >> 8) & 0xff;
            b = l & 0xff;

            debugborder = gdImageColorAllocate(Image->gdImage, r, g, b);
        }

        if (debugborder)
            gdImageRectangle(Image->gdImage, 1, 1, _width - 1, _height - 1, debugborder);

        err = gdImageStringFT(Image->gdImage, &brect[0], color, font, size, 0.0, x, y, text);

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


static void widget_ttf_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_TTF *Image = W->data;

    /* process the parent only */
    if (W->parent == NULL) {

        /* evaluate properties */
        property_eval(&Image->value);
        property_eval(&Image->size);
        property_eval(&Image->font);
        property_eval(&Image->fcolor);
        property_eval(&Image->update);
        property_eval(&Image->reload);
        property_eval(&Image->visible);
        property_eval(&Image->inverted);
        property_eval(&Image->center);
        property_eval(&Image->_width);
        property_eval(&Image->_height);
        property_eval(&Image->align);
        property_eval(&Image->debugborder);
        property_eval(&Image->background);

        /* render image into bitmap */
        widget_ttf_render(W->name, Image);

    }

    /* finally, draw it! */
    if (W->class->draw)
        W->class->draw(W);

    /* add a new one-shot timer */
    if (P2N(&Image->update) > 0) {
        timer_add_widget(widget_ttf_update, Self, P2N(&Image->update), 1);
    }
}



int widget_ttf_init(WIDGET * Self)
{
    char *section;
    WIDGET_TTF *Image;

    /* re-use the parent if one exists */
    if (Self->parent == NULL) {

        /* prepare config section */
        /* strlen("Widget:")=7 */
        section = malloc(strlen(Self->name) + 8);
        strcpy(section, "Widget:");
        strcat(section, Self->name);

        Image = malloc(sizeof(WIDGET_TTF));
        memset(Image, 0, sizeof(WIDGET_TTF));

        /* initial size */
        Image->width = 0;
        Image->height = 0;
        Image->bitmap = NULL;

        /* load properties */
        property_load(section, "expression", "Samsung", &Image->value);
        property_load(section, "size", "0", &Image->size);
        property_load(section, "font", NULL, &Image->font);
        property_load(section, "fcolor", "ff0000", &Image->fcolor);
        property_load(section, "update", "100", &Image->update);
        property_load(section, "reload", "0", &Image->reload);
        property_load(section, "visible", "1", &Image->visible);
        property_load(section, "inverted", "0", &Image->inverted);
        property_load(section, "center", "0", &Image->center);
        property_load(section, "width", "0", &Image->_width);
        property_load(section, "height", "0", &Image->_height);
        property_load(section, "align", "C", &Image->align);
        property_load(section, "debugborder", "000000", &Image->debugborder);
        property_load(section, "background", NULL, &Image->background);

        /* sanity checks */
        if (!property_valid(&Image->font)) {
            error("Warning: widget %s has no font", section);
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
    widget_ttf_update(Self);

    return 0;
}


int widget_ttf_quit(WIDGET * Self)
{
    if (Self) {
        /* do not deallocate child widget! */
        if (Self->parent == NULL) {
            if (Self->data) {
                WIDGET_TTF *Image = Self->data;
                if (Image->gdImage) {
                    gdImageDestroy(Image->gdImage);
                    Image->gdImage = NULL;
                }
                free(Image->bitmap);
                property_free(&Image->value);
                property_free(&Image->size);
                property_free(&Image->font);
                property_free(&Image->fcolor);
                property_free(&Image->update);
                property_free(&Image->reload);
                property_free(&Image->visible);
                property_free(&Image->inverted);
                property_free(&Image->center);
                property_eval(&Image->_width);
                property_eval(&Image->_height);
                property_eval(&Image->align);
                property_free(&Image->debugborder);
                property_free(&Image->background);

                free(Self->data);
                Self->data = NULL;
            }
        }
    }

    return 0;

}



WIDGET_CLASS Widget_Truetype = {
    .name = "Truetype",
    .type = WIDGET_TYPE_XY,
    .init = widget_ttf_init,
    .draw = NULL,
    .quit = widget_ttf_quit,
};
