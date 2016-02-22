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
 * @fileoverview    Client side logic for Horst
 *
 * @author          Hauke Petersen <hauke.petersen@fu-berlin.de>
 */


/**
 * Define available views
 */
const VIEW_S_TEMP   = 's:temp';
const VIEW_S_HUM    = 's:hum';
const VIEW_S_LIGHT  = 's:light';
const VIEW_S_RGB    = 's:rgb';
const VIEW_S_ACC    = 's:acc';
const VIEW_S_MAG    = 's:mag';
const VIEW_S_GYRO   = 's:gyro';
const VIEW_A_RGB    = 'a:rgb';
const VIEW_A_LED    = 'a:led';
const VIEW_A_BUTTON = 'a:button';

const STALE_TIME    = 5000;     /* node gets stale if no updated in 5 seconds */

/**
 * Define some global variables
 */
var socket = io();
var nodes = {};
var charts = {};
var active_node = undefined;

var last_speed = 0;

var chart_opts = {
    's:temp':   {'maxValue': 40, 'minValue': 20, 'millisPerPixel': 30},
    's:hum':    {'maxValue': 100, 'minValue': 0, 'millisPerPixel': 30},
    's:light':  {'maxValue': 1500, 'minValue': 0, 'millisPerPixel': 30},
    's:rgb':    {'maxValue': 256, 'minValue': 0, 'millisPerPixel': 30},
    's:acc':    {'maxValue': 4000, 'minValue': -4000, 'millisPerPixel': 15},
    's:mag':    {'maxValue': 500, 'minValue': -500, 'millisPerPixel': 15},
    's:gyro':   {'maxValue': 2500, 'minValue': -2500, 'millisPerPixel': 15},
    'a:rgb':    {'maxValue': 256, 'minValue': 0, 'millisPerPixel': 30},
    'a:led':    {'maxValue': 1, 'minValue': 0, 'millisPerPixel': 30},
    'a:button': {'maxValue': 1, 'minValue': 0, 'millisPerPixel': 30},
};

var line_colors = [
    '#3fa687',
    '#bc1a29',
    '#aaaaaa'
];

var val_toString = function(vals) {
    var res = '';
    if (Array.isArray(vals)) {
        res = '[';
        vals.forEach(function(v, i) {
            var comma = (i == 0) ? '' : ', ';
            res += comma + v;
        })
        res += ']';
    }
    else {
        res = vals;
    }
    return res;
};

var snip_infobox = function(id, node) {
    var c = '';
    c +=    '<div class="row">';
    c +=      '<span class="k">ID:</span>';
    c +=      '<span class="v">' + id + '</span>';
    c +=    '</div>';
    c +=    '<div class="row">';
    c +=      '<span class="k">IP:</span>';
    c +=      '<span class="v">' + node.ip + '</span>';
    c +=    '</div>';
    c +=    '<div class="row">';
    c +=      '<span class="k">HAS:</span>';
    Object.keys(node.devs).forEach(function(k, i) {
        var comma = (i == 0) ? '' : ', ';
        c += '<span class="v">' + comma + k + '</span>';
    });
    c +=    '</div>';

    return c;
};

var snip_a_car = function(parent, k, dev) {
    var time = new Date(dev.time[0]).toLocaleTimeString();
    var vals = val_toString(dev.vals[0]);

    var c = '';
    c +=    '<div class="row">';
    c +=      '<div class="col">';
    c +=        '<span class="k">Type:</span>';
    c +=        '<span class="v">' + k + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Unit:</span>';
    c +=        '<span class="v">' + dev.unit + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Last Update:</span>';
    c +=        '<span id="' + k.replace(':', '-') + 'update" class="v">' + time + '</span>';
    c +=      '</div>';
    c +=    '</div>';
    c +=    '<div id="behave_header" class="row">Programmed behavior:</div>';

    c +=    '<div class="row">';
    for (var i = 0; i < 4; i++) {
        var ent = (i == dev.vals[0]) ? 'act' : 'pass';
        c +=    '<div id="acar_b' + i + '" class="carbtn ' + ent + '"></div>';
    }
    c +=    '</div>';
    c +=    '<div class="row"></div>';
    parent.html(c);

    d3.select("#acar_b0").on('click', function() {
        socket.emit('coap_send', {'addr': nodes[active_node].ip, 'ep': 'behave', 'val': '0'});
    });
    d3.select("#acar_b1").on('click', function() {
        socket.emit('coap_send', {'addr': nodes[active_node].ip, 'ep': 'behave', 'val': '1'});
    });
    d3.select("#acar_b2").on('click', function() {
        socket.emit('coap_send', {'addr': nodes[active_node].ip, 'ep': 'behave', 'val': '2'});
    });
    d3.select("#acar_b3").on('click', function() {
        socket.emit('coap_send', {'addr': nodes[active_node].ip, 'ep': 'behave', 'val': '3'});
    });
};

