/* $Id$
 * $URL$
 *
 * graphic bar widget handling
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 * Copyright (C) 2021 David Annett <david@annett.co.nz>
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
 * WIDGET_CLASS Widget_GraphicBar
 *   the graphic bar widget
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
#include "property.h"
#include "rgb.h"
#include "drv_generic.h"
#include "drv_generic_graphic.h"
#include "timer_group.h"
#include "layout.h"
#include "widget.h"
#include "widget_graphicbar.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static int getcolorint(PROPERTY *colorprop, void *Image)
{
    char          *colorstr;
    int            colorint;
    char          *e;
    unsigned long  l;
    unsigned char  r, g, b, a;

    colorstr = P2S(colorprop);

    if (strlen(colorstr) == 8) {
        l = strtoul(colorstr, &e, 16);
        r = (l >> 24) & 0xff;
        g = (l >> 16) & 0xff;
        b = (l >> 8) & 0xff;
        a = (l & 0xff) / 2;

        colorint = gdImageColorAllocateAlpha(Image, r, g, b, a);
    } else {
        l = strtoul(colorstr, &e, 16);
        r = (l >> 16) & 0xff;
        g = (l >> 8) & 0xff;
        b = l & 0xff;

        colorint = gdImageColorAllocate(Image, r, g, b);
    }

    return colorint;
}

static void widget_graphicbar_render(const char *Name, WIDGET_GRAPHICBAR * Bar)
{
    int x, y;
    int inverted;
    gdImagePtr gdImage;
    double dval;
    double dlow, dhigh;
    double dmin, dmax;
    int barend, barstart;
    int xs, ys, xe, ye;
    int colorbackground;
    int colorbar;

    /* Work out the bar ends and value as doubles clipping as needed */

    dval = P2N(&Bar->value);
    /* minimum: if value is empty, do auto-scaling */
    if (property_valid(&Bar->expr_min)) {
        dmin = P2N(&Bar->expr_min);
    } else {
        dmin = Bar->min;
        if (dval < dmin) {
            dmin = dval;
        }
    }

    /* maximum: if value is empty, do auto-scaling */
    if (property_valid(&Bar->expr_max)) {
        dmax = P2N(&Bar->expr_max);
    } else {
        dmax = Bar->max;
        if (dval > dmax) {
            dmax = dval;
        }
    }

    /* Get the low color limit */
    if (property_valid(&Bar->valuelow) && property_valid(&Bar->valuelow)) {
        dlow = P2N(&Bar->valuelow);
    } else {
        dlow = dval;
    }

    /* Get the high color limit */
    if (property_valid(&Bar->valuehigh) && property_valid(&Bar->valuehigh)) {
        dhigh = P2N(&Bar->valuehigh);
    } else {
        dhigh = dval;
    }

    /* debugging */
    if (Bar->min != dmin || Bar->max != dmax) {
        debug("Bar '%s': new scale %G - %G", Name, dmin, dmax);
    }

    /* Save the limits for future autoscaling */

    Bar->min = dmin;
    Bar->max = dmax;

    if (dmin == dmax)   // Prevent division by zero case
        dmax += 1;

    /* Now scale as pixels */
    barend = P2N(&Bar->length) * ((dval - dmin) / (dmax - dmin));
    barstart = P2N(&Bar->length) * ((0 - dmin) / (dmax - dmin));

    if (barend < 0) {
        barend = 0;
    }
    if (barstart < 0) {
        barstart = 0;
    }
    if (barstart > barend) {
        int swap = barend;
        barend = barstart;
        barstart = swap;
    }

    /* barend is how many pixels the value is along the bar */

    switch (Bar->direction) {
    case DIR_NORTH:
        xs = Bar->xsize-barend;
        ys = 0;
        xe = Bar->xsize-barstart;
        ye = Bar->ysize-1;
        break;
        
    case DIR_SOUTH:
        xs = barstart;
        ys = 0;
        xe = barend;
        ye = Bar->ysize-1;
        break;

    case DIR_WEST:
        xs = 0;
        ys = Bar->ysize-barend;
        xe = Bar->xsize-1;
        ye = Bar->ysize-barstart;
        break;

    default:    /* Should be DIR_EAST */
        xs = 0;
        ys = barstart;
        xe = Bar->xsize-1;
        ye = barend;
        break;
    }
    if (xe >= Bar->xsize)
        xe = Bar->xsize-1;
    if (ye >= Bar->ysize)
        ye = Bar->ysize-1;
    if (xs >= Bar->xsize)
        xs = Bar->xsize-1;
    if (ys >= Bar->ysize)
        ys = Bar->ysize-1;

    /* Render the bar as a couple rectangles */

    /* clear bitmap */
    if (Bar->bitmap) {
        Bar->oldheight = Bar->xsize;
        int i;
        for (i = 0; i < Bar->ysize * Bar->xsize; i++) {
            RGBA empty = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0xff };
            Bar->bitmap[i] = empty;
        }
    }

    /* free previous image */
    if (Bar->gdImage) {
        gdImageDestroy(Bar->gdImage);
        Bar->gdImage = NULL;
    }

    Bar->gdImage = gdImageCreateTrueColor(Bar->xsize, Bar->ysize);
    gdImageSaveAlpha(Bar->gdImage, 1);

    if (Bar->gdImage == NULL) {
        error("Warning: Image %s: Create failed!", Name);
        return;
    }

    /* Create color numbers we will be using */
    colorbackground = getcolorint(&Bar->background, Bar->gdImage);
    if (dval < dlow) {
        colorbar = getcolorint(&Bar->colorlow, Bar->gdImage);
    } else if (dval > dhigh) {
        colorbar = getcolorint(&Bar->colorhigh, Bar->gdImage);
    } else {
        colorbar = getcolorint(&Bar->color, Bar->gdImage);
    }

    /* Draw the background */
    gdImageFill(Bar->gdImage, 0, 0, colorbackground);

    /* Draw the bar */
    if (Bar->style == STYLE_HOLLOW)
        gdImageRectangle(Bar->gdImage, xs, ys, xe, ye, colorbar);
    else
        gdImageFilledRectangle(Bar->gdImage, xs, ys, xe, ye, colorbar);

    /* Allocate memory for bitmap if needed */
    if (Bar->bitmap == NULL && Bar->ysize > 0 && Bar->xsize > 0) {
        int i = Bar->ysize * Bar->xsize * sizeof(Bar->bitmap[0]);
        Bar->bitmap = malloc(i);
        if (Bar->bitmap == NULL) {
            error("Warning: Image malloc failed");
            return;
        }
        for (i = 0; i < Bar->ysize * Bar->xsize; i++) {
            RGBA empty = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0x00 };
            Bar->bitmap[i] = empty;
        }
    }

    /* finally really render it */
    gdImage = Bar->gdImage;
    inverted = P2N(&Bar->inverted);
    if (P2N(&Bar->visible)) {
        for (x = 0; x < gdImage->sx; x++) {
            for (y = 0; y < gdImage->sy; y++) {
                int p = gdImageGetTrueColorPixel(gdImage, x, y);
                int a = gdTrueColorGetAlpha(p);
                int i = x * Bar->ysize + y;
                Bar->bitmap[i].R = gdTrueColorGetRed(p);
                Bar->bitmap[i].G = gdTrueColorGetGreen(p);
                Bar->bitmap[i].B = gdTrueColorGetBlue(p);
                /* GD's alpha is 0 (opaque) to 127 (tranparanet) */
                /* our alpha is 0 (transparent) to 255 (opaque) */
                Bar->bitmap[i].A = (a == 127) ? 0 : 255 - 2 * a;
                if (inverted) {
                    Bar->bitmap[i].R = 255 - Bar->bitmap[i].R;
                    Bar->bitmap[i].G = 255 - Bar->bitmap[i].G;
                    Bar->bitmap[i].B = 255 - Bar->bitmap[i].B;
                }
            }
        }
    }
}

