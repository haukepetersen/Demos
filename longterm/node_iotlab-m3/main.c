/*
 * Copyright (C) 2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     demo_embedded_world_2016
 * @{
 *
 * @file
 * @brief       Embedded World 2016 Demo: CoAP node (iotlab-m3)
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Cenk Gündoğan <cnkgndgn@gmail.com>
 *
 * @}
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/af.h>

#include "board.h"
#include "kernel.h"
#include "shell.h"
#include "xtimer.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "net/conn.h"
#include "net/conn/udp.h"
#include "coap.h"
#include "periph/gpio.h"
#include "isl29020.h"
#include "lps331ap.h"
#include "periph/gpio.h"

#define UPDATE_INTERVAL     (1000 * 1000U)
#define MSG_UPDATE_EVENT    (0x3338)

#define Q_SZ                (4)
#define PRIO                (THREAD_PRIORITY_MAIN - 1)
#define COAP_SERVER_PORT    (5683)
#define SPORT               (1234)
#define UDP_PORT            (5683)

#define LIGHT_MODE          ISL29020_MODE_AMBIENT
#define LIGHT_RANGE         ISL29020_RANGE_16K
#define TP_RATE             LPS331AP_RATE_7HZ

#ifdef WITH_SHELL
static msg_t _main_msg_q[Q_SZ];
static char beac_stack[THREAD_STACKSIZE_DEFAULT];
#endif
static msg_t _coap_msg_q[Q_SZ], _beac_msg_q[Q_SZ];
static char coap_stack[THREAD_STACKSIZE_DEFAULT];

static isl29020_t light_dev;
static lps331ap_t tp_dev;

static uint8_t udp_buf[512];
static uint8_t scratch_raw[1024];      /* microcoap scratch buffer */
static coap_rw_buffer_t scratch_buf = { scratch_raw, sizeof(scratch_raw) };
static ipv6_addr_t dst_addr;

/* buffer for composing SemML messages in */
static char p_buf[512];
static size_t initial_pos;

static const coap_header_t req_hdr = {
        .version = 1,
        .type    = COAP_TYPE_NONCON,
        .tkllen  = 0,
        .code    = COAP_METHOD_POST,
        .mid     = {5, 57}            // is equivalent to 1337 when converted to uint16_t
};

static const coap_endpoint_path_t path_led = { 1, { "led" } };

static int handle_post_led(coap_rw_buffer_t *scratch,
                           const coap_packet_t *inpkt, coap_packet_t *outpkt,
                           uint8_t id_hi, uint8_t id_lo)
{
    coap_responsecode_t resp = COAP_RSPCODE_CHANGED;
    uint8_t val = inpkt->payload.p[0];

    if ((inpkt->payload.len == 1) && ((val == '1') || (val == '0'))) {
        gpio_write(LED_RED_GPIO, (val - '1'));
    }
    else {
        resp = COAP_RSPCODE_NOT_ACCEPTABLE;
    }

    return coap_make_response(scratch, outpkt, NULL, 0,
                              id_hi, id_lo, &inpkt->token, resp,
                              COAP_CONTENTTYPE_TEXT_PLAIN, false);
}

const coap_endpoint_t endpoints[] =
{
    { COAP_METHOD_POST,  handle_post_led, &path_led, "ct=0" },
    /* marks the end of the endpoints array: */
    { (coap_method_t)0, NULL, NULL, NULL }
};

void *microcoap_server(void *arg)
{
    (void) arg;
    msg_init_queue(_coap_msg_q, Q_SZ);

    uint8_t laddr[16] = { 0 };
    uint8_t raddr[16] = { 0 };
    size_t raddr_len;
    uint16_t rport;
    conn_udp_t conn;
    int rc = conn_udp_create(&conn, laddr, sizeof(laddr), AF_INET6, COAP_SERVER_PORT);

    while (1) {
        if ((rc = conn_udp_recvfrom(&conn, (char *)udp_buf, sizeof(udp_buf),
                                    raddr, &raddr_len, &rport)) < 0) {
            continue;
        }

        coap_packet_t pkt;
        /* parse UDP packet to CoAP */
        if (0 == (rc = coap_parse(&pkt, udp_buf, rc))) {
            coap_packet_t rsppkt;

            /* handle CoAP request */
            coap_handle_req(&scratch_buf, &pkt, &rsppkt, false, false);

            /* build reply */
            size_t rsplen = sizeof(udp_buf);
            if ((rc = coap_build(udp_buf, &rsplen, &rsppkt)) == 0) {
                /* send reply via UDP */
                rc = conn_udp_sendto(udp_buf, rsplen, NULL, 0, raddr, raddr_len,
                                     AF_INET6, COAP_SERVER_PORT, rport);
            }
        }
    }

    /* never reached */
    return NULL;
}

