/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @fileoverview    SAFEST Alarm Sink
 *
 * @author          Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

/**
 * Setup the base configuration
 */
const UDP_PORT =        11311
const MSG_HEAD =        23
const MSG_BUSY =        0
const MSG_FREE =        1

/**
 * Redis configuration
 */
const REDIS_HOST = 'localhost';
const REDIS_PORT = 6379;
const REDIS_CHANNEL = 'door-event-channel';

/**
 * Global variables and needed modules
 */
var seq = 0;
var dgram = require('dgram');
var redis = require('node-redis');
// var redisClient = redis.createClient(REDIS_PORT, REDIS_HOST);


/**
 * React on received actions
 */
function incoming(msg) {
    /* check message format */
    if (msg.length != 3 || msg[0] != MSG_HEAD) {
        return;
    }
    /* check sequence number */
    if ((msg[1] < 10) && (seq > 250)) {
        seq = msg[1];
    } else if (msg[1] <= seq) {
        return;
    } else {
        seq = msg[1];
    }
    /* publish event */
    if (msg[2] == MSG_BUSY) {
        console.log("UDP: GOT BUSY EVENT");
    } else if (msg[2] == MSG_FREE) {
        console.log("UDP: GOT FREE EVENT");
    } else {
        console.log("UDP: GOT BOGUS EVENT");
    }
}

/**
 * Send and publish a report to Redis
 */
function redisSend(eventData) {
    redisClient.hmset(eventData.oid,
        'payload', JSON.stringify(eventData),
        'subject', 'safest',
        'unmarshaller', 'de.fraunhofer.fokus.safest.model.SafestEntityUnmarshaller',
        function() {
            console.log("REDIS:  Put event into database");
    });
    redisClient.publish(REDIS_CHANNEL, eventData.oid);
    //console.log("REDIS:  Data send and published: " + JSON.stringify(eventData));
};

/**
 * Report detected events to the Redis database and tell the visualization about the traffic
 */
function reportToRedis(event, nodeid) {
    var eventData = undefined;
    var time = Math.round((new Date()).getTime() / 1000);
    if (event == "crit") {
        eventData = {
            'type': 'Alarm',
            'oid': 'fence01_' + time,
            'causes': [],
            'description': REDIS_MSG_CRIT,
            'source': 'fnode_023',
            'severity': 'extreme',
            'timestamp': time
        };
    } else if (event == "evt") {
        eventData = {
            'type': 'Alarm',
            'oid': 'fence01_' + time,
            'causes': [],
            'description': REDIS_MSG_CRIT,
            'source': 'fnode_023',
            'severity': 'moderate',
            'timestamp': time
        };
    } else {
        return;
    }
    redisSend(eventData);
    // visualize
    var vis = {"hopsrc": "gw", "hopdst": "redis", "group": "evt", "type": event, "payload": {}, "time": time};
    publish('update', vis);
};

/**
 * Finally: bootstrap this server
 */
sock = dgram.createSocket('udp6');
sock.bind(UDP_PORT, function() {
    console.log('Started UDP server at port', UDP_PORT);

    sock.on('message', function(msg, rinfo) {
        incoming(msg);
        console.log('UDP: received %d bytes from %s:%d\n',
                    msg.length, rinfo.address, rinfo.port);
    });
});