static void widget_graphicbar_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_GRAPHICBAR *Bar = W->data;

    /* process the parent only */
    if (W->parent == NULL) {

        /* evaluate properties */
        property_eval(&Bar->value);
        property_eval(&Bar->expr_min);
        property_eval(&Bar->expr_max);
        // property_eval(&Bar->length); // If we do this we should also do xsize etc
        // property_eval(&Bar->width);
        property_eval(&Bar->color);
        property_eval(&Bar->valuelow);
        property_eval(&Bar->colorlow);
        property_eval(&Bar->valuehigh);
        property_eval(&Bar->colorhigh);
        property_eval(&Bar->background);
        property_eval(&Bar->update);
        property_eval(&Bar->reload);
        property_eval(&Bar->visible);

        /* render image into bitmap */
        widget_graphicbar_render(W->name, Bar);

    }

    /* finally, draw it! */
    if (W->class->draw)
        W->class->draw(W);

    /* add a new one-shot timer */
    if (P2N(&Bar->update) > 0) {
        timer_add_widget(widget_graphicbar_update, Self, P2N(&Bar->update), 1);
    }
}


int widget_graphicbar_init(WIDGET * Self)
{
    char *section;
    char *c;
    WIDGET_GRAPHICBAR *Bar;

    /* re-use the parent if one exists */
    if (Self->parent == NULL) {

        /* prepare config section */
        /* strlen("Widget:")=7 */
        section = malloc(strlen(Self->name) + 8);
        strcpy(section, "Widget:");
        strcat(section, Self->name);

        Bar = malloc(sizeof(WIDGET_GRAPHICBAR));
        memset(Bar, 0, sizeof(WIDGET_GRAPHICBAR));

        /* initial size */
        Bar->bitmap = NULL;
        Bar->min = 0;
        Bar->max = 0;

        /* load properties */
        property_load(section, "min",        NULL,     &Bar->expr_min);
        property_load(section, "max",        NULL,     &Bar->expr_max);
        property_load(section, "width",      NULL,     &Bar->width);
        property_load(section, "length",     NULL,     &Bar->length);
        property_load(section, "update",     "100",    &Bar->update);
        property_load(section, "reload",     "0",      &Bar->reload);
        property_load(section, "visible",    "1",      &Bar->visible);
        property_load(section, "inverted",   "0",      &Bar->inverted);
        property_load(section, "center",     "0",      &Bar->center);
        property_load(section, "expression", NULL,     &Bar->value);
        property_load(section, "color",      "00ff00", &Bar->color);
        property_load(section, "valuelow",   NULL,     &Bar->valuelow);
        property_load(section, "colorlow",   NULL,     &Bar->colorlow);
        property_load(section, "valuehigh",  NULL,     &Bar->valuehigh);
        property_load(section, "colorhigh",  NULL,     &Bar->colorhigh);
        property_load(section, "background", "404040", &Bar->background);

        /* sanity checks */
        if (!property_valid(&Bar->value)) {
            error("Warning: widget %s has no value", section);
        }
        if (!property_valid(&Bar->length)) {
            error("Warning: widget %s has no length", section);
            return 1;
        }
        if (!property_valid(&Bar->width)) {
            error("Warning: widget %s has no width", section);
            return 1;
        }

        property_eval(&Bar->length);
        property_eval(&Bar->width);
        /* direction: East (default), West, North, South */
        c = cfg_get(section, "direction", "E");
        switch (toupper(*c)) {
        case 'E':
            Bar->direction = DIR_EAST;
            Bar->xsize = P2N(&Bar->width);
            Bar->ysize = P2N(&Bar->length);
            break;
        case 'W':
            Bar->direction = DIR_WEST;
            Bar->xsize = P2N(&Bar->width);
            Bar->ysize = P2N(&Bar->length);
            break;
        case 'N':
            Bar->direction = DIR_NORTH;
            Bar->xsize = P2N(&Bar->length);
            Bar->ysize = P2N(&Bar->width);
            break;
        case 'S':
            Bar->direction = DIR_SOUTH;
            Bar->xsize = P2N(&Bar->length);
            Bar->ysize = P2N(&Bar->width);
            break;
        default:
            error("widget %s has unknown direction '%s'; known directions: 'E', 'W', 'N', 'S'; using 'E(ast)'", Self->name,
                  c);
            Bar->direction = DIR_EAST;
            Bar->xsize = P2N(&Bar->width);
            Bar->ysize = P2N(&Bar->length);
        }
        free(c);

        /* style: none (default), hollow */
        c = cfg_get(section, "style", "0");
        switch (toupper(*c)) {
        case 'H':
            Bar->style = STYLE_HOLLOW;
            break;
        case '0':
            Bar->style = 0;
            break;
        default:
            error("widget %s has unknown style '%s'; known styles: '0' or 'H'; using '0'", Self->name, c);
            Bar->style = 0;
        }
        free(c);

        free(section);
        Self->data = Bar;
        Self->x2 = Self->col + Bar->xsize - 1;
        Self->y2 = Self->row + Bar->ysize - 1;

    } else {

        /* re-use the parent */
        Self->data = Self->parent->data;

    }

    /* just do it! */
    widget_graphicbar_update(Self);

    return 0;
}


