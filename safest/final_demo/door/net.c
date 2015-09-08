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
 * @brief       Network simplification code
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdint.h>

#include "net/gnrc.h"
#include "net/ipv6.h"
#include "net/gnrc/ipv6.h"
#include "net/udp.h"
#include "net/gnrc/udp.h"

#include "net.h"
#include "demo_config.h"

static uint8_t seq = 0;

static ipv6_addr_t sink_addr = CONF_SINK_ADDR;
static uint8_t sink_port[2] = CONF_SINK_PORT;

static void sendudp(uint8_t type)
{
    gnrc_pktsnip_t *snip, *udp, *ip;

    /* payload */
    snip = gnrc_pktbuf_add(NULL, NULL, 3, GNRC_NETTYPE_UNDEF);
    if (snip == NULL) {
        return;
    }
    uint8_t *load = (uint8_t *)snip->data;
    load[0] = CONF_MSG_HEAD;
    load[1] = seq;
    load[2] = type;
    /* udp header */
    udp = gnrc_udp_hdr_build(snip, sink_port, 2, sink_port, 2);
    if (udp == NULL) {
        gnrc_pktbuf_release(snip);
        return;
    }
    /* ip header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, 0, (uint8_t *)&sink_addr,
                             sizeof(sink_addr));
    if (ip == NULL) {
        gnrc_pktbuf_release(udp);
        return;
    }
    /* send packet */
    gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP, GNRC_NETREG_DEMUX_CTX_ALL, ip);
}

void net_send(uint8_t type)
{
    for (int i = 0; i < CONF_MSG_RETRANS; i++) {
        sendudp(type);
    }
    ++seq;
}

