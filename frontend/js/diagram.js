/*
 * Copyright (c) 2015
 * Author: Nikita Sushko
 * Url: fridary.com
 */


 Raphael.fn.highlight = function (path, number, has_dashes) {
    function getRandColor(brightness) {
        //6 levels of brightness from 0 to 5, 0 being the darkest
        var rgb = [Math.random() * 256, Math.random() * 256, Math.random() * 256];
        var mix = [brightness*51, brightness*51, brightness*51]; // 51 => 255/5
        var mixedrgb = [rgb[0] + mix[0], rgb[1] + mix[1], rgb[2] + mix[2]].map(function(x){ return Math.round(x/2.0)})
        return "rgb(" + mixedrgb.join(",") + ")";
    }

    var bbox = path.getBBox();
    var x = bbox.x + bbox.width/2,
        y = bbox.y + bbox.height/2 + 15;
    var text = this.text(x, y, "+" + number).attr({fill: getRandColor(5), "font-size": 14});
    text.animate({y: y - 30, opacity: 0}, 2000, function() {
        text.remove();
    });

    if (number != 1) path.attr({"stroke-dasharray": ""});

    if (number > 1 && number <= 5) number = 2;
    else if (number > 5 && number <= 10) number = 3;
    else if (number > 10 && number <= 25) number = 4;
    else if (number > 25 && number <= 50) number = 5;
    else if (number > 50 && number <= 100) number = 6;
    else if (number > 100 && number <= 200) number = 7;
    else if (number > 200 && number <= 300) number = 9;
    else if (number > 300 && number <= 400) number = 11;
    else if (number > 400 && number <= 500) number = 13;
    else if (number > 500 && number <= 1000) number = 16;
    else if (number > 1000) number = 20;


    path.animate({"stroke-width": number}, 2000, function() {
        path.animate({"stroke-width": 1}, 1000);
    });
};

Raphael.fn.connection = function (o) {
    var obj1 = o.shape1,
        obj2 = o.shape2,
        dash = o.dash || "",
        line = o.line || "#fff",
        bg = o.bg || null,
        longer = o.longer || 0;

    if (o.tri && o.from && o.to) {
        obj1 = o.from;
        obj2 = o.to;
    }
    var bb1 = obj1.getBBox(),
        bb2 = obj2.getBBox(),
        p = [
                {x: bb1.x + bb1.width / 2, y: bb1.y - 1},
                {x: bb1.x + bb1.width / 2, y: bb1.y + bb1.height + 1},
                {x: bb1.x - 1, y: bb1.y + bb1.height / 2},
                {x: bb1.x + bb1.width + 1, y: bb1.y + bb1.height / 2},
                {x: bb2.x + bb2.width / 2, y: bb2.y - 1},
                {x: bb2.x + bb2.width / 2, y: bb2.y + bb2.height + 1},
                {x: bb2.x - 1, y: bb2.y + bb2.height / 2},
                {x: bb2.x + bb2.width + 1, y: bb2.y + bb2.height / 2}
        ],
        d = {}, dis = [];
    for (var i = 0; i < 4; i++) {
        for (var j = 4; j < 8; j++) {
            var dx = Math.abs(p[i].x - p[j].x),
                dy = Math.abs(p[i].y - p[j].y);
            if ((i == j - 4) || (((i != 3 && j != 6) || p[i].x < p[j].x) && ((i != 2 && j != 7) || p[i].x > p[j].x) && ((i != 0 && j != 5) || p[i].y > p[j].y) && ((i != 1 && j != 4) || p[i].y < p[j].y))) {
                dis.push(dx + dy);
                d[dis[dis.length - 1]] = [i, j];
            }
        }
    }

    if (dis.length == 0) {
        var res = [0, 4];
    } else {
        for (var i = 0; i < longer; i++)
            dis.splice(dis.indexOf(Math.min.apply(Math, dis)), 1);
        res = d[Math.min.apply(Math, dis)];
    }
    var x1 = p[res[0]].x,
        y1 = p[res[0]].y,
        x4 = p[res[1]].x,
        y4 = p[res[1]].y;
    dx = Math.max(Math.abs(x1 - x4) / 2, 10);
    dy = Math.max(Math.abs(y1 - y4) / 2, 10);
    var x2 = [x1, x1, x1 - dx, x1 + dx][res[0]].toFixed(3),
        y2 = [y1 - dy, y1 + dy, y1, y1][res[0]].toFixed(3),
        x3 = [0, 0, 0, 0, x4, x4, x4 - dx, x4 + dx][res[1]].toFixed(3),
        y3 = [0, 0, 0, 0, y1 + dy, y1 - dy, y4, y4][res[1]].toFixed(3);
    var path = ["M", x1.toFixed(3), y1.toFixed(3), "C", x2, y2, x3, y3, x4.toFixed(3), y4.toFixed(3)].join(",");


    var pa = this.path(path).attr({stroke: color, fill: "none", "stroke-dasharray": dash});
    var t = pa.getPointAtLength(pa.getTotalLength() / 2);
    var midX = t.x;
    var midY = t.y;
    pa.remove();

    var diff = 0;
    var cos = 0.866; 
    var sin = 0.500; 
    var dx = (x4 - x1); 
    var dy = (y4 - y1); 

    var length = Math.sqrt(dx * dx + dy * dy) * 0.8; 
    dx = 8 * (dx / length); 
    dy = 8 * (dy / length); 
    var pX1 = (midX + diff) - (dx * cos + dy * -sin); 
    var pY1 = midY - (dx * sin + dy * cos); 
    var pX2 = (midX + diff) - (dx * cos + dy * sin); 
    var pY2 = midY - (dx * -sin + dy * cos); 

    triangle = ["M", midX + diff, midY,
                            "L", pX1, pY1,
                            "L", pX2, pY2,
                            "L",midX + diff, midY,
                            "z"]; 
    if (o.tri) {
        o.bg && o.bg.attr({path: path});
        o.line.attr({path: path});
        o.tri.attr({path: triangle}); 
    } else {
        var color = typeof line == "string" ? line : "#000";
        return {
            bg: bg && bg.split && this.path(path).attr({stroke: bg.split("|")[0], fill: "none", "stroke-width": bg.split("|")[1] || 3}),
            line: this.path(path).attr({stroke: color, fill: "none", "stroke-dasharray": dash}),
            tri: this.path(triangle).attr({stroke: color,"fill":(line == "transparent" ? "transparent" : "#000")}),
            from: obj1,
            to: obj2,
            longer: longer
        };
    }
};