int widget_graphicbar_quit(WIDGET * Self)
{
    if (Self) {
        /* do not deallocate child widget! */
        if (Self->parent == NULL) {
            if (Self->data) {
                WIDGET_GRAPHICBAR *Bar = Self->data;
                if (Bar->gdImage) {
                    gdImageDestroy(Bar->gdImage);
                    Bar->gdImage = NULL;
                }
                free(Bar->bitmap);
                property_free(&Bar->expr_min);
                property_free(&Bar->expr_max);
                property_free(&Bar->width);
                property_free(&Bar->length);
                property_free(&Bar->update);
                property_free(&Bar->reload);
                property_free(&Bar->visible);
                property_free(&Bar->inverted);
                property_free(&Bar->center);
                property_free(&Bar->value);
                property_free(&Bar->color);
                property_free(&Bar->valuelow);
                property_free(&Bar->colorlow);
                property_free(&Bar->valuehigh);
                property_free(&Bar->colorhigh);
                property_free(&Bar->background);

                free(Self->data);
                Self->data = NULL;
            }
        }
    }

    return 0;

}



WIDGET_CLASS Widget_GraphicBar = {
    .name = "graphicbar",
    .type = WIDGET_TYPE_XY,
    .init = widget_graphicbar_init,
    .draw = NULL,
    .quit = widget_graphicbar_quit,
};