var snip_s_car = function(parent, k, dev) {
    var time = new Date(dev.time[0]).toLocaleTimeString();
    var vals = val_toString(dev.vals[0]);

    var c = '';
    c +=    '<div class="row">';
    c +=      '<div class="col">';
    c +=        '<span class="k">Type:</span>';
    c +=        '<span class="v">' + k + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Unit:</span>';
    c +=        '<span class="v">' + dev.unit + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Last Update:</span>';
    c +=        '<span id="' + k.replace(':', '-') + 'update" class="v">' + time + '</span>';
    c +=      '</div>';
    c +=    '</div>';
    c +=    '<div class="row">';
    c +=    '  <div id="scar_speed" class="stop"></div>';
    c +=    '  <div id="scar_block">';
    c +=    '    <div id="scar_block_f" class="free"></div>';
    c +=    '    <div id="scar_block_icon"></div>';
    c +=    '    <div id="scar_block_b" class="blocked"></div>';
    c +=    '  </div>';
    c +=    '</div>';
    c +=    '<div class="row"></div>';
    parent.html(c);
};

var snip_a_led = function(parent, k, dev) {
    var time = new Date(dev.time[0]).toLocaleTimeString();
    var vals = val_toString(dev.vals[0]);

    var c = '';
    c +=    '<div class="row">';
    c +=      '<div class="col">';
    c +=        '<span class="k">Type:</span>';
    c +=        '<span class="v">' + k + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Unit:</span>';
    c +=        '<span class="v">' + dev.unit + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Last Update:</span>';
    c +=        '<span id="' + k.replace(':', '-') + 'update" class="v">' + time + '</span>';
    c +=      '</div>';
    c +=    '</div>';
    c +=    '<div class="row">';
    c +=      '<span class="k">Value(s):</span>';
    c +=      '<span id="' + k.replace(':', '-') + 'val" class="v">' + vals + '</span>';
    c +=    '</div>';
    c +=    '<div class="row">';
    c +=        '<input type="button" value="on" id="' + k.replace(':', '-') + '_set_on" />';
    c +=        '<input type="button" value="off" id="' + k.replace(':', '-') + '_set_off" />';
    c +=    '</div>';

    parent.html(c);
    d3.select("#" + k.replace(':', '-') + "_set_on").on('click', function() {
        socket.emit('coap_send', {'addr': nodes[active_node].ip, 'ep': 'led', 'val': '1'});
    });
    d3.select("#" + k.replace(':', '-') + "_set_off").on('click', function() {
        socket.emit('coap_send', {'addr': nodes[active_node].ip, 'ep': 'led', 'val': '0'});
    });
}

