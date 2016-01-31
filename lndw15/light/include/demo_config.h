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
 * @ingroup     scala
 * @{
 *
 * @file
 * @brief       Global configuration options
 *
 * @author      Hauke Petersen <mail@haukepetersen.de>
 * @author      Cenk Gündoğan <cnkgndgn@gmail.com>
 */

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

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
#define CONF_COMM_ADDR              {0x61, 0x62}
#define CONF_COMM_CHAN              (11U)

#define CONF_COMM_MSGID             (0xc5)
/** @} */

/**
 * @brief   Light configuration
 * @{
 */
#define CONF_LIGHT_PWM              (PWM_0)
#define CONF_LIGHT_CHAN             (0U)
/**  @} */

#ifdef __cplusplus
}
#endif

#endif /* DEMO_CONFIG_H*/
/** @} */
