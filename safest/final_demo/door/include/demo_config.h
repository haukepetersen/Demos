/*
 * Copyright (C) 2015 Hauke Petersen <mail@haukepetersen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @ingroup     peta
 * @{
 *
 * @file
 * @brief       Global configuration options
 *
 * @author      Hauke Petersen <mail@haukepetersen.de>
 */

#ifndef PETA_CONFIG_H
#define PETA_CONFIG_H

#include "periph/gpio.h"
#include "periph/spi.h"
#include "srf02.h"

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief   Distance sensor configuration
 * @{
 */
#define CONF_SENSE_I2C              (I2C_0)
#define CONF_SENSE_PERIOD           (100000)     /* sample every 100ms */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PETA_CONFIG_H*/
/** @} */
