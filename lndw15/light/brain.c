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
 * @brief       Brain
 *
 * @author      Hauke Petersen <mail@haukepetersen.de>
 * @author      Cenk Gündoğan <cnkgndgn@gmail.com>
 *
 * @}
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "kernel.h"
#include "thread.h"
#include "msg.h"
#include "periph/gpio.h"
#include "servo.h"
#include "net/ng_netbase.h"

#include "brain.h"
#include "comm.h"
#include "demo_config.h"

#define STACKSIZE           (THREAD_STACKSIZE_MAIN)
#define STACKPRIO           (THREAD_PRIORITY_MAIN - 2)

static char stack[STACKSIZE];

static void _dispatch(uint8_t *data, size_t len)
{
    if ((len != 2) || (data[0] != CONF_COMM_MSGID)) {
        printf("ignore frame with payload size: %d and MSGID: %d\n", len, data[0]);
        return;
    }

    pwm_set(CONF_LIGHT_PWM, CONF_LIGHT_CHAN, data[1]);
    return;
}

static void *_brain_thread(void *arg)
{
    ng_netreg_entry_t netreg;
    ng_pktsnip_t *snip;
    msg_t msg;

    netreg.pid = thread_getpid();
    netreg.demux_ctx = NG_NETREG_DEMUX_CTX_ALL;
    ng_netreg_register(NG_NETTYPE_UNDEF, &netreg);

    while (1) {
        msg_receive(&msg);

        if (msg.type == NG_NETAPI_MSG_TYPE_RCV) {
            snip = (ng_pktsnip_t *)msg.content.ptr;
            _dispatch(snip->data, snip->size);
            ng_pktbuf_release(snip);
        }
    }

    /* never reached */
    return NULL;
}

void brain_init(void)
{
    /* initialize the light */
    puts("init light");
    pwm_init(CONF_LIGHT_PWM, PWM_LEFT, 500, 0xff);

    /* initializing network support */
    puts("init comm");
    comm_init();

    /* run brain thread */
    puts("run the brain");
    thread_create(stack, STACKSIZE, STACKPRIO, CREATE_STACKTEST, _brain_thread,
                  NULL, "brain");
}
