/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     demo_agile_kickoff
 * @{
 *
 * @file
 * @brief       Agile kick-off demo: CoAP light sensor node
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "kernel.h"
#include "shell.h"
#include "xtimer.h"

#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "coap.h"

#include "isl29020.h"


#define DELAY           (1000000U)

#define P1              "berlin"
#define P2              "light"
#define P3              "01"

static const ipv6_addr_t gw_addr = {{ 0x2a, 0x01, 0x04, 0xf8, \
                                      0x01, 0x51, 0x00, 0x64, \
                                      0x00, 0x00, 0x00, 0x00, \
                                      0x00, 0x00, 0x00, 0x12 }};

static const uint16_t gw_port = 5683;


void udp_send(ipv6_addr_t addr, uint16_t p, uint8_t *data, size_t len)
{
    uint8_t port[2];

    /* parse port */
    port[0] = (uint8_t)p;
    port[1] = p >> 8;

    gnrc_pktsnip_t *payload, *udp, *ip;
    /* allocate payload */
    payload = gnrc_pktbuf_add(NULL, data, len, GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        puts("Error: unable to copy data to packet buffer");
        return;
    }
    /* allocate UDP header, set source port := destination port */
    udp = gnrc_udp_hdr_build(payload, port, 2, port, 2);
    if (udp == NULL) {
        puts("Error: unable to allocate UDP header");
        gnrc_pktbuf_release(payload);
        return;
    }
    /* allocate IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, 0, (uint8_t *)&addr, sizeof(addr));
    if (ip == NULL) {
        puts("Error: unable to allocate IPv6 header");
        gnrc_pktbuf_release(udp);
        return;
    }
    /* send packet */
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP, GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
        puts("Error: unable to locate UDP thread");
        gnrc_pktbuf_release(ip);
        return;
    }
    printf("Success: send %u byte\n", (unsigned)payload->size);
}


void send_coap_post(uint8_t *data, size_t len)
{
        uint8_t  snd_buf[128];
        size_t   req_pkt_sz;

        coap_header_t req_hdr = {
                .version = 1,
                .type    = COAP_TYPE_NONCON,
                .tkllen  = 0,
                .code    = COAP_METHOD_POST,
                .mid     = {5, 57}            // is equivalent to 1337 when converted to uint16_t
        };

        coap_buffer_t payload = {
                .p   = data,
                .len = len
        };

        coap_packet_t req_pkt = {
                .header  = req_hdr,
                .token   = (coap_buffer_t) {},
                .numopts = 3,
                .opts    = { {{(uint8_t *)P1, 6}, (uint8_t)COAP_OPTION_URI_PATH},
                             {{(uint8_t *)P2, 5}, (uint8_t)COAP_OPTION_URI_PATH},
                             {{(uint8_t *)P3, 2}, (uint8_t)COAP_OPTION_URI_PATH} },
                .payload = payload
        };

        req_pkt_sz = sizeof(req_pkt);

        if (coap_build(snd_buf, &req_pkt_sz, &req_pkt) != 0) {
                printf("CoAP build failed :(\n");
                return;
        }

        udp_send(gw_addr, gw_port, snd_buf, req_pkt_sz);
}


int main(void)
{
    isl29020_t dev;
    const char *id = "light";
    char buf[64];
    uint32_t last_wakeup = xtimer_now();

    /* initialize light sensor */
    isl29020_init(&dev, ISL29020_I2C, ISL29020_ADDR,
                  ISL29020_RANGE_16K, ISL29020_MODE_AMBIENT);

    while (1) {
        /* get light value */
        int value = isl29020_read(&dev);
        sprintf(buf, "%s:%i", id, value);

        /* push value using CoAP */
        send_coap_post((uint8_t *)buf, strlen(buf));

        /* sleep a while */
        xtimer_usleep_until(&last_wakeup, DELAY);
    }

    return 0;
}
