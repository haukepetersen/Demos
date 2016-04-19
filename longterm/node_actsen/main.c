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
 * @brief       Embedded World 2016 Demo: CoAP node
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
#include "saul_reg.h"
#include "periph/gpio.h"

#define UPDATE_INTERVAL     (1000 * 1000U)
#define EVT_REPEAT_DELAY    (1000)
#define DEBOUNCE_TIME       (50 * 1000)

#define EVT_REPEAT          (2)

#define MSG_UPDATE_EVENT    (0x3338)
#define MSG_BUTTON_EVENT    (0x3339)


#define Q_SZ                (8)
#define PRIO                (THREAD_PRIORITY_MAIN - 1)
#define COAP_SERVER_PORT    (5683)
#define SPORT               (1234)
#define UDP_PORT            (5683)
#define MAX_RESPONSE_LEN    500

static msg_t _coap_msg_q[Q_SZ], _beac_msg_q[Q_SZ];
static char coap_stack[THREAD_STACKSIZE_MAIN], beac_stack[THREAD_STACKSIZE_MAIN];

static uint8_t udp_buf[512];
static uint8_t scratch_raw[1024];      /* microcoap scratch buffer */
static coap_rw_buffer_t scratch_buf = { scratch_raw, sizeof(scratch_raw) };
static ipv6_addr_t dst_addr;

static xtimer_t debounce_timer;

/* buffer for composing SemML messages in */
static char p_buf[512];
static size_t initial_pos;

static uint8_t response[MAX_RESPONSE_LEN] = { 0 };



static const coap_endpoint_path_t path_riot_board = { 2, { "riot", "board" } };
static const coap_endpoint_path_t path_led = {1, {"led"} };

static int handle_post_led(coap_rw_buffer_t *scratch,
                                 const coap_packet_t *inpkt, coap_packet_t *outpkt,
                                 uint8_t id_hi, uint8_t id_lo)
{
    coap_responsecode_t resp = COAP_RSPCODE_CHANGED;
    printf("Hello, we got a post request to LED\n");
    printf("payload has length %i\n", (int)inpkt->payload.len);

    uint8_t val = inpkt->payload.p[0];

    if ((inpkt->payload.len == 1) && ((val == '1') || (val == '0'))) {
        gpio_write(LED_GPIO, (val - '1'));
        printf("LED something: %c\n", (char)val);
    }
    else {
        puts("wrong payload");
        resp = COAP_RSPCODE_NOT_ACCEPTABLE;
    }


    return coap_make_response(scratch, outpkt, NULL, 0,
                              id_hi, id_lo, &inpkt->token, resp,
                              COAP_CONTENTTYPE_TEXT_PLAIN, false);
}

static int handle_get_riot_board(coap_rw_buffer_t *scratch,
                                 const coap_packet_t *inpkt, coap_packet_t *outpkt,
                                 uint8_t id_hi, uint8_t id_lo)
{
    const char *riot_name = RIOT_BOARD;
    int len = strlen(RIOT_BOARD);

    memcpy(response, riot_name, len);

    return coap_make_response(scratch, outpkt, (const uint8_t *)response, len,
                              id_hi, id_lo, &inpkt->token, COAP_RSPCODE_CONTENT,
                              COAP_CONTENTTYPE_TEXT_PLAIN, false);
}

const coap_endpoint_t endpoints[] =
{
    { COAP_METHOD_GET,	handle_get_riot_board, &path_riot_board, "ct=0" },
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

static void btn_debounce_evt(void *arg)
{
    (void)arg;
    gpio_irq_enable(BUTTON_GPIO);
}

static void btn_evt(void *arg)
{
    msg_t m = { .type = MSG_BUTTON_EVENT };
    gpio_irq_disable(BUTTON_GPIO);
    xtimer_set(&debounce_timer, DEBOUNCE_TIME);
    msg_send(&m, *((kernel_pid_t *)arg));
}

static void send_btn_evt(size_t pos, char *buf)
{
    char btn = (gpio_read(BUTTON_GPIO)) ? '0' : '1';

    pos += sprintf(&buf[pos], "{\"n\":\"a:button\", \"u\":\"bool\", \"v\":\"%c\"}]",
                   btn);

    for (int i = 0; i < EVT_REPEAT; i++) {
        send_coap_post((uint8_t *)buf, pos);
        xtimer_usleep(EVT_REPEAT_DELAY);
    }
}

static void send_update(size_t pos, char *buf)
{
    char led = (gpio_read(LED_GPIO)) ? '0' : '1';
    char btn = (gpio_read(BUTTON_GPIO)) ? '1' : '0';

    pos += sprintf(&buf[pos], "{\"n\":\"a:led\", \"u\":\"bool\", \"v\":\"%c\"},",
                   led);
    pos += sprintf(&buf[pos], "{\"n\":\"a:button\", \"u\":\"bool\", \"v\":\"%c\"}]",
                   btn);

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

    /* register button event */
    debounce_timer.callback = btn_debounce_evt;
    gpio_init_int(BUTTON_GPIO, GPIO_PULLUP, GPIO_BOTH, btn_evt, &mypid);

    while(1) {
        msg_receive(&msg);

        switch (msg.type) {
            case MSG_UPDATE_EVENT:
                xtimer_set_msg(&status_timer, UPDATE_INTERVAL, &update_msg, mypid);
                send_update(initial_pos, p_buf);
                break;
            case MSG_BUTTON_EVENT:
                send_btn_evt(initial_pos, p_buf);
                break;
            default:
                break;
        }
    }

    /* never reached */
    return NULL;
}

static const shell_command_t shell_commands[] = { { NULL, NULL, NULL } };

int main(void)
{
    eui64_t iid;
    // uint16_t chan = 15;
    netopt_enable_t acks = NETOPT_DISABLE;
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];

    gnrc_netif_get(ifs);
    gnrc_netapi_set(ifs[0], NETOPT_AUTOACK, 0, &acks, sizeof(acks));
    ipv6_addr_from_str(&dst_addr, "2001:affe:1234::1");
    // gnrc_netapi_set(ifs[0], NETOPT_CHANNEL, 0, &chan, sizeof(chan));
    // ipv6_addr_from_str(&dst_addr, "fd38:3734:ad48:0:211d:50ce:a189:7cc4");



    /* initialize senml payload */
    gnrc_netapi_get(ifs[0], NETOPT_IPV6_IID, 0, &iid, sizeof(eui64_t));

    initial_pos  = sprintf(&p_buf[initial_pos], "[{\"bn\":\"urn:dev:mac:");
    initial_pos += sprintf(&p_buf[initial_pos], "%02x%02x%02x%02x%02x%02x%02x%02x",
                           iid.uint8[0], iid.uint8[1], iid.uint8[2], iid.uint8[3],
                           iid.uint8[4], iid.uint8[5], iid.uint8[6], iid.uint8[7]);
    initial_pos += sprintf(&p_buf[initial_pos], "\"},");


    thread_create(coap_stack, sizeof(coap_stack), PRIO - 1, THREAD_CREATE_STACKTEST, microcoap_server,
                  NULL, "coap");
    thread_create(beac_stack, sizeof(beac_stack), PRIO, THREAD_CREATE_STACKTEST, beaconing,
                  NULL, "beaconing");


    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
