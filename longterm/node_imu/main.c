/*
 * Copyright (C) 2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     node_imu
 * @{
 *
 * @file
 * @brief       IMU node talking to Horst
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "xtimer.h"
#include "byteorder.h"

#include "saul_reg.h"

#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "coap.h"

/**
 * @brief   The maximal expected link layer address length in byte
 */
#define MAX_ADDR_LEN            (8U)

#define DELAY                   (100000U)

static const ipv6_addr_t gw_addr = {{ 0x20, 0x01, 0xaf, 0xfe, \
                                      0x12, 0x34, 0x00, 0x00, \
                                      0x00, 0x00, 0x00, 0x00, \
                                      0x00, 0x00, 0x00, 0x01 }};

static const uint16_t gw_port = 5683;


static char payload[512];
static size_t pos;

void udp_send(ipv6_addr_t addr, uint16_t port, uint8_t *data, size_t len)
{
    gnrc_pktsnip_t *payload, *udp, *ip;
    /* allocate payload */
    payload = gnrc_pktbuf_add(NULL, data, len, GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        puts("Error: unable to copy data to packet buffer");
        return;
    }
    /* allocate UDP header, set source port := destination port */
    udp = gnrc_udp_hdr_build(payload, port, port);
    if (udp == NULL) {
        puts("Error: unable to allocate UDP header");
        gnrc_pktbuf_release(payload);
        return;
    }
    /* allocate IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, &addr);
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
                .numopts = 1,
                .opts    = {{{(uint8_t *)"senml", 5}, (uint8_t)COAP_OPTION_URI_PATH}},
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
    uint32_t last_wakeup = xtimer_now();
    phydat_t data[3];
    const char *types[] = {"s:acc", "s:mag", "s:gyro"};

    /* get the network device */
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];
    gnrc_netif_get(ifs);

    /* disable auto ACKS */
    netopt_enable_t acks = NETOPT_DISABLE;
    gnrc_netapi_set(ifs[0], NETOPT_AUTOACK, 0, &acks, sizeof(acks));

    /* get EUID (same than hardware address...) */
    eui64_t iid;
    gnrc_netapi_get(ifs[0], NETOPT_IPV6_IID, 0, &iid, sizeof(eui64_t));

    /* prepare JSON payload */
    pos = sprintf(payload, "[{\"bn\":\"urn:dev:mac:");
    for (int i = 0; i < 8; i++) {
        sprintf(&payload[pos], "%02x", iid.uint8[i]);
        pos += 2;
    }
    pos += sprintf(&payload[pos], "\"},");

    /* get sensors */
    saul_reg_t *acc = saul_reg_find_type(SAUL_SENSE_ACCEL);
    saul_reg_t *mag = saul_reg_find_type(SAUL_SENSE_MAG);
    saul_reg_t *gyr = saul_reg_find_type(SAUL_SENSE_GYRO);
    if ((acc == NULL) || (mag == NULL) || (gyr == NULL)) {
        puts("Unable to find sensors");
        return 1;
    }



    while (1) {
        /* get light value */
        saul_reg_read(acc, &data[0]);
        saul_reg_read(mag, &data[1]);
        saul_reg_read(gyr, &data[2]);

        // for (int i = 0; i < 3; i++) {
        //     phydat_dump(&data[i], 3);
        // }

        size_t p = pos;
        for (int i = 0; i < 3; i++) {
            p += sprintf(&payload[p],
                    "{\"n\":\"%s\", \"u\":\"g\", \"v\":[%i, %i, %i]},",
                    types[i], (int)data[i].val[0], (int)data[i].val[1], (int)data[i].val[2]);
        }
        p--;
        p += sprintf(&payload[p], "]");
        payload[p] = '\0';

        LED0_TOGGLE;

        /* push value using CoAP */
        send_coap_post((uint8_t *)payload, p);

        /* sleep a while */
        xtimer_usleep_until(&last_wakeup, DELAY);
    }

    return 0;
}
