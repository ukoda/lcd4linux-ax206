/* $Id$
 * $URL$
 *
 * graphic bar widget handling
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 * Copyright (C) 2021 David Annett <david@annett.co.nz>
 *
 * This file is part of LCD4Linux.
 *
 * LCD4Linux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * LCD4Linux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#ifndef _WIDGET_GRAPHICBAR_H_
#define _WIDGET_GRAPHICBAR_H_

#include "property.h"
#include "widget.h"
#include "widget_bar.h"
#include "rgb.h"

typedef struct WIDGET_GRAPHICBAR {
    /* gdImage to visible must be in this order to match WIDGET_IMAGE */
    void *gdImage;              /* raw gd image */
    RGBA *bitmap;               /* image bitmap */
    int ysize, xsize;           /* bar size on horizontal Y axis, vertical X axis */
    int oldheight;              /* height of the image before */
    PROPERTY expr_min;          /* explicit minimum value */
    PROPERTY expr_max;          /* explicit maximum value */
    PROPERTY width;             /* bar width */
    PROPERTY length;            /* bar length */
    PROPERTY update;            /* update interval (msec) */
    PROPERTY reload;            /* reload image on update? */
    PROPERTY visible;           /* image visible? */
    PROPERTY inverted;          /* image inverted? */
    PROPERTY center;            /* image centered? Not used, just for padding*/
    /* From here onward can be custom to this widget */
    PROPERTY value;             /* value (length) of bar */
    PROPERTY color;             /* bar color */
    PROPERTY valuelow;          /* value below which to use colorlow for the bar */
    PROPERTY colorlow;          /* bar color for low value */
    PROPERTY valuehigh;         /* value above which to use colorhigh for the bar */
    PROPERTY colorhigh;         /* bar color for high value */
    PROPERTY background;        /* background color */
    DIRECTION direction;        /* bar direction */
    STYLE style;                /* bar style (hollow) */
    double min;                 /* minimum value, used for auto scaling */
    double max;                 /* maximum value, used for auto scaling */
} WIDGET_GRAPHICBAR;


extern WIDGET_CLASS Widget_GraphicBar;

#endif
