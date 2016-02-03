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
 * @fileoverview    This script simulates sensors by sensing out SenML encoded
 *                  CoAP messages to a defined CoAP server and endpoint
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
    {'bn': 'urn:dev:mac:0103fa34d4e5',
     'iv': 1500,
     'devs': [
        {'n': 's:temp', 'u': 'Cel', 'min': 0, 'max': 35, 'last': [0], 'decimals': 2},
        {'n': 's:hum', 'u': '%RH', 'min':25, 'max': 75, 'last': [25], 'decimals': 2}
    ]},
    {'bn': 'urn:dev:mac:affe08155508',
     'iv': 500,
     'devs': [
        {'n': 's:temp', 'u': 'Cel', 'min': 0, 'max': 35, 'last': [0], 'decimals': 2},
        {'n': 's:hum', 'u': '%RH', 'min':25, 'max': 75, 'last': [25], 'decimals': 2}
    ]},
    {'bn': 'urn:dev:mac:3308ae84fea4',
     'iv': 1000,
     'devs': [
        {'n': 's:light', 'u': 'lm', 'min': 80, 'max': 1500, 'last': [80], 'decimals': 0},
    ]},
    {'bn': 'urn:dev:mac:arschibold12',
     'iv': 10000,
     'devs': [
        {'n': 's:temp', 'u': 'Cel', 'min': 50, 'max': 90, 'last': [50], 'decimals': 2},
        {'n': 's:hum', 'u': '%RH', 'min':25, 'max': 75, 'last': [25], 'decimals': 2}
    ]},
    {'bn': 'urn:dev:mac:aed80806cc34',
     'iv': 100,
     'devs': [
        {'n': 's:acc', 'u': 'g', 'min': 0, 'max': 6, 'last': [0, 0, 0], 'decimals': 3},
        {'n': 's:mag', 'u': 'Gs', 'min': 0, 'max': 30, 'last': [0, 0, 0], 'decimals': 3},
        {'n': 's:gyro', 'u': 'dps', 'min': 0, 'max': 2500, 'last': [0, 0, 0], 'decimals': 0}
    ]},
    {'bn': 'urn:dev:mac:fe80beeb1234',
     'iv': 3000,
     'devs': [
        {'n': 'a:rgb', 'u': 'rgb', 'min': 0, 'max': 255, 'last': [0, 0, 0], 'decimals': 0},
    ]}
];

/**
 * Trigger the sending of some sensor data
 */
var update = function(node) {
    var data = [];
    data.push({'bn': node.bn});
    node.devs.forEach(function(dev) {
        var val = [];
        for (var i = 0; i < dev.last.length; i++) {
            val[i] = (Math.random() * (dev.max - dev.min)) + dev.min;
            val[i] = (((3 * dev.last[i]) + val[i]) / 4).toFixed(dev.decimals);
        }
        dev.last = val;
        if (val.length == 1) {
            val = val[0];
        }
        data.push({'n': dev.n, 'u': dev.u, 'v': val});
    });

    var req = coap.request(opts);
    req.on('response', function(res) {
        var s = (res.code == '2.04') ? '[ok]' : '[fail]';
        console.log(node.bn + ':', s);
    });
    req.end(JSON.stringify(data));
}

/**
 * Install periodic update events
 */
var go = function() {
    sensors.forEach(function(node) {
        setInterval(function() {
            update(node);
        }, node.iv);
    });
}

go();
