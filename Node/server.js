/* Node seems to have this really weird issue where random line breaks are inserted randomly in the serial output.
   The fix we're using is to use * as a delimiter and to concatenate fragments.
 */

var serialPort = require("serialport"),
    //plotly = require('plotly')('davidiot', 'E9DSPqiVQr0Kv6xlAzXG'),
    //token = 'osc6pvcvfc';
    plotly = require('plotly')('danosaur98', 's0v9KTMkvA65NXusv8Qg'),
    token = 'ey3dwxyslw',
    token1 = 's4koc9mjjr';


var portName = 'COM4';
var buffer = "";
var hourFirst = 0;
var hourLast = 0;

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
    var datestr = new Date(time - 4 * 3600000 /* converts to EST */)
    return datestr.getHours();
}

var sp = new serialPort(portName, {
    baudRate: 9600,
    parser: serialPort.parsers.Readline(
        {delimiter: "*"} /* we're using * as a delimiter because of the glitch */)
});

var population_initdata = [{x: [], y: [], stream: {token: token, maxpoints: 500}}];
var population_initlayout = {fileopt: "extend", filename: "Population"};

var populationChange_init = [{x: [], y: [], type: "bar", stream: {token: token1, maxpoints: 500}}];
var populationChange_initlayout = {fileopt: "overwrite", filename: "PopulationChange"};

var populationBucket_init = [{x: [], y: [], type: "bar"}];
var populationBuckete_initlayout = {fileopt: "overwrite", filename: "PopulationBucket"};

plotly.plot(population_initdata, population_initlayout, function (err, msg) {
    if (err) {
        return console.log(err)
    }

    console.log(msg);
    var stream = plotly.stream(token, function (err, res) {
        console.log(err, res);
    });

    sp.on('data', function (input) {
            var inputString = input.toString().trim();
            if (inputString.endsWith("*")) {
                populationNumber = (buffer + inputString).replace(/[\W_]+/g, "")
                var streamObject = JSON.stringify({
                    x: getDateString(),
                    y: populationNumber
                });
                console.log(streamObject);
                stream.write(streamObject + '\n');
                buffer = "";
                if (populationChange_init[0]['x'].indexOf(getHour()) === -1) {
                    populationChange_init[0]['x'].push(getHour());
                    hourFirst = hourLast = populationNumber
                    populationChange_init[0]['y'].push(0);
                }
                else {
                    hourLast = populationNumber;
                }
                populationChange_init[0]['y'][populationChange_init[0]['x'].indexOf(getHour())] = hourLast - hourFirst;
                plotly.plot(populationChange_init, populationChange_initlayout, function (err, msg) {
                    console.log(msg);
                });

            } else {
                buffer += inputString;
            }
        }
    );
    setInterval(function () {
        stream.write('\n')
    }, 29000); //makes sure it doesn't disconnect after one minute
});

