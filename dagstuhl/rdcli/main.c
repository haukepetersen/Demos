/*
 * Copyright (C) 2017 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       CoAP RD simple registration test application
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "fmt.h"
#include "shell.h"
#include "mutex.h"
#include "saul_reg.h"
#include "net/ipv6/addr.h"
#include "net/gcoap.h"
#include "net/rdcli_common.h"

#define INFO_MAXLEN        (32U)

/* stack configuration */
#define STACKSIZE           (THREAD_STACKSIZE_DEFAULT)
#define PRIO                (THREAD_PRIORITY_MAIN - 2)
#define TNAME               "observe_server"

static char stack[STACKSIZE];

static char info[INFO_MAXLEN];

static mutex_t obs_lock = MUTEX_INIT;

static saul_reg_t *btn;
static saul_reg_t *led;

static uint8_t buf[200];

/* define some dummy CoAP resources */
static ssize_t handler_info(coap_pkt_t *pdu, uint8_t *buf, size_t len)
{
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    size_t slen = strlen(info);
    memcpy(pdu->payload, info, slen);
    return gcoap_finish(pdu, slen, COAP_FORMAT_TEXT);
}

static ssize_t handler_led(coap_pkt_t *pdu, uint8_t *buf, size_t len)
{
    unsigned resp = COAP_CODE_NOT_ACCEPTABLE;
    ssize_t resp_len = 0;
    uint8_t code = pdu->hdr->code;
    char status;

    phydat_t data = { { 0, 0, 0 }, UNIT_BOOL, 0 };

    if (code == COAP_METHOD_PUT) {

        if (pdu->payload_len > 0) {
            if (pdu->payload[0] == '0') {
                data.val[0] = 0;
            } else  {
                data.val[0] = 1;
            }
            saul_reg_write(led, &data);
            resp = COAP_CODE_CHANGED;
        }
    }
    else if (code == COAP_METHOD_GET) {
        saul_reg_read(led, &data);
        status = (data.val[0]) ? '1' : '0';
        resp_len = 1;
        resp = COAP_CODE_CONTENT;
    }


    gcoap_resp_init(pdu, buf, len, resp);
    if (resp_len == 1) {
        pdu->payload[0] = status;
    }
    return gcoap_finish(pdu, resp_len, COAP_FORMAT_TEXT);
}

static ssize_t handler_btn(coap_pkt_t *pdu, uint8_t *buf, size_t len)
{
    phydat_t data;
    saul_reg_read(btn, &data);

    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    if (data.val[0]) {
        pdu->payload[0] = '0';
    }
    else {
        pdu->payload[0] = '1';
    }
    return gcoap_finish(pdu, 1, COAP_FORMAT_TEXT);
}

static const coap_resource_t resources[] = {
    { "/act/led",  (COAP_GET | COAP_PUT), handler_led },
    { "/node/info",  COAP_GET, handler_info },
    { "/sense/btn", COAP_GET, handler_btn }
};

static gcoap_listener_t listener = {
    .resources     = (coap_resource_t *)&resources[0],
    .resources_len = sizeof(resources) / sizeof(resources[0]),
    .next          = NULL
};

static int obsup(unsigned num, void *payl, size_t len)
{
    coap_pkt_t pdu;

    mutex_lock(&obs_lock);
    if (gcoap_obs_init(&pdu, buf, sizeof(buf), &resources[num]) != GCOAP_OBS_INIT_OK) {
        mutex_unlock(&obs_lock);
        return -1;
    }
    memcpy(pdu.payload, payl, len);
    ssize_t plen = gcoap_finish(&pdu, len, COAP_FORMAT_TEXT);
    gcoap_obs_send(buf, plen, &resources[num]);
    mutex_unlock(&obs_lock);

    return 0;
}

static void *observe_trigger(void *arg)
{
    (void)arg;

    phydat_t dat;
    unsigned who = 0;

    int printed = 0;

    while (1) {

        /* observe LED status */
        if ((who & 0x1) == 0) {
            saul_reg_read(led, &dat);
            char p = (dat.val[0]) ? '1' : '0';
            if (printed == 0) {
                printf("observe LED (status %c)", p);
            }
            if (obsup(0, &p, 1) < 0 && printed == 0) {
                printf(" --- no one is interested");
            }
        }

        /* observe BTN status */
        if (who & 0x1) {
            saul_reg_read(btn, &dat);
            char p = (dat.val[0]) ? '1' : '0';
            if (printed == 0) {
                printf("observe BTN (status %c)", p);
            }
            if (obsup(2, &p, 1) < 0 && printed == 0) {
                printf(" --- no one is interested");
            }
        }

        if (printed == 0) {
            puts("");
        }

        /* just for chance, send node info */
        if (who & 0x08) {
            who &= ~0x8;
            if (printed == 0) {
                printf("observe INFO           ");
            }
            if (obsup(1, info, strlen(info)) < 0 && printed == 0) {
                printf(" --- no one is interested");
            }
            if (printed == 0) {
                puts("");
            }
            printed = 1;
        }

        who += 1;
        xtimer_sleep(1);
    };

    return NULL;    /* never reached */
}

static int cmd_set_info(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s <info text>\n", argv[0]);
        return 1;
    }

    size_t info_len = strlen(argv[1]);

    if (info_len >= INFO_MAXLEN) {
        puts("error: info string too long");
        return 1;
    }
    strcpy(info, argv[1]);

    /* see if someone observes this info */
    puts("obs for info");
    if (obsup(1, info, info_len) < 0) {
        puts("no one interested in info updates...");
    }

    return 0;
}

static const shell_command_t shell_commands[] = {
    { "set_info", "change info text", cmd_set_info },
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("Hello Dagstuhl!\n");

    strcpy(info, rdcli_common_get_ep());
    printf("info is %s\n", info);

    led = saul_reg_find_type(SAUL_ACT_SWITCH);
    btn = saul_reg_find_type(SAUL_SENSE_BTN);

    assert(led && btn);

    gcoap_register_listener(&listener);

    thread_create(stack, sizeof(stack), PRIO, 0, observe_trigger, NULL, TNAME);

    puts("Client information:");
    printf("  ep: %s\n", rdcli_common_get_ep());
    printf("  lt: %is\n", (int)RDCLI_LT);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
