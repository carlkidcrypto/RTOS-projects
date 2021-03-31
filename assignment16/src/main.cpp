#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>

const char *SSID = "";
const char *PASSWORD = "";
const String IOT_SERVER_ADDR = "";
const String IOT_SERVER_KEY = "";
const String IOT_SERVER_ID = "";
WebServer server(80);
const int led = LED_BUILTIN;

/***** Begin: Define tasks/functions *****/
void handleRoot();
void handleNotFound();
void setup();
void loop();
void drawGraph();
String iot_server_detect();
String iot_server_register();
void WEB_SERVER_TASK(void *pvParameters);
/***** End: Define tasks/functions *****/

void setup()
{
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);

  /***** Begin: Setup the wifi stuff *****/
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  /***** End: Setup the wifi stuff *****/

  /***** Begin: Tasks are created here *****/
  BaseType_t xWST_rtval = xTaskCreate(
      WEB_SERVER_TASK, "WST_TASK" // The Driver task.
      ,
      4096 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 1 // priority
      ,
      NULL // Task Handle
  );

  if (xWST_rtval != pdPASS)
  {
    for (;;)
      Serial.println(F("WST_TASK: Creation Error, not enough heap or stack!"));
  }
  /***** End: Tasks are created here *****/
}

void WEB_SERVER_TASK(void *pvparameters)
{
  /***** Begin: Setup the MDNS Responder *****/

  // This allows us to use http://esp32.local instead if http://ipaddress/
  if (MDNS.begin("esp32"))
  {
    Serial.println("MDNS responder started");
  }
  /***** End: Setup the MDNS Responder *****/

  /***** Begin: Setup callback functions for webserver *****/
  server.on("/", handleRoot);
  server.on("/test.svg", drawGraph);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  /***** End: Setup callback functions for webserver *****/

  /***** Begin: IOT Server Stuff *****/

  // 1: Check we can access the IOT SERVER
  String results = iot_server_detect();
  if(results != "\0")
  {
    // We good, keep going
  }
  else
  {
    // Not good, restart the ESP
    ESP.restart();
  }

  // 2: Lets register with the IOT Server
  results = iot_server_register();
  if(results != "\0")
  {
    // We good
    Serial.println(results);
  }
  else
  {
    // Not good, restart the ESP
    ESP.restart();
  }
  /***** End: IOT Server Stuff *****/

  for (;;)
  {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void handleRoot()
{
  digitalWrite(led, 1);
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP32!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

           hr, min % 60, sec % 60);
  server.send(200, "text/html", temp);
  digitalWrite(led, 0);
}

void handleNotFound()
{
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void drawGraph()
{
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10)
  {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}

String iot_server_detect()
{
  HTTPClient http;
  String request_str = IOT_SERVER_ADDR + "IOTAPI/" + "DetectServer";
  http.begin(request_str);
  http.addHeader("Content-Type", "application/json");
  String message = "{\"key\":\"" + IOT_SERVER_KEY + "\"" + "}";
  int rtval = http.POST(message);

  if (rtval == 201) // success and resource created
  { //Check for the returning code
    String payload = http.getString(); 
    return payload;
  }

  else
  {
    return "\0";
  }

  http.end(); //Free the resources
}

String iot_server_register()
{
  HTTPClient http;
  String request_str = IOT_SERVER_ADDR + "IOTAPI/" + "RegisterWithServer";
  http.begin(request_str);
  http.addHeader("Content-Type", "application/json");
  String message = "{\"key\":\"" + IOT_SERVER_KEY + "\"" + ",\"iotid\":" + IOT_SERVER_ID + "}";
  int rtval = http.POST(message);

  if (rtval == 201)
  { //Check for the returning code
    String payload = http.getString();
    return payload;
  }

  else
  {
    return "\0";
  }

  http.end(); //Free the resources
}

void loop()
{
}