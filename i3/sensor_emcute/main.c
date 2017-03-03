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
 * @brief       MQTT based sensor node
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "luid.h"
#include "xtimer.h"
#include "saul_reg.h"
#include "periph/gpio.h"

#include "net/emcute.h"
#include "net/ipv6/addr.h"

#include "encode.h"

#define GW_ADDR             ("affe::1")
#define GW_PORT             (1885U)

#ifndef NODE_ID
#define NODE_ID             ("p.schmerzl")
#endif

#define TOPIC_SENSE         ("msa/sense")

#define LINKPIN             (GPIO_PIN(PORT_E, 2))
#define LINKPORT            (UART_DEV(1))

#define STACKSIZE           (THREAD_STACKSIZE_DEFAULT)
#define PRIO                (THREAD_PRIORITY_MAIN - 2)

#define FLAG_CH             (0x0100)
#define FLAG_CON            (0x0200)
#define FLAG_SEN            (0x0400)
#define FLAG_ALL            (0x0700)

#define TIMEOUT_SAMPLE      (1 * US_PER_SEC)
#define TIMEOUT_CONNECT     (5 * US_PER_SEC)
#define TIMEOUT_DEBOUNCE    (50 * US_PER_MS)

/* we need an additional thread to run emCute so we need some stack memory */
char emstack[STACKSIZE];

/* some useful timers */
static xtimer_t tim_evt;
static xtimer_t tim_debounce;

static thread_t *evt_handler_thread;

static volatile uint16_t state;

static char payload[128];

enum {
    LED_RED   = 0,
    LED_GREEN = 1,
    LED_BLUE  = 2
};

static gpio_t leds[] = { LED0_PIN, LED1_PIN, LED2_PIN };

static void led(int n)
{
    for (int i = 0; i < 3; i++) {
        gpio_write(leds[i], !(i == n));
    }
}

/* run emcute */
static void *emcute(void *arg)
{
    emcute_run(EMCUTE_DEFAULT_PORT, (char *)arg);
    return NULL;    /* should never be reached */
}

/* timer and GPIO event callbacks */
static void evt_to(void *arg)
{
    int f = (int)arg;
    thread_flags_set(evt_handler_thread, (thread_flags_t)f);
}

static void evt_to_debounce(void *arg)
{
    int pinstate = gpio_read(LINKPIN);
    if (pinstate && (state == FLAG_CH)) {
        state = FLAG_CON;
        thread_flags_set(evt_handler_thread, FLAG_CON);
    }
    else if (!pinstate && (state != FLAG_CH)) {
        state = FLAG_CH;
        thread_flags_set(evt_handler_thread, FLAG_CH);
    }
    gpio_irq_enable(LINKPIN);
}

static void evt_pin(void *arg)
{
    (void)arg;

    gpio_irq_disable(LINKPIN);
    tim_debounce.arg = (void *)gpio_read(LINKPIN);
    xtimer_set(&tim_debounce, TIMEOUT_DEBOUNCE);
}

int main(void)
{
    evt_handler_thread = (thread_t *)sched_active_thread;
    sock_udp_ep_t gw_ep = SOCK_IPV6_EP_ANY;
    emcute_topic_t topic = { TOPIC_SENSE, 0 };

    /* init the encoder */
    encode_init();

    /* initialize timers */
    tim_evt.callback = evt_to;
    tim_debounce.callback = evt_to_debounce;

    /* set MQTT-SN gateway info */
    ipv6_addr_from_str((ipv6_addr_t *)&gw_ep.addr, GW_ADDR);
    gw_ep.port = GW_PORT;

    /* start the emCute thread */
    thread_create(emstack, sizeof(emstack), PRIO, 0, emcute, NODE_ID, "mqtt-sn");

    /* initialize LINKPIN */
    gpio_init_int(LINKPIN, GPIO_IN_PU, GPIO_BOTH, evt_pin, NULL);

    /* set initial state */
    state = (gpio_read(LINKPIN)) ? FLAG_CON : FLAG_CH;
    thread_flags_set(evt_handler_thread, state);
    led(LED_RED);

    /* get the light sensor */
    saul_reg_t *light = saul_reg_find_type(SAUL_SENSE_COLOR);
    phydat_t data;

    while (1) {
        thread_flags_t evt = thread_flags_wait_any(FLAG_ALL);

        if (evt & FLAG_CH) {
            emcute_discon();
            xtimer_remove(&tim_evt);
            led(LED_BLUE);
            puts("\n*** CONNECTED TO CHARGING STATION ***");
        }
        else if (evt & FLAG_CON) {
            led(LED_RED);

            if (state == FLAG_CON) {
                puts("\n*** TRYING TO CONNECT TO GATEWAY ***");
                state = FLAG_SEN;
            }

            /* try to connect to gateway */
            if (emcute_con(&gw_ep, true, NULL, NULL, 0, 0) != EMCUTE_OK) {
                tim_evt.arg = (void *)FLAG_CON;
                xtimer_set(&tim_evt, TIMEOUT_CONNECT);
                puts("connection request timed out");
            }
            else {
                /* register topic */
                emcute_reg(&topic);
                thread_flags_set(evt_handler_thread, FLAG_SEN);
                puts("\n*** CONNECTED TO GATEWAY ***");
            }
        }
        else if (evt & FLAG_SEN) {
            led(LED_GREEN);
            tim_evt.arg = (void *)FLAG_SEN;
            xtimer_set(&tim_evt, TIMEOUT_SAMPLE);

            saul_reg_read(light, &data);
            size_t pos = encode_json(payload, &data, 3);

            if (emcute_pub(&topic, payload, pos, EMCUTE_QOS_1) == EMCUTE_OK) {
                printf("pub -> '%s': %s\n", topic.name, payload);
            }
            else {
                puts("publishing of data failed\n");
            }
        }

    }

    return 0;
}