void send_coap_post(uint8_t *data, size_t len)
{
    uint8_t  snd_buf[128];
    size_t   req_pkt_sz;

    coap_buffer_t payload = {
            .p   = data,
            .len = len
    };

    coap_packet_t req_pkt = {
            .header  = req_hdr,
            .token   = (coap_buffer_t) { 0 },
            .numopts = 1,
            .opts    = {{{(uint8_t *)"senml", 5}, (uint8_t)COAP_OPTION_URI_PATH}},
            .payload = payload
    };

    req_pkt_sz = sizeof(req_pkt);

    if (coap_build(snd_buf, &req_pkt_sz, &req_pkt) != 0) {
            printf("CoAP build failed :(\n");
            return;
    }

    conn_udp_sendto(snd_buf, req_pkt_sz, NULL, 0, &dst_addr, sizeof(dst_addr),
                    AF_INET6, SPORT, UDP_PORT);
}

static void send_update(size_t pos, char *buf)
{
    char led = (gpio_read(LED_RED_GPIO)) ? '0' : '1';
    int lux, pres, pres_abs, temp, temp_abs;

    lux = isl29020_read(&light_dev);

    pres = lps331ap_read_pres(&tp_dev);
    temp = lps331ap_read_temp(&tp_dev);
    pres_abs = pres / 1000;
    pres -= pres_abs * 1000;
    temp_abs = temp / 1000;
    temp -= temp_abs * 1000;

    pos += sprintf(&buf[pos], "{\"n\":\"a:led\", \"u\":\"bool\", \"v\":\"%c\"},",
                   led);
    pos += sprintf(&buf[pos], "{\"n\":\"s:light\", \"u\":\"lux\", \"v\":\"%d\"},",
                   lux);
    pos += sprintf(&buf[pos], "{\"n\":\"s:pressure\", \"u\":\"bar\", \"v\":\"%2i.%03i\"},",
                   pres_abs, pres);
    pos += sprintf(&buf[pos], "{\"n\":\"s:temp\", \"u\":\"°C\", \"v\":\"%2i.%03i\"}]",
                   temp_abs, temp);

    send_coap_post((uint8_t *)buf, pos);
}

void *beaconing(void *arg)
{
    (void) arg;
    xtimer_t status_timer;
    msg_t msg;
    msg_t update_msg;
    kernel_pid_t mypid = thread_getpid();

    /* initialize message queue */
    msg_init_queue(_beac_msg_q, Q_SZ);

    /* start periodic timer */
    update_msg.type = MSG_UPDATE_EVENT;
    xtimer_set_msg(&status_timer, UPDATE_INTERVAL, &update_msg, mypid);

    while(1) {
        msg_receive(&msg);

        switch (msg.type) {
            case MSG_UPDATE_EVENT:
                xtimer_set_msg(&status_timer, UPDATE_INTERVAL, &update_msg, mypid);
                send_update(initial_pos, p_buf);
                break;
            default:
                break;
        }
    }

    /* never reached */
    return NULL;
}

#ifdef WITH_SHELL
static const shell_command_t shell_commands[] = { { NULL, NULL, NULL } };
#endif

int main(void)
{
#ifdef WITH_SHELL
    /* initialize message queue */
    msg_init_queue(_main_msg_q, Q_SZ);
#endif

    eui64_t iid;
    netopt_enable_t acks = NETOPT_DISABLE;
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];

    gnrc_netif_get(ifs);
    gnrc_netapi_set(ifs[0], NETOPT_AUTOACK, 0, &acks, sizeof(acks));
    ipv6_addr_from_str(&dst_addr, "2001:affe:1234::1");
    // ipv6_addr_from_str(&dst_addr, "fd38:3734:ad48:0:211d:50ce:a189:7cc4");

    /* initialize senml payload */
    gnrc_netapi_get(ifs[0], NETOPT_IPV6_IID, 0, &iid, sizeof(eui64_t));

    initial_pos  = sprintf(&p_buf[initial_pos], "[{\"bn\":\"urn:dev:mac:");
    initial_pos += sprintf(&p_buf[initial_pos], "%02x%02x%02x%02x%02x%02x%02x%02x",
                           iid.uint8[0], iid.uint8[1], iid.uint8[2], iid.uint8[3],
                           iid.uint8[4], iid.uint8[5], iid.uint8[6], iid.uint8[7]);
    initial_pos += sprintf(&p_buf[initial_pos], "\"},");

    /* initialize ISL29020 Light Sensor */
    isl29020_init(&light_dev, ISL29020_I2C, ISL29020_ADDR, LIGHT_RANGE, LIGHT_MODE);
    lps331ap_init(&tp_dev, LPS331AP_I2C, LPS331AP_ADDR, TP_RATE);

    LED_GREEN_OFF;
    LED_ORANGE_OFF;
    LED_RED_OFF;

    thread_create(coap_stack, sizeof(coap_stack), PRIO - 1, THREAD_CREATE_STACKTEST, microcoap_server,
                  NULL, "coap");
#ifdef WITH_SHELL
    thread_create(beac_stack, sizeof(beac_stack), PRIO, THREAD_CREATE_STACKTEST, beaconing,
                  NULL, "beaconing");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
#else
    beaconing(NULL);
#endif

    return 0;
}
