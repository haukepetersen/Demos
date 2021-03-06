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

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief   Configure communication
 * @{
 */
#define CONF_COMM_PAN               (0x1593)
#define CONF_COMM_ADDR              {0x81, 0x95}
#define CONF_COMM_CHAN              (11U)

#define CONF_COMM_MSGID             (0xc5)

#define CONF_COMM_SCALA_ADDR        {0x61, 0x62}
/** @} */

/**
 * @brief   Sensor configuration
 * @{
 */
#define CONF_SENSE_ADC              (ADC_0)
#define CONF_SENSE_CHAN             (0)
#define CONF_SENSE_RATE             (10 * 1000)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PETA_CONFIG_H*/
/** @} */
