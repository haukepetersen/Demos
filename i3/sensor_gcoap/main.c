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
#include "shell.h"
#include "xtimer.h"
#include "saul_reg.h"
#include "periph/gpio.h"

#include "net/gcoap.h"
#include "net/ipv6/addr.h"
#include "net/sock/udp.h"

#include "encode.h"

#define CLOUD_ADDR          ("affe::1")
#define CLOUD_PORT          (5683U)

#ifndef NODE_ID
#define NODE_ID             ("p.schmerzl")
#endif

#define RES_REG             ("/msa/reg")
#define RES_SENSE           ("/msa/sense")
#define RES_STATUS          ("/msa/status")

#define TIMEOUT_SAMPLE      (1 * US_PER_SEC)
#define TIMEOUT_REGISTER    (5 * US_PER_SEC)

#define JSON_BUFISZE        (100U)

#define SENSE_PRIO          (THREAD_PRIORITY_MAIN - 1)

#define MAIN_QUEUE_SIZE     (8)

static msg_t main_msg_queue[MAIN_QUEUE_SIZE];

static char json_buf[JSON_BUFISZE];
static uint8_t coap_buf[GCOAP_PDU_BUF_SIZE];
static uint8_t obs_buf[GCOAP_PDU_BUF_SIZE];

static char stack[THREAD_STACKSIZE_DEFAULT];

static saul_reg_t *light;

static size_t read_and_encode(char *buf)
{
    phydat_t data;
    size_t pos;

    saul_reg_read(light, &data);
    pos = encode_json(buf, &data, 3);
    buf[pos] = '\0';

    return pos;
}

static void reg_resp_handler(unsigned req_state, coap_pkt_t* pdu)
{
    (void)req_state;
    (void)pdu;

    if (req_state == GCOAP_MEMO_TIMEOUT) {
        printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        return;
    }
    else if (req_state == GCOAP_MEMO_ERR) {
        printf("gcoap: error in response\n");
        return;
    }

    printf("got response for CoAP POST request to %s\n", RES_REG);
}

static int cmd_reg(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /* parse cloud address */
    sock_udp_ep_t cloud = SOCK_IPV6_EP_ANY;
    ipv6_addr_from_str((ipv6_addr_t *)cloud.addr.ipv6, CLOUD_ADDR);
    cloud.port = (uint16_t)CLOUD_PORT;

    /* build register packet */
    coap_pkt_t pdu;
    // ssize_t n = gcoap_request(&pdu, coap_buf, COAP_BUFSIZE, COAP_POST, RES_REG);
    int n = gcoap_req_init(&pdu, coap_buf, GCOAP_PDU_BUF_SIZE, GCOAP_POST, RES_REG);
    if (n < 0) {
        printf("gcoap_req_init failed: %i\n", (int)n);
        return 1;
    }
    const char *lala = "moin frosch, moin Hase, moin Igel!";
    memcpy(pdu.payload, lala, strlen(lala));
    n = gcoap_finish(&pdu, strlen(lala), COAP_FORMAT_TEXT);
    if (n < 0) {
        printf("gcoap_finish failed: %i\n", n);
        return 1;
    }

    gcoap_req_send2(coap_buf, n, &cloud, reg_resp_handler);

    printf("register node with CoAP server [%s]:%u\n", CLOUD_ADDR, CLOUD_PORT);

    return 0;
}

/* CoAP resource handler */
static ssize_t res_sense_handle(coap_pkt_t* pdu, uint8_t *buf, size_t len)
{
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    size_t plen = read_and_encode((char *)pdu->payload);
    return gcoap_finish(pdu, plen, COAP_CT_JSON);
}

static ssize_t res_status_handle(coap_pkt_t* pdu, uint8_t *buf, size_t len)
{
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    size_t plen = encode_status((char *)pdu->payload);
    ((char *)pdu->payload)[plen] = '\0';

    printf("status: %s (plen is %i)\n", (char *)pdu->payload, (int)plen);

    return gcoap_finish(pdu, plen, COAP_CT_JSON);
}

/* CoAP resources */
static const coap_resource_t resources[] = {
    { RES_SENSE, COAP_GET, res_sense_handle },
    { RES_STATUS, COAP_GET, res_status_handle },
};

static gcoap_listener_t listener = {
    (coap_resource_t *)&resources[0],
    sizeof(resources) / sizeof(resources[0]),
    NULL
};

/* shell commands */
static const shell_command_t shell_commands[] = {
    { "reg", "register with cloud", cmd_reg },
    { NULL, NULL, NULL }
};

static void *sense(void *arg)
{
    (void)arg;

    while (1) {
        xtimer_usleep(TIMEOUT_SAMPLE);

        size_t len = read_and_encode(json_buf);
        printf("sense: new data - %s\n", json_buf);

        /* publish data in case somebody is observing RES_SENSE */
        coap_pkt_t pdu;
        int res = gcoap_obs_init(&pdu, obs_buf, GCOAP_PDU_BUF_SIZE, &resources[0]);
        if (res == GCOAP_OBS_INIT_OK) {
            memcpy(pdu.payload, json_buf, len);
            size_t plen = gcoap_finish(&pdu, len, COAP_CT_JSON);
            gcoap_obs_send(obs_buf, plen, &resources[0]);
            printf("sense: data send via CoAP\n");
        }
        else {
            xtimer_usleep(TIMEOUT_REGISTER);
            printf("sense: no one is observing, sending register message\n");
            cmd_reg(0, NULL);
        }

    }

    return NULL;
}

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(main_msg_queue, MAIN_QUEUE_SIZE);

    /* get the light sensor */
    light = saul_reg_find_type(SAUL_SENSE_COLOR);

    /* init the encoder */
    encode_init();

    /* register coap resources */
    gcoap_register_listener(&listener);

    /* start the sensor thread */
    thread_create(stack, sizeof(stack), SENSE_PRIO, 0, sense, NULL, "sense");

    /* run the shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
