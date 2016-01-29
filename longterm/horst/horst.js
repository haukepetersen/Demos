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

/**
 * Load Node packages and initialize global variables
 */
var coap            = require('coap');
var coap_server     = coap.createServer();
var express         = require('express');
var exp_app         = express();
var web_server      = require('http').createServer(exp_app);
var web_io          = require('socket.io').listen(web_server);
var fs              = require('fs');

/**
 * This object holds known and previously known devices
 */
var devdb = {
    'active': {},
    'stale': {}
}

var db_add = function(entry) {
    if (entry.id in devdb['active']) {
        delete devdb['active'][entry.id];
    }
    devdb['active'][entry.id] = {'has': entry.has, 'update': new Date().getTime()};
    if (entry.id in devdb['stale']) {
        delete devdb['stale'][entry.id];
    }

    console.log(devdb);
}

var db_update = function() {
    var time = new Date().getTime();
    for (node in devdb['active']) {
        var n = devdb['active'][node];
        if ((time - n.update) > STALE_TIME) {
            devdb['stale'][node] = n;
            delete devdb['active'][node];

            console.log(devdb);
        }
    }
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
var ep_reg = function(req, res) {
    if (req.method != 'POST') {
        coap_resp(405, res);
        return;
    }

    var data = JSON.parse(req.payload);
    if (!data.id || !data.has) {
        coap_resp(406, res);
        return;
    }
    db_add(data);
    coap_resp(204, res);
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
        'methods': ['GET'],
        'desc': {'title': "Discover me"}
    },
    '/test': {
        'cb': ep_test,
        'methods': ['GET'],
        'desc': {'title': "Some test resource", 'rt': "Test RT"}
    },
    '/reg': {
        'cb': ep_reg,
        'methods': ['POST'],
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


/**
 * Setup routes for the web server
 */
exp_app.get('*', function(req, res) {
    var file = WEB_DIR + req.url;
    if (file.match(/(\.js)|(\.css)|(\.png)$/g) != undefined) {
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
 * Start everything
 */
coap_server.listen(COAP_PORT, function() {
    console.log("CoAP server running at coap://[::]:" + COAP_PORT);
})
web_server.listen(WEB_PORT, function() {
    console.log("Web server running at http://[::]:" + WEB_PORT);
});
setInterval(db_update, DB_UPDATE_INT);
