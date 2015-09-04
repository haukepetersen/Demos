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
 * @defgroup    lndw15_sensor Plant Sensor Application
 * @brief       Firmware for the plant sensor demo
 * @{
 *
 * @file
 * @brief       Bootstrap the communication module and the shell
 *
 * @author      Hauke Petersen <mail@haukepetersen.de>
 *
 * @}
 */


#include <stdio.h>

#include "shell.h"

#include "sense.h"
#include "config.h"

/**
 * @brief   Define some shell commands for testing the brain
 */
static const shell_command_t _commands[] = {
    { NULL, NULL, NULL }
};

int main(void)
{
    /* say hello */
    puts("SAFEST Final Demo");

    /* initialize the sensing module */
    sense_run();

    /* run the shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, sizeof(line_buf));

    /* never reached */
    return 0;
}
