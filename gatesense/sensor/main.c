/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Posix UDP Server/Client example
 *
 * @author      Christian Mehlis <mehlis@inf.fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "posix_io.h"
#include "shell.h"
#include "board_uart0.h"
#include "kernel.h"
#include "thread.h"

#include "config.h"
#include "netsetup.h"
#include "net.h"
#include "sensor.h"

static char sensor_stack[KERNEL_CONF_STACKSIZE_MAIN];

int sensor_pid;

const shell_command_t shell_commands[] = {
    {"send", "send data through udp", shell_send},
    {NULL, NULL, NULL}
};

int main(void)
{
    puts("Sensor Token Up");

    /* we assume the transceiver is started! */
    netsetup_set_channel(CONFIG_CHANNEL);
    netsetup_set_address(CONFIG_OWN_ADDRESS);
    netsetup_register_ondata(net_receive);
    printf("CHANNEL: %d\tADDRESS: %d\n", CONFIG_CHANNEL, CONFIG_OWN_ADDRESS);
    netsetup_start();

    /* start acc and game threads */
    sensor_pid = thread_create(sensor_stack, KERNEL_CONF_STACKSIZE_MAIN, PRIORITY_MAIN - 2, CREATE_STACKTEST,
                            sensor_thread, "sensor_reader");

    /* start shell */
    posix_open(uart0_handler_pid, 0);
    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);
    shell_run(&shell);
}
