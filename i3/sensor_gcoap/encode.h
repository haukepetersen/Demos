/*
 * Copyright (C) 2017 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     demo_i3_emcute
 *
 * @{
 * @file
 * @brief       Encode sensor data as pre-defined JSON string
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef ENCODE_H
#define ENCODE_H


#ifdef __cplusplus
extern "C" {
#endif

void encode_init(void);

size_t encode_json(char *buf, phydat_t *data, int dim);

size_t encode_status(char *buf);

#ifdef __cplusplus
}
#endif

#endif /* ENCODE_H */
/** @} */
