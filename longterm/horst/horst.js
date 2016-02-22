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
const WEB_PORT      = 12345;
const WEB_DIR       = __dirname + '/web';

const DB_UPDATE_INT = 500;      /* how often to update the node list [in ms] */
const STALE_TIME    = 2000;     /* time until a node gets stale [in ms] */

const DATA_HISTORY  = 50;      /* save this amount of datapoints per device */

const JS_FW         = 5;
const JS_BW         = 2;
const JS_STEER      = 0;
const JS_BTN_DISCO  = 0;
const JS_INTERVAL   = 100;

/**
 * Load Node packages and initialize global variables
 */
var coap            = require('coap');
var coap_server     = coap.createServer({'type': 'udp6'});
var exp_app         = require('express')();
var web_server      = require('http').createServer(exp_app);
var web_sock        = require('socket.io')(web_server);
var fs              = require('fs');
var joystick        = new (require('joystick'))(0, 3500, 350);

/**
 * Also hold the current joystick control data
 */
var js_speed = 0;
var js_steer = 0;
var ctrl_int = undefined;

/**
 * This object holds known and previously known devices
 */
var nodes = {};

var update_behavior = function(bh) {
    if (ctrl_int != undefined) {
        clearInterval(ctrl_int);
    }
    if (bh == '3') {
        console.log("Going into manual control mode");
        ctrl_int = setInterval(send_ctrl, JS_INTERVAL);
    }
}

var db_update_senml = function(data, src_ip){
    /* check data */
    if (!Array.isArray(data) || (data[0].bn == undefined)) {
        throw("invalid SenML input");
    }

    var id = data[0].bn;
    var now = Date.now();

    if (nodes[id] == undefined) {
        nodes[id] = {
            'update': 0,
            'ip': '',
            'devs': {}
        }
    }

    var node = nodes[id];
    node.update = now;
    node.ip = src_ip;

    for (var i = 1; i < data.length; i++) {
        var sendev = data[i];
        if (!(sendev.n in node.devs)) {
            node.devs[sendev.n] = {
                'unit': sendev.u,
                'time': [],
                'vals': []
            }
        }
        var dev = node.devs[sendev.n];
        if (dev.time.length > DATA_HISTORY) {
            dev.time.pop();
            dev.vals.pop();
        }
        dev.time.unshift(now);
        dev.vals.unshift(sendev.v);
    }

    console.log("udpate from", id, '[' + node.ip + ']');
    web_sock.emit('update', {'id': id, 'node': node});
}

/**
 * Define some CoAP helpers
 */
var coap_resp = function(code, res, data) {
    res.statusCode = code;
    res.end(data);
}

/**
 * Definition of CoAP endpoints
 */
var ep_senml = function(req, res) {
    if (req.method != 'POST') {
        coap_resp(405, res);
        return;
    }

    try {
        var data = JSON.parse(req.payload);
        db_update_senml(data, req.rsinfo.address);
        coap_resp(204, res);
    } catch (e) {
        console.log(e);
        coap_resp(406, res);
    }
}

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

var ep_test = function(req, res) {
    console.log('EP_TEST');
    foo = {'test': 123, 'foo': 'bar'};

    res.setOption("Content-Format", "application/json");
    res.end(JSON.stringify(foo));
}

var eps = {
    '/.well-known/core': {
        'cb': ep_wellknown_core,
        'desc': {'title': "Discover me"}
    },
    '/test': {
        'cb': ep_test,
        'desc': {'title': "Some test resource", 'rt': "Test RT"}
    },
    '/senml': {
        'cb': ep_senml,
        'desc': {'title': "Register a node"}
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

var coap_post = function(addr, ep, data) {
    console.log("send something to coap server", addr, ep, data);
    var req = coap.request({'host': addr,
                            'method': 'POST',
                            'pathname': "/" + ep,
                            'confirmable': false});
    req.end(data);
}

/**
 * Setup routes for the web server
 */
exp_app.get('*', function(req, res) {
    var file = WEB_DIR + req.url;
    if (file.match(/(\.js)|(\.css)|(\.png)|(\.otf)$/g) != undefined) {
        try {
            fs.statSync(file);
            res.sendFile(file);
        }
        catch (e) {
            res.sendStatus(404);
        }
    }
    else {
        res.sendFile(WEB_DIR + "/index.html");
    }
});

/**
 * Setup the web socket endpoint
 */
web_sock.on('connection', function(socket) {
    console.log('connection from ', socket.id);

    socket.on('cmd', function(cmd) {
        console.log("got command", cmd);
    });
    socket.on('disconnect', function() {
        console.log('disconnected ', socket.id);
    });
    socket.on('coap_send', function(ctx) {
        if (ctx.ep == 'behave') {
            update_behavior(ctx.val);
        }
        coap_post(ctx.addr, ctx.ep, ctx.val);
    });

    socket.emit('init', nodes);
});

/**
 * Start everything
 */
coap_server.listen(COAP_PORT, function() {
    console.log("CoAP server running at coap://[::1]:" + COAP_PORT);
})
web_server.listen(WEB_PORT, function() {
    console.log("Web server running at http://[::1]:" + WEB_PORT);
});

var send_ctrl = function() {
    var val = new Buffer(4);
    val[1] = (js_speed & 0xff);
    val[0] = ((js_speed >> 8) & 0xff);
    val[3] = (js_steer & 0xff);
    val[2] = ((js_steer >> 8) & 0xff);
    coap_post(nodes[Object.keys(nodes)[0]].ip, 'ctrl', val);
};

joystick.on('axis', function(ctx) {
    if (ctx.init == true) {
        return;
    }
    if (ctx.number == JS_STEER) {
        js_steer = (ctx.value >> 5);
        if (js_steer < 0) {
            js_steer += 10;
        }
        else {
            js_steer -= 10;
        }
        console.log('steer', js_steer);
    }
    if (ctx.number == JS_BW) {
        js_speed = Math.round(-(ctx.value + 32767) / 90);
        console.log('speed', js_speed);
    }
    if (ctx.number == JS_FW) {
        js_speed = Math.round((ctx.value + 32767) / 90);
        console.log('speed', js_speed);
    }
});

joystick.on('button', function(btn) {
    if (btn.init == true) {
        console.log('btn init');
        return;
    }

    if ((btn.number == JS_BTN_DISCO)) {
        console.log("DISCO TIME");
        coap_post(nodes[Object.keys(nodes)[0]].ip, 'disco', '1');
    }
});
