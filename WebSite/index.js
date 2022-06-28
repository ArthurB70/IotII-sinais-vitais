const http = require('http');
var url = require('url');
var fs = require('fs');
var p = require ('path');
var mqtt = require('mqtt');


var temperatura = '0';
var batimento = '0';
var oxigenacao = '0';
var leitura = 'final';
var gatilho = 0;
var request = null;
var response = null;

// Crie uma instância do servidor http para manipular solicitações HTTP
let app = http.createServer((req, res) => {
	request = req;
	response = res;
	page_route();
});

function page_route(){
	var path = url.parse(request.url).pathname, extension;
    var extension = p.extname(path);
    switch(extension){
        case '.png':
            response.writeHead(200, {  
                'Content-Type':  'image/png'  
            });  
            break;
        case '.jpg' || '.jpeg':
            response.writeHead(200, {  
                'Content-Type':  'image/jpeg'  
            }); 
            break;
        case '.html':
            response.writeHead(200, {  
                'Content-Type':  'text/html'  
            });  
            break; 
        case '.css':
            response.writeHead(200, {  
                'Content-Type':  'text/css'  
            });  
            break;
        case '.js':
            response.writeHead(200, {  
                'Content-Type':  'text/javascript'  
            });  
            break;
        case '.post':
            client.publish('sinais_vitais_topic', 'Hello');
            break;
        default:
            response.writeHead(200, {  
                'Content-Type':  'text/html'  
            });  
            break;
    }
    fs.readFile(__dirname + path, function(error, data) {  
        if (error) {  
            response.writeHead(404);  
            response.write(error);  
            response.end();  
        } else {
            if(path == '/leitura.html'){
                
                var string_content = String(data);
				string_content = string_content.replace('{oxigenacao}', oxigenacao);
				string_content = string_content.replace('{batimento}', batimento);
				string_content = string_content.replace('{temperatura}', temperatura);
                string_content = string_content.replace('{leitura}', leitura);
                if(gatilho == 0){
                    client.publish('sinais_vitais_topic', 'usuario: m');
                }
                gatilho = 1;
				response.write(string_content);  
            }
            else{
                if(path == '/index.html'){
                    gatilho = 0;
                    temperatura = '0';
                    batimento = '0';
                    oxigenacao = '0';
                    leitura = 'parcial';
                }
                response.write(data);  
            }
            
            response.end();  
        }  
    });
}

// Inicie o servidor na porta 80
app.listen(80, '127.0.0.1');

var options = {
    host: 'ba87952a69c64e73a1569a69541d7fd6.s1.eu.hivemq.cloud',
    port: 8883,
    protocol: 'mqtts',
    username: 'iot-vital-signs-user',
    password: '#Po23aa.14sz'
}

// initialize the MQTT client
var client = mqtt.connect(options);

// setup the callbacks
client.on('connect', function () {
    console.log('Connected');
});

client.on('error', function (error) {
    console.log(error);
});

client.on('message', function (topic, message) {
    // called each time a message is received
	var string_message = message.toString();
	if(string_message.length > 11) {
		var device = string_message.substring(0,6);
		if(device = 'device'){
            leitura_subs = string_message.substring(6,9);
			string_message = string_message.substring(11,string_message.length);
			split_message = string_message.split(',');
			if(split_message.length == 3){
				temperatura = split_message[2];
				batimento = split_message[1];
				oxigenacao = split_message[0];
                if(leitura_subs == '(p)'){
                    leitura = 'parcial';
                }
                else if(leitura_subs == '(f)'){
                    leitura = 'final'
                }
                else{
                    leitura = '-----'
                }
            }

		}
	}
	
    console.log('Received message:', topic, message.toString());
});

// subscribe to topic 'my/test/topic'
client.subscribe('sinais_vitais_topic');

// publish message 'Hello' to topic 'my/test/topic'
// client.publish('sinais_vitais_topic', 'Hello');