window.onload = function() {

    $('#tabs a').click(function(e) {
        e.preventDefault();
        $('#tabs li').removeClass('active');
        $(this).parent().addClass('active');
        $('#diagram, #logs, #states').hide();
        $($(this).attr('href')).fadeIn('fast');

        if ($(this).attr('href') == '#states') {
            var arr_states = [];
            for (var i = 0; i < 11; i++)
                arr_states.push([]);

            $.each(hash, function(k, v) {
                var state_number;
                if (v.server_state == "CLOSED") state_number = 0;
                if (v.server_state == "LISTEN") state_number = 1;
                if (v.server_state == "SYN RECEIVED") state_number = 2;
                if (v.server_state == "SYN SENT") state_number = 3;
                if (v.server_state == "ESTABLISHED") state_number = 4;
                if (v.server_state == "FIN WAIT 1") state_number = 5;
                if (v.server_state == "FIN WAIT 2") state_number = 6;
                if (v.server_state == "CLOSING") state_number = 7;
                if (v.server_state == "TIME WAIT") state_number = 8;
                if (v.server_state == "CLOSE WAIT") state_number = 9;
                if (v.server_state == "LAST ACK") state_number = 10;

                arr_states[state_number].push(k);
            });

            var states = $('#states');
            states.empty();
            for (var i in objects) {
                if (i == 11) break; // CLOSED dublicate
                states.append('<div class="state" id="s_'+i+'"><a href="#">'+objects[i][2]+' <span>'+arr_states[i].length+'</span></a><div class="ips"></div></div>');
            }

            console.log(hash);

            states.find('> div > a').click(function(e) {
                e.preventDefault();
                var ips = $(this).closest('div').find('.ips');
                var state_number = $(this).closest('.state').attr('id').substring(2);
                if (!ips.is(':visible')) {
                    ips.empty();
                    for (var i in arr_states[state_number])
                        ips.append('<div>'+arr_states[state_number][i]+'</div>');
                }
                ips.slideToggle();
            });
        }
    });

    var hash = {},
        r_w = $('#diagram').width(),
        r_h = $('#diagram').height(),
        r = Raphael("diagram", "100%", "100%"),
        state_w = 110,
        state_h = 40,
        state_r = 10,
        objects = [
                [0, 0, "CLOSED", "rgb(191, 0, 0)"], // 0
                [0, 100, "LISTEN", "rgb(191, 172, 0)"], // 1
                [-350, 200, "SYN\nRECEIVED", "gray"], // 2
                [350, 200, "SYN\nSENT", "gray"], // 3
                [0, 250, "ESTABLISHED", "rgb(124, 191, 0)"], // 4
                [-350, 350, "FIN WAIT 1", "rgb(191, 172, 0)"], // 5
                [-350, 450, "FIN WAIT 2", "rgb(191, 172, 0)"], // 6
                [0, 350, "CLOSING", "rgb(191, 172, 0)"], // 7
                [0, 450, "TIME WAIT", "rgb(191, 172, 0)"], // 8
                [350, 350, "CLOSE WAIT", "rgb(191, 172, 0)"], // 9
                [350, 450, "LAST ACK", "rgb(191, 172, 0)"], // 10
                [0, 530, "CLOSED", "rgb(191, 0, 0)"], // 11
        ],
        dragger = function () {
            this.ox = this.type == "ellipse" ? this.attr("cx") : this.attr("x");
            this.oy = this.type == "ellipse" ? this.attr("cy") : this.attr("y");
            if (this.type != "text") this.animate({"fill-opacity": .4}, 500);
                
            this.pair.ox = this.pair.type == "ellipse" ? this.pair.attr("cx") : this.pair.attr("x");
            this.pair.oy = this.pair.type == "ellipse" ? this.pair.attr("cy") : this.pair.attr("y");
            if (this.pair.type != "text") this.pair.animate({"fill-opacity": .4}, 500);    
        },
        move = function (dx, dy) {
            var att = this.type == "ellipse" ? {cx: this.ox + dx, cy: this.oy + dy} : 
                                               {x: this.ox + dx, y: this.oy + dy};
            this.attr(att);
            
            att = this.pair.type == "ellipse" ? {cx: this.pair.ox + dx, cy: this.pair.oy + dy} : 
                                               {x: this.pair.ox + dx, y: this.pair.oy + dy};
            this.pair.attr(att);            
            
            for (var i = connections.length; i--;) {
                r.connection(connections[i]);
            }
            r.safari();
        },
        up = function () {
            if (this.type != "text") this.animate({"fill-opacity": .2}, 500);
            if (this.pair.type != "text") this.pair.animate({"fill-opacity": .2}, 500);            
        },
        connections = [],
        shapes = [],
        texts = [];

    for (var i = 0, ii = objects.length; i < ii; i++) {
        shapes.push(r.rect(r_w / 2 - state_w / 2 + objects[i][0], 30 + objects[i][1], state_w, state_h, state_r));
        texts.push(r.text(r_w / 2 + objects[i][0], 30 + state_h / 2 + objects[i][1], objects[i][2]));
    }

    for (var i = 0, ii = shapes.length; i < ii; i++) {
        var tempS = shapes[i].attr({fill: objects[i][3], stroke: objects[i][3], "fill-opacity": .2, "stroke-width": 2, cursor: "move"});
        var tempT = texts[i].attr({fill: objects[i][3], stroke: "none", "font-size": 14, cursor: "move"});
        shapes[i].drag(move, dragger, up);
        texts[i].drag(move, dragger, up);
        
        tempS.pair = tempT;
        tempT.pair = tempS;
    }

    var has_dashes = [2, 3, 4, 7, 8, 9, 14, 15, 17];

    connections.push(r.connection({shape1: shapes[0], shape2: shapes[1]})); // 0
    connections.push(r.connection({shape1: shapes[0], shape2: shapes[3], longer: 1})); // 1
    connections.push(r.connection({shape1: shapes[1], shape2: shapes[0], dash: "--", longer: 1})); // 2
    connections.push(r.connection({shape1: shapes[1], shape2: shapes[2], longer: 1})); // 3
    connections.push(r.connection({shape1: shapes[1], shape2: shapes[3], dash: "--"})); // 4
    connections.push(r.connection({shape1: shapes[2], shape2: shapes[1], dash: "--"})); // 5
    connections.push(r.connection({shape1: shapes[2], shape2: shapes[4]})); // 6
    connections.push(r.connection({shape1: shapes[2], shape2: shapes[5], dash: "--"})); // 7
    connections.push(r.connection({shape1: shapes[3], shape2: shapes[0], dash: "--"})); // 8
    connections.push(r.connection({shape1: shapes[3], shape2: shapes[2], dash: "--"})); // 9
    connections.push(r.connection({shape1: shapes[3], shape2: shapes[4]})); // 10
    connections.push(r.connection({shape1: shapes[4], shape2: shapes[5]})); // 11
    connections.push(r.connection({shape1: shapes[4], shape2: shapes[9]})); // 12
    connections.push(r.connection({shape1: shapes[5], shape2: shapes[6]})); // 13
    connections.push(r.connection({shape1: shapes[5], shape2: shapes[7], dash: "--"})); // 14
    connections.push(r.connection({shape1: shapes[5], shape2: shapes[8], dash: "--"})); // 15
    connections.push(r.connection({shape1: shapes[6], shape2: shapes[8]})); // 16
    connections.push(r.connection({shape1: shapes[7], shape2: shapes[8], dash: "--"})); // 17
    connections.push(r.connection({shape1: shapes[8], shape2: shapes[11]})); // 18
    connections.push(r.connection({shape1: shapes[9], shape2: shapes[10]})); // 19
    connections.push(r.connection({shape1: shapes[10], shape2: shapes[11]})); // 20



    //------------------------------------------------------------------------------+



    if ("WebSocket" in window) {
        var host = "127.0.0.1",
            ws = new WebSocket("ws://" + host + ":8088/echo");

        var timer_packets = [];



        function sendOnlyFlags(arr, flags) {
            for (var i in flags) {
                if (arr.indexOf(flags[i]) == -1)
                    return false;
            }
            if (arr.length != flags.length)
                return false;

            return true;
        }

        function sendSomeFlags(arr, flags) {
            for (var i in flags) {
                if (arr.indexOf(flags[i]) == -1)
                    return false;
            }

            return true;
        }

        function filterPackets(p) {
            var f = p.substr(p.indexOf("{"), p.length);
            return '[' + f.substr(0, f.lastIndexOf("},") + 1) + ']';
        }


        ws.onopen = function(e) {
            $('#logs').append('<p>websocket opened</p>');

            setInterval(function() {
                var line_count = [];
                for (var i = 0; i < connections.length; i++)
                    line_count.push(0);

                for (var i in timer_packets) {
                    var o = timer_packets[i];
                    if (o.ip_src == host) {
                        // server sends
                        var ho = o.ip_dst + ':' + o.th_dport;
                        if (hash.hasOwnProperty(ho)) {
                            if (sendSomeFlags(o.flags, ["RST"])) {
                                delete hash[ho];
                            }

                            if (hash[ho].client_flags == "SYN" && sendOnlyFlags(o.flags, ["SYN", "ACK"])) {
                                line_count[3]++; // LISTEN -> SYN RECEIVED
                                hash[ho].server_state = "SYN RECEIVED";
                            }


                            //-------+
                            // right diagram
                            //-------+

                            if (hash[ho].client_flags == "FIN" && sendOnlyFlags(o.flags, ["ACK"])) {
                                line_count[12]++; // ESTABLISHED -> CLOSE WAIT
                                hash[ho].server_state = "CLOSE WAIT";
                            } else if (sendOnlyFlags(o.flags, ["FIN"]) && hash[ho].server_state == "CLOSE WAIT") {
                                line_count[19]++; // CLOSE WAIT -> LAST ACK
                                hash[ho].server_state = "LAST ACK";
                            }


                            //-------+
                            // left diagram
                            //-------+

                            if (sendOnlyFlags(o.flags, ["FIN"]) && hash[ho].server_state == "ESTABLISHED") {
                                line_count[11]++; // ESTABLISHED -> FIN WAIT 1
                                hash[ho].server_state = "FIN WAIT 1";
                            } else if (hash[ho].client_flags == "FIN" && sendOnlyFlags(o.flags, ["ACK"]) && hash[ho].server_state == "FIN WAIT 1") {
                                line_count[14]++; // FIN WAIT 1 -> CLOSING
                                hash[ho].server_state = "CLOSING";
                            } else if (hash[ho].client_flags == "FIN" && sendOnlyFlags(o.flags, ["ACK"]) && hash[ho].server_state == "FIN WAIT 2") {
                                line_count[16]++; // FIN WAIT 2 -> TIME WAIT
                                hash[ho].server_state = "TIME WAIT";
                            } else if (hash[ho].client_flags == "FIN ACK" && sendOnlyFlags(o.flags, ["ACK"]) && hash[ho].server_state == "FIN WAIT 1") {
                                line_count[15]++; // FIN WAIT 1 -> TIME WAIT
                                hash[ho].server_state = "TIME WAIT";
                            }


                            hash[ho].client_flags = "";
                        }

                        if (sendSomeFlags(o.flags, ["FIN", "ACK"])) {
                            //line_count[15]++; // FIN WAIT 1 -> TIME WAIT
                            line_count[12]++; // ESTABLISHED -> CLOSE WAIT
                            line_count[19]++; // CLOSE WAIT -> LAST ACK
                        }
                    } else {
                        // client sends
                        var ho = o.ip_src + ':' + o.th_sport;
                        if (sendOnlyFlags(o.flags, ["SYN"])) {
                            hash[ho] = {"client_flags": "SYN"};
                        } else if (hash.hasOwnProperty(ho)) {
                            if (sendSomeFlags(o.flags, ["RST"])) {
                                delete hash[ho];
                            }

                            if (sendOnlyFlags(o.flags, ["ACK"]) && hash[ho].server_state == "SYN RECEIVED") {
                                //hash[ho].client_flags = "ACK";

                                line_count[6]++; // SYN RECEIVED -> ESTABLISHED
                                hash[ho].server_state = "ESTABLISHED";
                            }


                            //-------+
                            // right diagram
                            //-------+

                            if (sendOnlyFlags(o.flags, ["FIN"]) && hash[ho].server_state == "ESTABLISHED") {
                                hash[ho].client_flags = "FIN";
                            }

                            if (sendOnlyFlags(o.flags, ["ACK"]) && hash[ho].server_state == "LAST ACK") {
                                line_count[20]++; // LAST ACK -> CLOSED
                                delete hash[ho];
                            }


                            //-------+
                            // left diagram
                            //-------+

                            if (sendOnlyFlags(o.flags, ["ACK"]) && hash[ho].server_state == "FIN WAIT 1") {
                                line_count[13]++; // FIN WAIT 1 -> FIN WAIT 2
                                hash[ho].server_state = "FIN WAIT 2";
                            } else if (sendOnlyFlags(o.flags, ["FIN"]) && hash[ho].server_state == "FIN WAIT 1") {
                                hash[ho].client_flags = "FIN";
                            } else if (sendOnlyFlags(o.flags, ["FIN ACK"]) && hash[ho].server_state == "FIN WAIT 1") {
                                hash[ho].client_flags = "FIN ACK";
                            }

                            if (sendOnlyFlags(o.flags, ["FIN"]) && hash[ho].server_state == "FIN WAIT 2") {
                                hash[ho].client_flags = "FIN";
                            }

                            if (sendOnlyFlags(o.flags, ["ACK"]) && hash[ho].server_state == "CLOSING") {
                                line_count[17]++; // CLOSING -> TIME WAIT
                                hash[ho].server_state = "TIME WAIT";
                            }

                        }
                    }
                }

                timer_packets.length = 0;

                for (var i = 0; i < connections.length; i++) {
                    if (line_count[i] != 0)
                        r.highlight(connections[i].line, line_count[i], has_dashes.indexOf(i) > -1);
                }
            }, 1500);
        };

        ws.onerror = function(e) {
            $('#logs').append('<p>websocket error</p>');
        };

        ws.onmessage = function(e) {
            var packets = JSON.parse(filterPackets(e.data)),
                d = new Date(),
                logs = $('#logs');

            timer_packets = timer_packets.concat(packets);

            for (var i in packets) {
                var o = packets[i];
                var msg = "packet: " + o.packet + ", " + d.getHours()+":"+d.getMinutes()+":"+(d.getSeconds() < 10 ? '0'+d.getSeconds() : d.getSeconds())
                    +" IP "+(o.ip_src == host ? 'server' : o.ip_src)+":"+o.th_sport
                    +" > "+(o.ip_dst == host ? 'server' : o.ip_dst)+":"+o.th_dport
                    +", protocol: "+o.protocol+", flags: [ ";
                for (var ii in o.flags)
                    msg += o.flags[ii] + " ";
                msg += " ], seq: "+o.th_seq+", ack: "+o.th_ack+", win: "+o.th_win+", TCPsum: "+o.th_sum
                    +", IPsum: "+o.ip_sum+", urp: "+o.th_urp+", payload: "+o.payload+" bytes";

                logs.append('<p>' + msg + '</p>');
            }

            if ($(window).scrollTop() + $(window).height() > $(document).height() - 200) {
                $('html, body').scrollTop($(document).height());
            }

            if ($('#logs > p').length > 1000)
                $('#logs > p:lt(100)').remove();

        };

        ws.onclose = function(e) {
            $('#logs').append('<p>websocket closed</p>');
        };
    } else {
        alert('Your browser does not support websockets');
    }

};