/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     door
 * @{
 *
 * @file
 * @brief       Global configuration options
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef DOOR_DEMO_CONFIG_H
#define DOOR_DEMO_CONFIG_H

#include "periph/gpio.h"
#include "periph/spi.h"
#include "srf02.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Message format and types
 * @{
 */
#define CONF_MSG_HEAD               (23U)
#define CONF_MSG_BUSY               (0U)
#define CONF_MSG_FREE               (1U)

#define CONF_MSG_RETRANS            (2)
/** @} */

/**
 * @brief   Sink address and port
 * @{
 */
#define CONF_SINK_ADDR              {{ 0xab, 0xcd, 0x00, 0x00, \
                                       0x00, 0x00, 0x00, 0x00, \
                                       0x00, 0x00, 0x00, 0x00, \
                                       0x00, 0x00, 0x08, 0x15 }}
#define CONF_SINK_PORT              { 0x2c, 0x2f }        /* port 11311 */
/** @} */

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

#endif /* DOOR_DEMO_CONFIG_H*/
/** @} */
