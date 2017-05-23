/*
 * Copyright (C) 2017 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     demo_i3_emcute
 * @{
 *
 * @file
 * @brief       I3 demo JSON encoder
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fmt.h"
#include "luid.h"
#include "phydat.h"


static const uint8_t rand[] = { 0xa1, 0x48, 0xc3, 0x77, 0xaf };

static char did[11];
// static char sid[3][11];

static const char *pl0 = "{\"id\":\"";
static const char *pl1 = "\",\"val\":[";
static const char *pl2 = "]}";

static const char *ps2 = "\",\"bat\":723,\"up\":38373}"; //,\"fw\":\"a70b42\"}";

/* get ASCII hex string from binary ID */
static char nibble_to_char(uint8_t val)
{
    val &= 0x0f;
    if (val > 9) {
        return 'a' + (val - 10);
    }
    else {
        return '0' + val;
    }
}

static size_t hex_str(uint8_t *buf, size_t len, char *out)
{
    for (size_t i = 0; i < len; i++) {
        *(out++) = nibble_to_char(buf[i] >> 4);
        *(out++) = nibble_to_char(buf[i]);
    }
    *(out) = '\0';

    return (len * 2) + 1;
}

static size_t fill_id(char *buf)
{
    strcpy(buf, pl0);
    size_t pos = strlen(pl0);
    strcpy(&buf[pos], did);
    pos += strlen(did);
    return pos;
}

void encode_init(void)
{
    uint8_t tmp[5];

    luid_get(tmp, 5);

    /* extract device ID */
    hex_str(tmp, 5, did);
    // /* and 'pseudo-randomize' the device ID to get sensor IDs */
    // for (int i = 0; i < 3; i++) {
    //     for (int p = 0; p < 5; p++) {
    //         tmp[p] ^= rand[i];
    //     }
    //     hex_str(tmp, 5, sid[i]);
    // }
}

size_t encode_json(char *buf, phydat_t *data, int dim)
{
    size_t pos = fill_id(buf);
    strcpy(&buf[pos], pl1);
    pos += strlen(pl1);
    for (int i = 0; i < dim; i++) {
        pos += fmt_u16_dec(&buf[pos], data->val[i]);
        buf[pos++] = ',';
    }
    if (dim > 0) {
        --pos;
    }

    strcpy(&buf[pos], pl2);
    pos += strlen(pl2);
    buf[pos] = '\0';

    // printf("test:\n%s\n", buf);
    // printf("len is %i\n", (int)pos);

    return pos;
}

size_t encode_status(char *buf)
{
    size_t pos = fill_id(buf);
    strcpy(&buf[pos], ps2);
    return pos + strlen(ps2);
}
