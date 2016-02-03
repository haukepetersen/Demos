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

const STALE_TIME    = 5000;     /* node gets stale if no updated in 5 seconds */

/**
 * Define some global variables
 */
var socket = io();
var nodes = {};
var charts = {};
var active_node = undefined;

var chart_opts = {
    's:temp':   {'maxValue': 75, 'minValue': 0, 'millisPerPixel': 30},
    's:hum':    {'maxValue': 100, 'minValue': 0, 'millisPerPixel': 30},
    's:light':  {'maxValue': 1500, 'minValue': 0, 'millisPerPixel': 30},
    's:rgb':    {'maxValue': 256, 'minValue': 0, 'millisPerPixel': 30},
    's:acc':    {'maxValue': 6, 'minValue': 0, 'millisPerPixel': 15},
    's:mag':    {'maxValue': 30, 'minValue': 0, 'millisPerPixel': 15},
    's:gyro':   {'maxValue': 2500, 'minValue': 0, 'millisPerPixel': 15},
    'a:rgb':    {'maxValue': 256, 'minValue': 0, 'millisPerPixel': 30},
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
}

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
}

var snip_graph = function(k, dev) {
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
    return c;
}

var snip_s_temp = function(k, dev) {
    console.log(dev);
}


var update_list_view = function() {
    var now = new Date().getTime();
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

    Object.keys(node.devs).forEach(function(k) {
        nodeview.append('div')
            .attr('class', 'snip')
            .html(snip_graph(k, node.devs[k]));
        add_chart(k, node.devs[k]);
    });

    active_node = id;
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

        update_chart(k, dev);
    }
}

var add_chart = function(k, dev) {
    var opts = chart_opts[k];
    if (opts == undefined) {
        opts = {};
        console.log('not found for', k);
    }
    console.log(opts);

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
    console.log(charts[k].data[0]);

    charts[k].data.forEach(function(o, i) {

    });

    update_chart(k, dev);
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

// var make_chart = function(k, dev) {
//     charts[k] = c3.generate({
//         'bindto': '#' + k.replace(':', '-') + 'chart',
//         'size': {
//             'height': 200,
//         },
//         'data': {
//             'x': 'time',
//             'columns': [],
//             'type': 'line',
//             'colors': {
//                 'a': '#3fa687',
//                 'b': '#bc1a29',
//                 'c': '#aaaaaa'
//             }
//         },
//         'axis': {
//             'x': {
//                 'type': 'time',
//                 'tick': {
//                     'rotate': 90,
//                     'multiline': false
//                 }
//             },
//             'y': {
//                 'label': {
//                     'text': dev.unit,
//                     'position': 'outer-middle'
//                 }
//             }
//         }
//     });

//     update_chart(k, dev);
// }

// var update_chart = function(k, dev) {
//     console.log("updating chart");

//     var cols = [];
//     var time = ['time'];
//     var rows = [['a'], ['b'], ['c']];

//     dev.time.forEach(function(t) {
//         time.push(t);
//     });
//     cols.push(time);

//     dev.vals.forEach(function(v) {
//         if (Array.isArray(v)) {
//             for (var i = 0; i < v.length; i++) {
//                 rows[i].push(v[i]);
//             }
//         }
//         else {
//             rows[0].push(v);
//         }
//     });
//     for (var i = 0; i < 3; i++) {
//         if (rows[i].length > 1) {
//             cols.push(rows[i]);
//         }
//     }

//     charts[k].load({'columns': cols});
// }

/**
 * Configure web socket endpoints
 */
socket.on('init', function(data) {
    nodes = data;
    update_list_view();
});
socket.on('update', function(data) {
    var new_node = !(data.id in nodes);
    nodes[data.id] = data.node;
    update_node_view(data.id, data.node);
    if (new_node) {
        update_list_view();
    }
});
setInterval(update_list_view, 4000);
