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
 * @brief       Sensing module
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "thread.h"
#include "vtimer.h"
#include "srf02.h"

#include "net.h"
#include "demo_config.h"

#define BASELINE_COUNT          (30U)
#define THRESHOLD               (25U)
#define BOGUS                   (25U)


#define PRIO                    (THREAD_PRIORITY_MAIN - 3)
static char stack[THREAD_STACKSIZE_MAIN];

static void(*on_data)(uint16_t);

static int init_counter = 0;
static int evt_counter = 0;
static uint32_t baseline = 0;


static void process(uint16_t range)
{
    /* filter bogus readings */
    if (range < BOGUS) {
        return;
    }
    if (range < (baseline - THRESHOLD)) {
        ++evt_counter;
    }
    else {
        if (evt_counter != 0) {
            net_send(CONF_MSG_FREE);
            printf("CLEAR: %i\n", (int)range);
            evt_counter = 0;
        }
    }
    if (evt_counter == 1) {
        net_send(CONF_MSG_BUSY);
        printf("EVENT: %i\n", (int)range);
    }
}

static void startup(uint16_t range)
{
    ++init_counter;
    baseline += range;
    if (init_counter == BASELINE_COUNT) {
        on_data = process;
        baseline /= BASELINE_COUNT;
        printf("now processing - baseline: %u\n", (unsigned)baseline);
    }
}

void *sense_thread(void *arg)
{
    (void)arg;
    srf02_t dev;
    // uint32_t last_wakeup = xtimer_now();
    uint16_t range;

    /* set initial data handler */
    on_data = startup;

    /* initialize the distance sensor */
    srf02_init(&dev, CONF_SENSE_I2C, SRF02_DEFAULT_ADDR);

    /* read sensor periodically */
    while (1) {
        // xtimer_usleep_until(&last_wakeup, CONF_SENSE_PERIOD);
        vtimer_usleep(30 * 1000);
        range = srf02_get_distance(&dev, SRF02_MODE_REAL_CM);
        on_data(range);
    }

    /* never to be reached */
    return NULL;
}

void sense_run(kernel_pid_t radio)
{
    /* start the sampling thread */
    thread_create(stack, sizeof(stack), PRIO, 0, sense_thread, NULL, "sense");
}
