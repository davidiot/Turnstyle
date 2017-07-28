/* Node seems to have this really weird issue where random line breaks are inserted randomly in the serial output.
   The fix we're using is to use * as a delimiter and to concatenate fragments.
 */

var serialPort = require("serialport"),
    //plotly = require('plotly')('davidiot', 'E9DSPqiVQr0Kv6xlAzXG'),
    //token = 'osc6pvcvfc';
    /*plotly = require('plotly')('danosaur98', 's0v9KTMkvA65NXusv8Qg'),
    token_population = 'ey3dwxyslw',
    token_populationChange = 's4koc9mjjr';*/
    /*plotly = require('plotly')('dzhou', 'bieDR42pirTYTd1Lco37'),
    token_population = '7bqn8g9ojq',
    token_populationChange = '8x7rqcs062',
    token_populationAverage = '8uklvf0e0x';*/
    /*plotly = require('plotly')('flopper', 'PPJUQUYOOxeMVChHf8M9'),
    token_population = 'osmgrats43',
    token_populationChange = 'lsroxkkn18';*/
    plotly = require('plotly')('floppuh', '7KSZ1tKOIP2E1WeuMEBI'),
    token_population = '68szkxe5ib',
    token_populationChange = '0pvb3sksqa';

var portName = 'COM3';
var buffer = "";
var populationNumber = 0;
var average = 0;

// helper function to get a nicely formatted date string
function getDateString() {
    var time = new Date().getTime();
    var datestr = new Date(time - 4 * 3600000 /* converts to EST */)
        .toISOString()
        .replace(/T/, ' ')
        .replace(/Z/, '');
    return datestr;
}

function getHour() {
    var time = new Date().getTime();
    var datestr = new Date(time - 4 * 3600000 /* converts to EST */);
    return datestr.getHours();
}

var sp = new serialPort(portName, {
    baudRate: 9600,
    parser: serialPort.parsers.Readline(
        {delimiter: "*"} /* we're using * as a delimiter because of the glitch */)
});

var population_initdata = [{x: [], y: [], stream: {token: token_population, maxpoints: 500}}];
var population_layout = {
    title: 'Time vs. Population',
    xaxis: {
        title: 'Time'
    },
    yaxis: {
        title: 'Population'
    }
};
var population_initlayout = {fileopt: "extend", filename: "Population", layout: population_layout};

var populationChange_init = [{x: [], type: "histogram", stream: {token: token_populationChange, maxpoints: 500}}];
var populationChange_layout = {
    title: 'Traffic per Hour',
    xaxis: {
        title: 'Hour'
    },
    yaxis: {
        title: 'Door Usage'
    }
};
var populationChange_initlayout = {fileopt: "extend", filename: "PopulationChange", layout: populationChange_layout};

var populationAverage_data = {x: [], y: []};
var populationAverage_init = [{x: [], y: [], type: "bar"}];
var populationAverage_layout = {
    title: 'Average population per hour',
    xaxis: {
        title: 'Hour'
    },
    yaxis: {
        title: 'Population'
    }
};
var populationAverage_initlayout = {
    fileopt: "overwrite",
    filename: "PopulationAverage",
    layout: populationAverage_layout
};

plotly.plot(population_initdata, population_initlayout, function (err, msg) {
    if (err) {
        return console.log(err)
    }

    console.log(msg);
    var stream_population = plotly.stream(token_population, function (err, res) {
        console.log(err, res);
    });

    var stream_populationChange = plotly.stream(token_populationChange, function (err, res) {
        console.log(err, res);
    });

    sp.on('data', function (input) {
            var inputString = input.toString().trim();
            if (inputString.endsWith("*")) {
                populationNumber = (buffer + inputString).replace(/[\W_]+/g, "");
                var population = JSON.stringify({
                    x: getDateString(),
                    y: populationNumber
                });
                console.log(population);
                stream_population.write(population + '\n');
                buffer = "";
                plotly.plot(populationChange_init, populationChange_initlayout, function (err, msg) {
                    var populationChange = JSON.stringify({
                        x: getHour()
                    });
                    console.log(populationChange);
                    //console.log(msg);
                    stream_populationChange.write(populationChange + '\n');
                    setInterval(function () {
                        stream_populationChange.write('\n')
                    }, 29000); //makes sure it doesn't disconnect after one minute
                });
                if (Number.isInteger(parseInt(populationNumber))) {
                    populationAverage_data['x'].push(getHour());
                    populationAverage_data['y'].push(populationNumber);
                    average += parseInt(populationNumber);
                }
                if (populationAverage_init[0]['x'].indexOf(getHour()) === -1) {
                    populationAverage_init[0]['x'].push(getHour());
                    populationAverage_init[0]['y'].push(0);
                }
                populationAverage_init[0]['y'][populationAverage_init[0]['x'].indexOf(getHour())] = (parseInt(average) / populationAverage_data['x'].length).toFixed(2);
                plotly.plot(populationAverage_init, populationAverage_initlayout, function (err, msg) {
                    console.log((parseInt(average) / populationAverage_data['x'].length).toFixed(2));
                    //console.log(msg);
                });
            } else {
                buffer += inputString;
            }
        }
    );
    setInterval(function () {
        stream_population.write('\n')
    }, 29000); //makes sure it doesn't disconnect after one minute
});

