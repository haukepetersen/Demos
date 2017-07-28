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
 * @fileoverview    Horst manages and controls RIOT based 6LoWPAN devices and
 *                  brings some user experience to otherwise lame demos
 *
 * @author          Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

/**
 * Setup the base configuration
 */
const COAP_PORT     = 5683;

const RES_SENSE     = "msa/sense";
const RES_STATUS    = "msa/status";

const STATUS_T      = 30000;

/**
 * Load Node packages and initialize global variables
 */
var coap            = require('coap');
var coap_server     = coap.createServer({'type': 'udp6'});

/**
 * This object holds known and previously known devices
 */
var nodes = [];

/**
 * Define some CoAP helpers
 */
var coap_resp = function(code, res, data) {
    res.statusCode = code;
    res.end(data);
};

var dump_data = function(head, hi, data)
{
    console.log(head, "from [" + hi.address + "]:" + hi.port + " - ",
                JSON.parse(data));
}

var on_sense_update = function(res) {
    dump_data("  data", res.rsinfo, res.payload);
    res.on('data', function(foo) {
        dump_data("  data", res.rsinfo, res.payload);
    });
};

var on_status_report = function(res) {
    dump_data("status", res.rsinfo, res.payload);
};

var status_report = function() {
    for (var i = 0; i < nodes.length; i++) {
        req = coap.request({
            'host': nodes[i].address,
            'port': nodes[i].port,
            'confirmable': false,
            'method': 'GET',
            'pathname': RES_STATUS
        });
        req.on('response', on_status_report);
        req.end();
    }
};

var observe = function(host, resource, cb) {
    req = coap.request({
        'host': host.address,
        'port': host.port,
        'confirmable': false,
        'method': 'GET',
        'observe': true,
        'pathname': resource
    });
    req.on('response', cb);
    req.end();
};

/**
 * Definition of CoAP endpoints
 */
var ep_wellknown_core = function(req, res) {
    console.log('EP_WELLKNOWN_CORE');

    var links = "";

    for (ep in eps) {
        links += "<" + ep + ">";
        for (l in eps[ep].desc) {
            links += ';' + l + '="' + eps[ep]['desc'][l] + '"';
        }
        links += ",";
    }

    res.setOption("Content-Format", "application/link-format");
    res.end(links);
}

var ep_reg = function(req, res) {
    console.log('EP_REG');

    if (req.method != 'POST') {
        coap_resp(404, res);
        return;
    }

    var found = false;

    for (var i = 0; i < nodes.length; i++) {
        if (nodes[i].address == req.rsinfo.address && nodes[i].port == req.rsinfo.port) {
            found = true;
            break;
        }
    }

    if (!found) {
        nodes.push({ 'address': req.rsinfo.address, 'port': req.rsinfo.port });
    }

    observe(req.rsinfo, RES_SENSE, on_sense_update);

    res.setOption("Content-Format", "text/plain");
    res.end("ok");
}

var ep_info = function(req, res) {
    console.log('EP_TEST');

    res.setOption("Content-Format", "application/json");
    res.end(JSON.stringify(nodes));
}

var eps = {
    '/.well-known/core': {
        'cb': ep_wellknown_core,
        'desc': {'title': "Discover me"}
    },
    '/msa/reg': {
        'cb': ep_reg,
        'desc': {'title': "Register a node"}
    },
    '/msa/info': {
        'cb': ep_info,
        'desc': {'title': "Get server infos"}
    }
};

/**
 * Setup CoAP server
 */
coap_server.on('request', function(req, res) {
    if (req.url in eps) {
        eps[req.url].cb(req, res);
    }
    else {
        coap_resp(404, res);
    }
});

/**
 * Start everything
 */
coap_server.listen(COAP_PORT, function() {
    console.log("CoAP server running at coap://[::1]:" + COAP_PORT);
})
setInterval(function() {
    status_report();
}, STATUS_T);
