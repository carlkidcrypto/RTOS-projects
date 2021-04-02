#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *SSID = "";
const char *PASSWORD = "";
const char *IOT_SERVER_ADDR = "";
const char *IOT_SERVER_KEY = "";
const char *IOT_SERVER_ID = "";
const char *auth_code = "";
WebServer server(80);
const int led = LED_BUILTIN;

/***** Begin: Define tasks/functions *****/
void handleRoot();
void handleNotFound();
void setup();
void loop();
String iot_server_detect();
String iot_server_register();
String iot_server_send_data(double data_temp, double data_humidity);
String iot_server_shutdown();
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
      5120 // Stack size
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
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  /***** End: Setup callback functions for webserver *****/

  /***** Begin: IOT Server Stuff *****/

  // 1: Check we can access the IOT SERVER
  String results = iot_server_detect();
  if (results != "\0")
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

  // Create a DynamicJsonDocument object
  DynamicJsonDocument DJD_Obj(1024);

  if (results != "\0")
  {
    // We good, let's grab and store that auth code
    deserializeJson(DJD_Obj, results);
    auth_code = DJD_Obj["auth_code"];
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
  char temp[525];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 525,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='10'/>\
    <title>esp32.local</title>\
    <style>\
      body { background-color: #99ff99; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP32!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>-----Server Info----- <br>\
    <b>IOT_SERVER_ADDR:</b> %s\
    <b>IOT_SERVER_KEY:</b> %s\
    <b>IOT_SERVER_ID:</b> %s\
    <b>IOT_SERVER_AUTH_CODE:</b> %s\
    </p>\
  </body>\
</html>",

           hr, min % 60, sec % 60, IOT_SERVER_ADDR, IOT_SERVER_KEY, IOT_SERVER_KEY, auth_code);
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

String iot_server_detect()
{
  HTTPClient http;
  String request_str = (String)IOT_SERVER_ADDR + "IOTAPI/" + "DetectServer";
  http.begin(request_str);
  http.addHeader("Content-Type", "application/json");
  String message = "{\"key\":\"" + (String)IOT_SERVER_KEY + "\"" + "}";
  int rtval = http.POST(message);

  if (rtval == 201) // success and resource created
  {                 //Check for the returning code
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
  String request_str = (String)IOT_SERVER_ADDR + "IOTAPI/" + "RegisterWithServer";
  http.begin(request_str);
  http.addHeader("Content-Type", "application/json");
  String message = "{\"key\":\"" + (String)IOT_SERVER_KEY + "\"" + ",\"iotid\":" + IOT_SERVER_ID + "}";
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

String iot_server_send_data(double data_temp, double data_humidity)
{
  HTTPClient http;
  String request_str = (String)IOT_SERVER_ADDR + "IOTAPI/" + "IOTData";
  http.begin(request_str);
  http.addHeader("Content-Type", "application/json");
  String message = "{\"auth_code\":\"" + (String)auth_code + "\"" + "\"temperature" "}";
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

String iot_server_shutdown()
{
  HTTPClient http;
  String request_str = (String)IOT_SERVER_ADDR + "IOTAPI/" + "IOTShutdown";
  http.begin(request_str);
  http.addHeader("Content-Type", "application/json");
  String message = "{\"auth_code\":\"" + (String)auth_code + "\"" + "}";
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