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
 * @brief       Scala
 *
 * @author      Hauke Petersen <mail@haukepetersen.de>
 * @author      Cenk Gündoğan <cnkgndgn@gmail.com>
 *
 * @}
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "shell.h"
#include "board.h"

#include "brain.h"
#include "demo_config.h"

#define SHELL_BUFSIZE       (64U)

static int _readc(void)
{
    return (int)getchar();
}

static void _putc(int c)
{
    putchar((char)c);
}
static const shell_command_t _commands[] = {
    { NULL, NULL, NULL }
};

int main(void)
{
    shell_t shell;
    LED_RED_OFF;

    /* initialize (and run) the brain */
    puts("initializing the brain");
    brain_init();

    /* run the shell for debugging purposes */
    puts("running the shell");
    shell_init(&shell, _commands, SHELL_BUFSIZE, _readc, _putc);
    shell_run(&shell);

    /* should never be reached */
    return 0;
}
