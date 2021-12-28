var http = require('http');
var fs = require('fs');
var index = fs.readFileSync( 'Areas.html');

var SerialPort = require('serialport');
const parsers = SerialPort.parsers;

const parser = new parsers.Readline({
    delimiter: '\r\n'
});

var port = new SerialPort('COM3',{ 
    baudRate: 9600,
    dataBits: 8,
    parity: 'none',
    stopBits: 1,
    flowControl: false
});

port.pipe(parser);

var app = http.createServer(function(req, res) {
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.end(index);
});

var io = require('socket.io').listen(app);

io.on('connection', function(socket) {
    
    console.log('Node is listening to port');
    
});

parser.on('data', function(data) {
    
    console.log('Received data from port: ' + data);
    
    if(data >= 5000000){
        io.emit('data_1', (data-5000000));
    }
    else if(data >= 4000000){
        io.emit('data_2', (data-4000000));
    }
    else if(data >= 3000000){
        io.emit('data_3', (data-3000000));
    }
    else if(data >= 2000000){
        io.emit('data_5', (data-2000000));
    }
    else if( data >= 1000000){
        io.emit('data_4', (data-1000000));
    };
    
});

app.listen(3000);
