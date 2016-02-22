/*
 * Copyright (C) 2016 Freie Universität Berlin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

/**
 * @fileoverview    Simulating the RIOT car
 *
 * @author          Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

/**
 * Setup the base configuration
 */
const SERVER_PORT     = 5683;
// const SERVER_IP     = 'fd38:3734:ad48:0:c260:e5f1:5ac2:53ef';
const SERVER_IP     = '::1';
const SERVER_EP     = '/senml';

/**
 * Load Node packages and initialize global variables
 */
var coap            = require('coap');
var opts = {
    'host': SERVER_IP,
    'port': SERVER_PORT,
    'pathname': SERVER_EP,
    'method': 'POST'
}

/**
 * Define some dummy sensor nodes
 */
var sensors = [
    {'bn': 'urn:dev:riotcar',
     'iv': 250,
     'devs': [
        {'n': 's:car', 'u': 'mixed'},
        {'n': 'a:car', 'u': 'mixed'}
    ]}
];

var state = [200, 0, 0, 0];
var behavior = 0;

/**
 * Trigger the sending of some sensor data
 */
var publish = function() {

    var node = sensors[0];
    var data = [];
    data.push({'bn': node.bn});
    node.devs[0]['v'] = state;
    node.devs[1]['v'] = behavior;
    data.push(node.devs[0]);
    data.push(node.devs[1]);
    console.log(JSON.stringify(data));

    var req = coap.request(opts);
    req.on('response', function(res) {
        var s = (res.code == '2.04') ? '[ok]' : '[fail]';
        console.log(node.bn + ':', s);
    });
    req.end(JSON.stringify(data));
}

var hitSomething = function() {
    if (state[0] <= 0) {
        state[0] = 200;
        state[1] = 0;
        state[2] = 1;
    }
    else {
        state[0] = -200;
        state[1] = 1000;
        state[3] = 1;
    }
    setTimeout(clear, 1500);
    setTimeout(hitSomething, (Math.random() * 5000) + 3000);
}

var clear = function() {
    state[2] = 0;
    state[3] = 0;
}

var behave = function() {
    behavior = ((++behavior) > 3) ? 0 : behavior;
}

/**
 * Install periodic update events
 */
setInterval(publish, 250);
setTimeout(hitSomething, 1500);
setInterval(behave, 2000);
