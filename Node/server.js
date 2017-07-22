/* Node seems to have this really weird issue where random line breaks are inserted randomly in the serial output.
   The fix we're using is to use * as a delimiter and to concatenate fragments.
 */

var serialPort = require("serialport"),
    plotly = require('plotly')('davidiot','E9DSPqiVQr0Kv6xlAzXG'),
    token = 'osc6pvcvfc';


var portName = 'COM4';
var buffer = "";

// helper function to get a nicely formatted date string
function getDateString() {
    var time = new Date().getTime();
    var datestr = new Date(time - 4 * 3600000 /* converts to EST */)
        .toISOString()
        .replace(/T/, ' ')
        .replace(/Z/, '');
    return datestr;
}

var sp = new serialPort(portName, {
    baudRate: 9600,
    parser: serialPort.parsers.Readline(
        {delimiter: "*"} /* we're using * as a delimiter because of the glitch */)
});

var initdata = [{x:[], y:[], stream:{token:token, maxpoints: 500}}];
var initlayout = {fileopt : "extend", filename : "stream-test"};

plotly.plot(initdata, initlayout, function (err, msg) {
    if (err) {
        return console.log(err)
    }

    console.log(msg);
    var stream = plotly.stream(token, function (err, res) {
        console.log(err, res);
    });

    sp.on('data', function(input) {
        var inputString = input.toString().trim();
        if (inputString.endsWith("*")) {
            var streamObject = JSON.stringify({
                x : getDateString(),
                y : (buffer + inputString).replace(/[\W_]+/g,"")
            });
            console.log(streamObject);
            stream.write(streamObject+'\n');
            buffer = "";
        } else {
            buffer += inputString;
        }
    });
});