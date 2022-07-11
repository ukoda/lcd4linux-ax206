/*
 * Copyright (C) 2021 redblue
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

/*
 * Driver use fbtft and fb_ili9486 linux kernel modules
 * on orange pi/rapsberry pi hardware
 */

/*
 * Hardware for testing: orange pi pc2 with TFT 3.5 Inch LCD Touch Screen SPI RGB Display
 */

/*
 * TODO: tochscreen
 */

/*
 *
 * exported fuctions:
 *
 * struct DRIVER drv_ili9486_fb
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>
#include <syslog.h>
#include <byteswap.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"

#include "drv_generic_graphic.h"

typedef enum { false = 0, true = !false } bool;

static char Name[] = "ili9486_fb";

/* Display data */
static int fd = -1, bpp = 0, stride_bpp_value = 0, xres = 0, yres = 0, stride = 0, backlight = 0;
static unsigned char *newLCD = NULL, *oldLCD = NULL;

#define WIDTH_MAX 480
#define HEIGHT_MAX 320
#define BPP_MAX 16

#define SET_BYTE(INPUT, VALUE, POSITION) (INPUT=(VALUE<<(POSITION<<3))|(INPUT&(0xFFFFFFFF^(0xFF<<(POSITION<<3)))))

static int ili9486_fb_open(const char *dev, int bpp_value, int xres_value, int yres_value)
{
    bpp = bpp_value;
    xres = xres_value;
    yres = yres_value;

    switch (bpp) {
    case 8:
        stride_bpp_value = 2;
        break;
    case 15:
    case 16:
        stride_bpp_value = 4;
        break;
    case 24:
    case 32:
        stride_bpp_value = 6;
        break;
    default:
        stride_bpp_value = (bpp + 7) / 8;
    }

    stride = xres * stride_bpp_value;

    fd = open(dev, O_RDWR);
    if (fd == -1) {
        error("cannot open lcd device\n");
        return -1;
    }

    return 0;
}

static int ili9486_fb_close()
{
    if (newLCD) {
        free(newLCD);
        newLCD = 0;
    }
    if (oldLCD) {
        free(oldLCD);
        oldLCD = 0;
    }
    if (-1 != fd) {
        close(fd);
        fd = -1;
    }

    return 0;
}

static int drv_ili9486_fb_open(const char *section)
{
    char *dev;
    char *size;
    char *bpp_dev;
    int bpp_value;
    int xres_value;
    int yres_value;

    dev = cfg_get(section, "Port", NULL);
    if (dev == NULL || *dev == '\0') {
        error("%s: no '%s.Port' entry from %s", Name, section, cfg_source());
        return -1;
    }

    size = cfg_get(section, "Size", NULL);
    if (size == NULL || *size == '\0') {
        error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
        return -1;
    }

    if (sscanf(size, "%dx%d", &xres_value, &yres_value) != 2 || xres_value < 1 || yres_value < 1
        || xres_value > WIDTH_MAX || yres_value > HEIGHT_MAX) {
        error("%s: bad %s.Size '%s' from %s", Name, section, size, cfg_source());
        free(size);
        return -1;
    }

    bpp_dev = cfg_get(section, "Bpp", NULL);
    if (bpp_dev == NULL || *bpp_dev == '\0') {
        error("%s: no '%s.Bpp' entry from %s", Name, section, cfg_source());
        return -1;
    }

    if (sscanf(bpp_dev, "%d", &bpp_value) != 1 || bpp_value < 1 || bpp_value > BPP_MAX) {
        error("%s: bad %s.Bpp '%s' from %s", Name, section, bpp_dev, cfg_source());
        free(bpp_dev);
        return -1;
    }

    int h = ili9486_fb_open(dev, bpp_value, xres_value, yres_value);
    if (h == -1) {
        error("%s: cannot open ili9486 device %s", Name, dev);
        return -1;
    }

    return 0;
}

static int drv_ili9486_fb_close(void)
{
    ili9486_fb_close();

    return 0;
}

static void drv_ili9486_fb_set_pixel(int x, int y, RGBA pix)
{
    long int location;

    unsigned char red = pix.R;
    unsigned char green = pix.G;
    unsigned char blue = pix.B;

    uint32_t data = NULL;
    SET_BYTE(data, blue, 0);
    SET_BYTE(data, green, 1);
    SET_BYTE(data, red, 2);

    uint32_t colraw = ((data & 0x00FF0000) >> (16 + 8 - 5) << 11) |     // red
        ((data & 0x0000FF00) >> (8 + 8 - 6) << 5) |     // green
        ((data & 0x000000FF) >> (0 + 8 - 5) << 0);      // blue;

    unsigned char col1 = colraw & 0x0000FF;
    unsigned char col2 = (colraw & 0x00FF00) >> 8;

    location = (x * xres + y) * stride_bpp_value / 2;

    *(newLCD + location + 0) = col1;
    *(newLCD + location + 1) = col2;
}

