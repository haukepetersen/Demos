/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @fileoverview    SAFEST Web Alarm Application
 *
 * @author          Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

/**
 * Setup the base configuration
 */
const WEB_PORT = 12345;
const ROOT_DIR = __dirname + '/root/';

/**
 * include packages
 */
var express = require('express');
var app = express();
var server = require('http').createServer(app);

var is_alarm = false;

/**
 * Setup static routes for img, js, css and the favicon
 */
app.use('/img', express.static(ROOT_DIR));
app.use('/js', express.static(ROOT_DIR));
app.use('/css', express.static(ROOT_DIR));

/**
 * Setup Dynamic endpoints, depending on the internal state
 */
app.get('/*', function(req, res) {
    if (req.url == '/debug') {
        res.sendFile(ROOT_DIR + 'debug.html');
    }
    else if (is_alarm) {
        res.sendFile(ROOT_DIR + 'assist.html');
    }
    else {
        res.sendFile(ROOT_DIR + 'landing.html');
    }
});

/**
 * @brief Process POST requests and set internal state accordingly
 */
app.post('/*', function(req, res) {
    if (req.url == '/alarm') {
        is_alarm = true;
        console.log("ALRAM");
    }
    else if (req.url == '/clear') {
        is_alarm = false;
        console.log("CLEAR");
    }
});

/**
 * Bootstrap and start the application
 */
server.listen(WEB_PORT, function() {
    console.info('WEBSERVER: Running at http://127.0.0.1:' + WEB_PORT + '/');
});

