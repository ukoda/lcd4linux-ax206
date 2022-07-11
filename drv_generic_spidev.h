/* $Id$
 * $URL$
 *
 * generic driver helper for displays connected via SPI bus
 *
 * Copyright (C) 2012 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2012 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 *
 * exported fuctions:
 *
 * int drv_generic_spidev_open (const char *section, const char *driver)
 *   reads 'Port' entry from config and opens
 *   the SPI device
 *   returns 0 if ok, -1 on failure
 *
 * int drv_generic_spidev_close (void)
 *   closes SPI device
 *   returns 0 if ok, -1 on failure
 *
 * void drv_generic_spidev_transfer (int count, struct spi_ioc_transfer *tr)
 *   transfer data to/from the SPI device
 *
 */

#ifndef _DRV_GENERIC_SPIDEV_H_
#define _DRV_GENERIC_SPIDEV_H_

#include <linux/spi/spidev.h>

int drv_generic_spidev_open(const char *section, const char *driver);
int drv_generic_spidev_close(void);
int drv_generic_spidev_transfer(const int count, struct spi_ioc_transfer *tr);

#endif                          /* _DRV_GENERIC_SPIDEV_H_ */