static void drv_ili9486_fb_blit(const int row, const int col, const int height, const int width)
{
    bool refreshAll = false;
    int r, c;

    for (r = row; r < row + height; r++) {
        for (c = col; c < col + width; c++) {
            drv_ili9486_fb_set_pixel(r, c, drv_generic_graphic_rgb(r, c));
        }
    }
    for (r = row; r < row + height; r++) {
        for (c = col; c < col + width; c++) {
            if (newLCD != oldLCD) {
                refreshAll = true;
                break;
            }
        }
    }
    if (refreshAll) {
        for (r = row; r < row + height; r++) {
            for (c = col; c < col + width; c++) {
                memcpy(oldLCD, newLCD, sizeof(newLCD) + 1);
            }
        }
        write(fd, newLCD + stride, stride * yres);
    }
}

static int drv_ili9486_fb_backlight(int number)
{
    return 0;
}

/* start graphic display */
static int drv_ili9486_fb_start(const char *section)
{
    int i;
    char *s;

    s = cfg_get(section, "Font", "6x8");
    if (s == NULL || *s == '\0') {
        error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
        return -1;
    }

    XRES = -1;
    YRES = -1;
    if (sscanf(s, "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
        error("%s: bad Font '%s' from %s", Name, s, cfg_source());
        return -1;
    }

    if (XRES < 6 || YRES < 8) {
        error("%s: bad Font '%s' from %s (must be at least 6x8)", Name, s, cfg_source());
        return -1;
    }
    free(s);

    // Get the backlight value (0 = off, 10 = max brightness)
    if (cfg_number(section, "Backlight", 0, 0, 10, &i) > 0)
        backlight = i;
    else
        backlight = 10;

    /* open communication with the display */
    if (drv_ili9486_fb_open(section) < 0) {
        return -1;
    }

    /* you surely want to allocate a framebuffer or something... */
    newLCD = (unsigned char *) malloc(yres * stride);
    if (newLCD)
        memset(newLCD, 0, yres * stride);

    if (newLCD == NULL) {
        error("%s: newLCD buffer could not be allocated: malloc() failed", Name);
        return -1;
    }
    oldLCD = (unsigned char *) malloc(yres * stride);
    if (oldLCD)
        memset(oldLCD, 0, yres * stride);

    if (oldLCD == NULL) {
        error("%s: oldLCD buffer could not be allocated: malloc() failed", Name);
        return -1;
    }

    drv_ili9486_fb_backlight(backlight);

    /* set width/height from ili9486 firmware specs */
    DROWS = yres;
    DCOLS = xres;

    info("%s: init succesfully, xres %d, yres %d, bpp %d, stride %d", Name, xres, yres, bpp, stride);

    return 0;
}

/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    int bl_on;
    bl_on = (R2N(arg1) == 0 ? 0 : 1);
    drv_ili9486_fb_backlight(bl_on);
    SetResult(&result, R_NUMBER, &bl_on);
}


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */
/* using drv_generic_gpio_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_ili9486_fb_list(void)
{
    info("ili9486 OLED driver");

    return 0;
}

/* initialize driver & display */
int drv_ili9486_fb_init(const char *section, const int quiet)
{
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_ili9486_fb_blit;

    /* start display */
    if ((ret = drv_ili9486_fb_start(section)) != 0)
        return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
        return ret;

    drv_generic_graphic_clear();

    if (!quiet) {
        char _buffer[40];
        qprintf(_buffer, sizeof(_buffer), "%s %dx%d", Name, DCOLS, DROWS);
        if (drv_generic_graphic_greet(_buffer, NULL)) {
            sleep(3);
            drv_generic_graphic_clear();
        }
    }

    /* register plugins */
    AddFunction("LCD::backlight", 1, plugin_backlight);

    return 0;
}


/* close driver & display */
int drv_ili9486_fb_quit(const int quiet)
{
    info("%s: shutting down.", Name);

    /* clear display */
    drv_generic_graphic_clear();

    //read goodby message from /tmp/lcd/goodbye
    char line1[80], value[80];
    FILE *fp;
    int i, size;

    fp = fopen("/tmp/lcd/goodbye", "r");
    if (!fp) {
        debug("couldn't open file '/tmp/lcd/goodbye'");
        line1[0] = '\0';
        if (!quiet) {
            drv_generic_graphic_greet("goodbye!", NULL);
        }
    } else {
        i = 0;
        while (!feof(fp) && i++ < 1) {
            fgets(value, sizeof(value), fp);
            size = strcspn(value, "\r\n");
            strncpy(line1, value, size);
            line1[size] = '\0';
            /* more than 80 chars, chew up rest of line */
            while (!feof(fp) && strchr(value, '\n') == NULL) {
                fgets(value, sizeof(value), fp);
            }
        }
        fclose(fp);
        if (i <= 1) {
            debug("'/tmp/lcd/goodbye' seems empty");
            line1[0] = '\0';
        }
        /* remove the file */
        debug("removing '/tmp/lcd/goodbye'");
        unlink("/tmp/lcd/goodbye");

        drv_generic_graphic_greet(NULL, line1);
    }

    drv_generic_graphic_quit();

    debug("closing connection");
    drv_ili9486_fb_close();

    return (0);
}


DRIVER drv_ili9486_fb = {
    .name = Name,
    .list = drv_ili9486_fb_list,
    .init = drv_ili9486_fb_init,
    .quit = drv_ili9486_fb_quit,
};