var snip_a_rgb = function(parent, k, dev) {
    var time = new Date(dev.time[0]).toLocaleTimeString();
    var vals = val_toString(dev.vals[0]);

    var c = '';
    c +=    '<div class="row">';
    c +=      '<div class="col">';
    c +=        '<span class="k">Type:</span>';
    c +=        '<span class="v">' + k + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Unit:</span>';
    c +=        '<span class="v">' + dev.unit + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Last Update:</span>';
    c +=        '<span id="' + k.replace(':', '-') + 'update" class="v">' + time + '</span>';
    c +=      '</div>';
    c +=    '</div>';
    c +=    '<div class="row">';
    c +=      '<span class="k">Value(s):</span>';
    c +=      '<span id="' + k.replace(':', '-') + 'val" class="v">' + vals + '</span>';
    c +=    '</div>';
    c +=    '<div class="row">';
    c +=        '<input id="' + k.replace(':', '-') + '_rgb" value="' + vals + '">';
    c +=    '</div>';

    parent.html(c);

    new jscolor(document.getElementById(k.replace(':', '-') + '_rgb'), {})
    d3.select("#" + k.replace(':', '-') + "_rgb").on('change', function() {
        socket.emit('coap_send', {'addr': nodes[active_node].ip, 'ep': 'rgb',
                    'val': '#' + this.value});
    });
}

var snip_graph = function(parent, k, dev) {
    var time = new Date(dev.time[0]).toLocaleTimeString();
    var vals = val_toString(dev.vals[0]);

    var c = '';
    c +=    '<div class="row">';
    c +=      '<div class="col">';
    c +=        '<span class="k">Type:</span>';
    c +=        '<span class="v">' + k + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Unit:</span>';
    c +=        '<span class="v">' + dev.unit + '</span>';
    c +=      '</div><div class="col">';
    c +=        '<span class="k">Last Update:</span>';
    c +=        '<span id="' + k.replace(':', '-') + 'update" class="v">' + time + '</span>';
    c +=      '</div>';
    c +=    '</div>';
    c +=    '<div class="row">';
    c +=      '<span class="k">Value(s):</span>';
    c +=      '<span id="' + k.replace(':', '-') + 'val" class="v">' + vals + '</span>';
    c +=    '</div>';
    // c +=    '<div class="row">';
    // c +=      '<div id="' + k.replace(':', '-') + 'chart"></div>';
    c +=        '<canvas id="' + k.replace(':', '-') + 'chart" width="460" height="200"></canvas>'
    // c +=    '</div>';

    parent.html(c);
};

var update_list_view = function() {
    var now = Date.now();
    var count = [0, 0];

    /* clear lists */
    var act = d3.select('#active');
    var sta = d3.select('#stale');
    act.selectAll('div').remove();
    sta.selectAll('div').remove();


    Object.keys(nodes).sort().forEach(function(k) {
        var node = nodes[k];

        if ((now - node.update) < STALE_TIME) {
            add_list_item(k, node, act);
            ++count[0];
        }
        else {
            add_list_item(k, node, sta);
            ++count[1];
        }
    });

    if (count[0] == 0) {
        act.append('div').attr('class', 'node')
            .html('No active nodes available');
    }
    if (count[1] == 0) {
        sta.append('div').attr('class', 'node')
            .html('No stale nodes in list');
    }

}

var add_list_item = function(id, node, list) {
    var inner =     '<div class="row">';
    inner +=        '  <span class="k">ID:</span>';
    inner +=        '  <span class="v">' + id + '</span>';
    inner +=        '</div>';
    inner +=        '<div class="row">';
    inner +=        '  <span class="k">IP:</span>';
    inner +=        '  <span class="v">[' + node.ip + ']</span>';
    inner +=        '</div>';

    var n = list.append("div").attr('class', 'node').html(inner);
    n.on('click', function() {
        display_node(id, node);
    });
}

var display_node = function(id, node) {
    var nodeview = d3.select("#nodeview");
    nodeview.selectAll(".snip").remove();

    nodeview.append('div')
        .attr('class', 'snip info')
        .html(snip_infobox(id, node));

    active_node = id;

    Object.keys(node.devs).forEach(function(k) {
        var view = nodeview.append('div').attr('class', 'snip');

        if (k == 'a:led') {
            snip_a_led(view, k, node.devs[k]);
        }
        else if (k == 'a:car') {
            snip_a_car(view, k, node.devs[k]);
        }
        else if (k == 'a:rgb') {
            snip_a_rgb(view, k, node.devs[k]);
        }
        else if (k == 's:car') {
            snip_s_car(view, k, node.devs[k]);
        }
        else {
            snip_graph(view, k, node.devs[k]);
            add_chart(k, node.devs[k]);
        }
    });
}

