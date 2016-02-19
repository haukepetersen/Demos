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
#include "hdc1000.h"
#include "tcs37727.h"
#include "mpl3115a2.h"

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
static msg_t _beac_msg_q[Q_SZ];

static hdc1000_t th_dev;
static mpl3115a2_t p_dev;
static tcs37727_t light_dev;

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

void send_coap_post(uint8_t *data, size_t len)
{
    uint8_t  snd_buf[512];
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
	uint32_t pressure;
    uint16_t rawtemp, rawhum;
    int temp, hum, temp_abs, hum_abs, pressure_abs;
	uint8_t status;
	tcs37727_data_t light_data;

	hdc1000_read(&th_dev, &rawtemp, &rawhum);
	hdc1000_convert(rawtemp, rawhum,  &temp, &hum);
    temp_abs = temp / 100;
    hum_abs = hum / 100;
    temp -= temp_abs * 100;
    hum -= hum_abs * 100;

	mpl3115a2_read_pressure(&p_dev, &pressure, &status);
    pressure_abs = pressure / 100000;
    pressure -= pressure_abs * 100000;
	//mpl3115a2_read_temp(&p_dev, &temp);

	tcs37727_read(&light_dev, &light_data);

    pos += sprintf(&buf[pos], "{\"n\":\"s:temp\", \"u\":\"°C\", \"v\":\"%2i.%03i\"},",
                   temp_abs, temp);
    pos += sprintf(&buf[pos], "{\"n\":\"s:hum\", \"u\":\"%%RH\", \"v\":\"%2i.%03i\"},",
                   hum_abs, hum);
    pos += sprintf(&buf[pos], "{\"n\":\"s:pres\", \"u\":\"bar\", \"v\":\"%2i.%03i\"},",
                   pressure_abs, (unsigned int) pressure);
    pos += sprintf(&buf[pos], "{\"n\":\"s:rgb\", \"u\":\"RGB\", \"v\":\"[%"PRIu32", %"PRIu32", %"PRIu32"]\"}]",
                   light_data.red, light_data.green, light_data.blue);

    hdc1000_startmeasure(&th_dev);

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

    /* initialize sensors */
    hdc1000_init(&th_dev, HDC1000_I2C, HDC1000_ADDR);
    hdc1000_startmeasure(&th_dev);
    mpl3115a2_init(&p_dev, MPL3115A2_I2C, MPL3115A2_ADDR, MPL3115A2_OS_RATIO_DEFAULT);
    mpl3115a2_set_active(&p_dev);
    tcs37727_init(&light_dev, TCS37727_I2C, TCS37727_ADDR, TCS37727_ATIME_DEFAULT);
    tcs37727_set_rgbc_active(&light_dev);

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
