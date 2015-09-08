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
 * @brief       Network abstraction interface
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef DOOR_NET_H
#define DOOR_NET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Send event notification to SINK server via UDP
 *
 * @param[in] type      event type
 */
void net_send(uint8_t type);

#ifdef __cplusplus
}
#endif

#endif /* DOOR_NET_H*/
/** @} */