var update_node_view = function(id, node) {
    if (active_node != id) {
        return;
    }

    for (var k in node.devs) {
        var dev = node.devs[k];
        var time = new Date(dev.time[0]).toLocaleTimeString();
        var vals = val_toString(dev.vals[0]);
        d3.select("#" + k.replace(':', '-') + 'update').html(time);
        d3.select("#" + k.replace(':', '-') + 'val').html(vals);

        if ((k == 'a:led') || (k == 'a:rgb')) {
            /* nothing to do yet */
        }
        else if (k == 'a:car') {
            update_acar(k, dev);
        }
        else if (k == 's:car') {
            update_scar(k, dev);
        }
        else {
            update_chart(k, dev);
        }
    }
}

var add_chart = function(k, dev) {
    var opts = chart_opts[k];
    if (opts == undefined) {
        opts = {};
    }

    charts[k] = {
        'chart': new SmoothieChart(opts),
        'data': []
    };
    charts[k].chart.streamTo(document.getElementById(k.replace(':', '-') + 'chart'));

    if (Array.isArray(dev.vals[0])) {
        for (var i = 0; i < dev.vals[0].length; i++) {
            charts[k].data[i] = new TimeSeries();
            charts[k].chart.addTimeSeries(charts[k].data[i], {'strokeStyle': line_colors[i]});
            for (var s = dev.time.length - 1; s >= 0; s--) {
                charts[k].data[i].append(dev.time[s], dev.vals[s][i]);
            }
        }
    }
    else {
        charts[k].data[0] = new TimeSeries();
        charts[k].chart.addTimeSeries(charts[k].data[0], {'strokeStyle': line_colors[0]});
        for (var s = dev.time.length - 1; s >= 0; s--) {
            charts[k].data[0].append(dev.time[s], dev.vals[s]);
        }
    }
}

var update_acar = function(k, dev) {
    for (var i = 0; i < 4; i++) {
        var id = '#acar_b' + i;
        if (dev.vals[0] == i) {
            d3.select(id).attr('class', "carbtn act");
        }
        else {
            d3.select(id).attr('class', "carbtn pass");
        }
    }
    d3.select("#acar")
}

var update_scar = function(k, dev) {
    if (dev.vals[0][0] == 0) {
        d3.select("#scar_speed").attr("class", "stop");
    }
    else if (dev.vals[0][0] < 0) {
        d3.select("#scar_speed").attr("class", "bw");
    }
    else {
        d3.select("#scar_speed").attr("class", "fw");
    }

    if (dev.vals[0][2] == 1) {
        d3.select("#scar_block_f").attr("class", "blocked");
    }
    else {
        d3.select("#scar_block_f").attr("class", "free");
    }
    if (dev.vals[0][3] == 1) {
        d3.select("#scar_block_b").attr("class", "blocked");
    }
    else {
        d3.select("#scar_block_b").attr("class", "free");
    }

    if (last_speed != dev.vals[0][0]) {
        last_speed = dev.vals[0][0];
        socket.emit('coap_send', {'addr': nodes[active_node].ip, 'ep': 'disco', 'val': '1'});
        console.log("disco");
    }
}

var update_chart = function(k, dev) {
    if (Array.isArray(dev.vals[0])) {
        for (var i = 0; i < dev.vals[0].length; i++) {
            charts[k].data[i].append(dev.time[0], dev.vals[0][i]);
        }
    }
    else {
        charts[k].data[0].append(dev.time[0], dev.vals[0]);
    }
};

/**
 * Configure web socket endpoints
 */
socket.on('init', function(data) {
    nodes = data;
    update_list_view();
});

socket.on('update', function(data) {
    var new_node = !(data.id in nodes);

    /* hack: update time stamps */
    var now = Date.now();
    data.node.update = now;
    for (k in data.node.devs) {
        data.node.devs[k].time[0] = now;
    }

    nodes[data.id] = data.node;
    update_node_view(data.id, data.node);
    if (new_node) {
        update_list_view();
    }
});
setInterval(update_list_view, 4000);
