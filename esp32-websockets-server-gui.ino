#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "Adafruit_HTU21DF.h"
#include <Ticker.h>

// Collecting HTU21D-F sensor data
Ticker timer;
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

// Connecting to the Internet
char * ssid = "<ssid>";
char * password = "<pswd>";

// Running a web server
WebServer server;

// Adding a websocket to the server
WebSocketsServer webSocket = WebSocketsServer(81);

char webpage[] PROGMEM = R"=====(
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>HTU sensor data stream</title>
  <script src='https://cdnjs.cloudflare.com/ajax/libs/smoothie/1.34.0/smoothie.min.js'></script> 
  <style type="text/css">
    body,h4 {
        color: white;
        font-family: tahoma;
        background-color: #000000;
      }
  </style>
</head>
<body onload="javascript:init()">
  <h4>temperature (C)</h4>
    <canvas id="tmp" width="1200" height="131"></canvas>
  
  <h4>humidity (%)</h4>
    <canvas id="hmd" width="1200" height="131"></canvas>
  
  <script>
    var webSocket;
    var tmp;
    var hmd;
    
    function init() {
      webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');   

      var smoothie1 = new SmoothieChart({
        millisPerPixel:22,
        interpolation:'step',
        labels:{fontSize:12},
        grid: {sharpLines:true,millisPerLine:2000,verticalSections:10},
        timestampFormatter:SmoothieChart.timeFormatter
      });

      var smoothie2 = new SmoothieChart({
        millisPerPixel:22,
        interpolation:'step',
        labels:{fontSize:12},
        grid: {sharpLines:true,millisPerLine:2000,verticalSections:10},
        timestampFormatter:SmoothieChart.timeFormatter
      });
      
      smoothie1.streamTo(document.getElementById("tmp"), 1000 /*delay*/);
      smoothie2.streamTo(document.getElementById("hmd"), 1000 /*delay*/);
      
      // Data
      var line1 = new TimeSeries();
      var line2 = new TimeSeries();

      // Add a temperature/humiduty value to each line every second
      setInterval(function() {
        line1.append(Date.now(), tmp);
        line2.append(Date.now(), hmd);
      }, 1000);

      // Add to SmoothieChart
      smoothie1.addTimeSeries(line1,
        { strokeStyle:'#00ffff', lineWidth:1.7 });
      smoothie2.addTimeSeries(line2,
         { strokeStyle:'#ff5bff', lineWidth:1.7 });
      
      webSocket.onmessage = function(event) {
        var data = JSON.parse(event.data);
        tmp = data.temperature;
        hmd = data.humidity;        
        console.log(data);
      }
    }
  </script>
</body>
</html>
)=====";

void setup() {
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
  }
  
  Serial.println("");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/",[](){
    server.send_P(200, "text/html", webpage);
  });  
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  htu.begin();
    
  // call function getData every 0.2 seconds
  timer.attach(0.2, getData);
}

void loop() {  
  server.handleClient();
  webSocket.loop();    

  // delay to collect correct data
  delay(400);
}

void getData(){
  String json = "{\"temperature\":";
  json += htu.readTemperature();
  json += ",\"humidity\":";
  json += htu.readHumidity();
  json += "}";
  webSocket.broadcastTXT(json.c_str(), json.length());
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
}
