/*
 * Copyright (C) 2017 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     demos_i3
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

#include "uuid.h"
#include "xtimer.h"
#include "emcute.h"
#include "saul_reg.h"

#define GW_ADDR             ("fec0:affe::1")
#define GW_PORT             (1885U)

#define LINKPIN             (GPIO_PIN(PC, 3))
#define LINKPORT            (UART_DEV(1))

#define STACKSIZE           (THREAD_STACKSIZE_DEFAULT)
#define PRIO                (THREAD_PRIORITY_MAIN - 2)

#define FLAG_TO_SAMPLE      (0x0001)
#define FLAG_TO_CONNECT     (0x0002)
#define FLAG_TO_DEBOUNCE    (0x0004)
#define FLAG_ALL            (FLAG_TO_SAMPLE | FLAG_TO_CONNECT | FLAG_TO_DEBOUNCE)

#define TIMEOUT_SAMPLE      (1 * US_PER_SEC)
#define TIMEOUT_CONNECT     (10 * US_PER_SEC)
#define TIMEOUT_DEBOUNCE    (100 * US_PER_MS)

/* we need an additional thread to run emCute so we need some stack memory */
char emstack[STACKSIZE];

/* some useful timers */
static xtimer_t tim_sample;
static xtimer_t tim_connect;
static xtimer_t tim_debounce;

static thread_t *evt_handler_thread;

/* run emcute */
static void *emcute(void *arg)
{
    emcute_run(EMCUTE_DEFAULT_PORT, (char *)arg);
}

/* timer and GPIO event callbacks */
static void evt_to(void *arg)
{
    thread_flags_set(evt_handler_thread, (thread_flags_t)arg);
}

static void evt_to_debounce(void *arg)
{
    int state = (int)arg;
    if ()
    thread_flags_set(evt_handler_thread, FLAG_STATE_CHARGING);
    gpio_irq_enable(LINKPIN);
}

static void evt_pin(void *arg)
{
    linkstate = gpio_read(LINKPIN);
    gpio_irq_disable(LINKPIN);
    tim_debounce.arg = (void *)linkstate;
    xtimer_set(tim_debounce, TIMEOUT_DEBOUNCE);
}

int main(void)
{
    uint32_t wakeup = xtimer_now_usec();
    evt_handler_thread = sched_active_thread;

    /* initialize timers */
    tim_sample.callback = evt_to;
    tim_sample.arg = (void *)FLAG_TO_SAMPLE;
    tim_connect.callback = evt_to;
    tim_connect.arg = (void *)FLAG_TO_CONNECT;
    tim_debounce.callback = evt_to;
    tim_debounce.arg = (void *)FLAG_TO_DEBOUNCE;

    /* start the emCute thread */
    thread_create(emstack, sizeof(emstack), PRIO, 0, emcute, id, "mqtt-sn");


    while (1) {
        thread_flags_t evt = thread_flags_any(FLAG_ALL);

        if (evt & FLAG_TO_DEBOUNCE) {
            int state = gpio_read(LINKPIN)
        }

    }

    return 0;
}
