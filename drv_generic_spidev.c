/* $Id$
 * $URL$
 *
 * generic driver helper for displays connected via SPI bus
 *
 * Copyright (C) 2012 Gabor Juhos <juhosg@openwrt.org>
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "debug.h"
#include "qprintf.h"
#include "cfg.h"
#include "drv_generic_spidev.h"

static char *generic_spidev_section = "";
static char *generic_spidev_driver = "";
static int generic_spidev_fd;

int drv_generic_spidev_open(const char *section, const char *driver)
{
    char *spidev;

    udelay_init();

    generic_spidev_section = (char *) section;
    generic_spidev_driver = (char *) driver;

    spidev = cfg_get(generic_spidev_section, "Port", NULL);

    info("%s: initializing SPI device %s", generic_spidev_driver, spidev);
    generic_spidev_fd = open(spidev, O_WRONLY);
    if (generic_spidev_fd < 0) {
        error("%s: unable to open SPI device %s!\n", generic_spidev_driver, spidev);
        goto exit_error;
    }

    return 0;

  exit_error:
    free(spidev);
    return -1;
}

int drv_generic_spidev_close(void)
{
    close(generic_spidev_fd);
    return 0;
}

int drv_generic_spidev_transfer(const int count, struct spi_ioc_transfer *tr)
{
    int ret;

    ret = ioctl(generic_spidev_fd, SPI_IOC_MESSAGE(count), tr);
    if (ret < count) {
        error("%s: can't send SPI message! (%s)\n", generic_spidev_driver, strerror(errno));
        return -1;
    }

    return 0;
}
