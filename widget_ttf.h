/* truetype widget handling
 *
 * Copyright (C) 2016 TangoCash <eric@loxat.de>
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


#ifndef _WIDGET_TTF_H_
#define _WIDGET_TTF_H_

#include "property.h"
#include "widget.h"
#include "rgb.h"

typedef struct WIDGET_TTF {
    void *gdImage;              /* raw gd image */
    RGBA *bitmap;               /* image bitmap */
    int width, height;          /* size of the image */
    int oldheight;              /* height of the image before */
    PROPERTY value;             /* text to render */
    PROPERTY size;              /* fontsize */
    PROPERTY font;              /* path to font */
    PROPERTY fcolor;            /* font color */
    PROPERTY update;            /* update interval */
    PROPERTY reload;            /* reload image on update? */
    PROPERTY visible;           /* image visible? */
    PROPERTY inverted;          /* image inverted? */
    PROPERTY center;            /* image centered? */
    PROPERTY _width;            /* scale font to witdh */
    PROPERTY _height;           /* scale font to height */
    PROPERTY align;             /* align font to L/C/R */
    PROPERTY debugborder;       /* outer line color */
} WIDGET_TTF;

extern WIDGET_CLASS Widget_Truetype;

#endif
