/*
 * Copyright (C) 2015 Hauke Petersen <mail@haukepetersen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @ingroup     scala
 * @{
 *
 * @file
 * @brief       Dino control
 *
 * @author      Hauke Petersen <mail@haukepetersen.de>
 *
 * @}
 */

#include <stdio.h>

#include "thread.h"
#include "kernel.h"
#include "hwtimer.h"
#include "periph/gpio.h"

#include "demo_config.h"

#define PRIO            (THREAD_PRIORITY_MAIN - 1)

static char stack[THREAD_STACKSIZE_MAIN];

static volatile int rocking = 0;

void *dino_thread(void *arg)
{
    (void)arg;

    while (1) {
        while (rocking == 0) {
            gpio_clear(CONF_DINO_GPIO);
            hwtimer_wait(CONF_DINO_DELAY_WAITING);
        }
        gpio_set(CONF_DINO_GPIO);
        hwtimer_wait(CONF_DINO_DELAY_ROCKING);
        rocking = 0;
    }

    /* never reached (hopefully) */
    return NULL;
}


void dino_init(void)
{
    /* initialize dino's GPIO pin */
    gpio_init_out(CONF_DINO_GPIO, GPIO_NOPULL);
    /* start dino thread */
    thread_create(stack, sizeof(stack), PRIO, 0, dino_thread, NULL, "dino");
}

void dino_rock(void)
{
    rocking = 1;
}
