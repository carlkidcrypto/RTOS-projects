#include <main.h>

void setup()
{
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);

  /***** Begin: Setup the wifi stuff *****/
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
#ifdef DEBUG_FLAG
  Serial.println("");
#endif

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#ifdef DEBUG_FLAG
    Serial.print(".");
#endif
  }

#ifdef DEBUG_FLAG
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  /***** End: Setup the wifi stuff *****/

  /***** Begin: Setup the Semaphores/Queues *****/
  LCD_SEMAPHORE = xSemaphoreCreateBinary();
  if (LCD_SEMAPHORE == NULL)
  {
    for (;;)
      Serial.println(F("LCD_SEMAPHORE: Creation Error, not enough heap mem!"));
  }

  HT_SEMAPHORE = xSemaphoreCreateBinary();
  if (HT_SEMAPHORE == NULL)
  {
    for (;;)
      Serial.println(F("HT_SEMAPHORE: Creation Error, not enough heap mem!"));
  }

  HT_QUEUE = xQueueCreate(1, sizeof(struct humi_temp));
  if (HT_QUEUE == NULL)
  {
    for (;;)
      Serial.println(F("HT_QUEUE: Creation Error, not enough heap mem!"));
  }

  SM_QUEUE = xQueueCreate(1, sizeof(struct stepper_motor));
  if (SM_QUEUE == NULL)
  {
    for (;;)
      Serial.println(F("SM_QUEUE: Creation Error, not enough heap mem!"));
  }

  NP_QUEUE = xQueueCreate(1, sizeof(struct NeoPixel));
  if (NP_QUEUE == NULL)
  {
    for (;;)
      Serial.println(F("NP_QUEUE: Creation Error, not enough heap mem!"));
  }
  /***** End: Setup the Semaphores/Queues *****/


  /***** Begin: Tasks are created here *****/
  BaseType_t xWST_rtval = xTaskCreate(
      WEB_SERVER_TASK, "WST_TASK" // The Web Server Task
      ,
      4096 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 3 // priority
      ,
      NULL // Task Handle
  );

  if (xWST_rtval != pdPASS)
  {
    for (;;)
      Serial.println(F("WST_TASK: Creation Error, not enough heap or stack!"));
  }

  vTaskDelay(pdMS_TO_TICKS(1000));

  BaseType_t xIOTT_rtval = xTaskCreate(
      IOT_TASK, "IOT_TASK" // The IOT Task
      ,
      4096 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 2 // priority
      ,
      NULL // Task Handle
  );

  if (xIOTT_rtval != pdPASS)
  {
    for (;;)
      Serial.println(F("xIOT_TASK: Creation Error, not enough heap or stack!"));
  }

  vTaskDelay(pdMS_TO_TICKS(2000));

  BaseType_t xNPT_RTVAL = xTaskCreate(
      NEO_PIXEL_TASK, "NEO_PIXEL_TASK_TASK", // NeoPixel Task
      1024                 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 1 // priority
      ,
      NULL);

  if (xNPT_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("NP_TASK: Creation Error, not enough heap or stack!"));
  }

  BaseType_t xHTT_RTVAL = xTaskCreate(
      HUMI_TEMP_TASK, "HT_TASK", // Humidity Temperature Task
      1024 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 1 // priority
      ,
      NULL);

  if (xHTT_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("HUMI_TEMP_TASK: Creation Error, not enough heap or stack!"));
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
#ifdef DEBUG_FLAG
  Serial.println("HTTP server started");
#endif
  /***** End: Setup callback functions for webserver *****/

  for (;;)
  {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void IOT_TASK(void *pvParameters)
{
  /***** Begin: IOT Server Stuff *****/

  // 1: Check we can access the IOT SERVER
  String results = iot_server_detect();
  vTaskDelay(pdMS_TO_TICKS(250));
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
  vTaskDelay(pdMS_TO_TICKS(250));

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

  // 3: Now we send test data to IOT Server
  results = iot_server_send_data(100.0, 100.0);
  vTaskDelay(pdMS_TO_TICKS(250));
  if (results != "\0")
  {
    // We good
  }
  else
  {
    // Not good, restart the ESP
    ESP.restart();
  }

  // 4: Now we check if we can receive commands from IOT Server
  results = iot_server_query_commands();
  vTaskDelay(pdMS_TO_TICKS(250));
  if (results != "\0")
  {
    // We good
  }
  else
  {
    // Not good, restart the ESP
    ESP.restart();
  }

  /***** End: IOT Server Stuff *****/

  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void NEO_PIXEL_TASK(void *pvParameters)
{
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEO_PIXEL_PIN, NEO_GRBW + NEO_KHZ800);
  NeoPixel neopixel;
  // Set NeoPixels Color & brightness
  neopixel.one.rgbw.red = 0;
  neopixel.one.rgbw.blue = 0;
  neopixel.one.rgbw.green = 0;
  neopixel.one.rgbw.white = 0;

  neopixel.two.rgbw.red = 0;
  neopixel.two.rgbw.blue = 0;
  neopixel.two.rgbw.green = 0;
  neopixel.two.rgbw.white = 0;

  neopixel.three.rgbw.red = 0;
  neopixel.three.rgbw.blue = 0;
  neopixel.three.rgbw.green = 0;
  neopixel.three.rgbw.white = 0;

  neopixel.four.rgbw.red = 0;
  neopixel.four.rgbw.blue = 0;
  neopixel.four.rgbw.green = 0;
  neopixel.four.rgbw.white = 0;

  neopixel.rainbow_mode = false;
  // Initialize all pixels to 'on'
  strip.setBrightness(20); // This func set brightness for all pixels
  strip.begin();
  strip.show();

  for (;;)
  {
    strip.show();

    // Check the NP_QUEUE
    if (NP_QUEUE != NULL)
    {
      if (xQueueReceive(NP_QUEUE, &neopixel, (TickType_t)0) == pdTRUE)
      {
        // we got something yay!
      }
    } // End check f or NP_QUEUE null

    // Do the NeoPixel Magic
    if (neopixel.rainbow_mode == true)
    {
      // Do the ranbow effect thing instead, set brightness for all pixels
      strip.setBrightness(20);
      for (uint16_t i = 0; i < strip.numPixels(); i++)
      {
        strip.setPixelColor(i, strip.Color(20, 0, 0, 0));
        strip.show();
        vTaskDelay(pdMS_TO_TICKS(25));
      }

      for (uint16_t i = 0; i < strip.numPixels(); i++)
      {
        strip.setPixelColor(i, strip.Color(0, 20, 0, 0));
        strip.show();
        vTaskDelay(pdMS_TO_TICKS(25));
      }

      for (uint16_t i = 0; i < strip.numPixels(); i++)
      {
        strip.setPixelColor(i, strip.Color(0, 0, 20, 0));
        strip.show();
        vTaskDelay(pdMS_TO_TICKS(25));
      }

      for (uint16_t i = 0; i < strip.numPixels(); i++)
      {
        strip.setPixelColor(i, strip.Color(0, 0, 0, 20));
        strip.show();
        vTaskDelay(pdMS_TO_TICKS(25));
      }
    }
    else // The rainbow var is not set
    {
      // Set each individual pixel
      strip.setPixelColor(0, strip.Color(neopixel.one.rgbw.red, neopixel.one.rgbw.green, neopixel.one.rgbw.blue, neopixel.one.rgbw.white));
      strip.show();
      strip.setPixelColor(1, strip.Color(neopixel.two.rgbw.red, neopixel.two.rgbw.green, neopixel.two.rgbw.blue, neopixel.two.rgbw.white));
      strip.show();
      strip.setPixelColor(2, strip.Color(neopixel.three.rgbw.red, neopixel.three.rgbw.green, neopixel.three.rgbw.blue, neopixel.three.rgbw.white));
      strip.show();
      strip.setPixelColor(3, strip.Color(neopixel.four.rgbw.red, neopixel.four.rgbw.green, neopixel.four.rgbw.blue, neopixel.four.rgbw.white));
      strip.show();
      vTaskDelay(pdMS_TO_TICKS(25));
    }

    // Yield for other tasks, we need to run as often as we can
    vTaskDelay(pdMS_TO_TICKS(1));
  } // End of for loop
}

void HUMI_TEMP_TASK(void *pvParameters)
{
  humi_temp HT_READINGS;

  // Create an object our sensor
  ClosedCube_HDC1080 hdc1080;

  // Start up the sensor at i2c address
  hdc1080.begin(0x40);
  // Set sensor resolution, humidity, temp
  hdc1080.setResolution(HDC1080_RESOLUTION_14BIT, HDC1080_RESOLUTION_14BIT);

  for (;;)
  {
    // get the humi/temp values
    HT_READINGS.temperature = hdc1080.readTemperature();
    HT_READINGS.humidity = hdc1080.readHumidity();

    // Do we have the semaphore
    if (xSemaphoreTake(HT_SEMAPHORE, 0) == pdTRUE)
    {
      // Send read value to HT_QUEUE
      if (HT_QUEUE != NULL)
      {
        if (xQueueSendToBack(HT_QUEUE, &HT_READINGS, (TickType_t)0) == pdTRUE)
        {
#if DEBUG_FLAG
          Serial.print(F("HT_TASK: Success sent value to HT_QUEUE! - "));
          Serial.print(HT_READINGS.humidity);
          Serial.print(" - ");
          Serial.println(HT_READINGS.temperature);
#endif
        }

        else
        {
#if DEBUG_FLAG
          Serial.print(F("HT_TASK: Failure sending value to HT_QUEUE! HT_QUEUE FULL! - "));
          Serial.print(HT_READINGS.humidity);
          Serial.print(" - ");
          Serial.println(HT_READINGS.temperature);
#endif
        }
      }
    }

    // We delay for other taks
    vTaskDelay(pdMS_TO_TICKS(100));
  } // End of for loop
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

#ifdef DEBUG_FLAG
  Serial.print("iot_server_detect(): sent request to - ");
  Serial.print(request_str);
  Serial.print(" - with POST message - ");
  Serial.println(message);
#endif

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

#ifdef DEBUG_FLAG
  Serial.print("iot_server_register(): sent request to - ");
  Serial.print(request_str);
  Serial.print(" - with POST message - ");
  Serial.println(message);
#endif

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
  String message = "{\"auth_code\":\"" + (String)auth_code + "\"" + ", \"temperature\":" + data_temp + ", \"humidity\":" + data_humidity + "}";
  int rtval = http.POST(message);

#ifdef DEBUG_FLAG
  Serial.print("iot_server_send_data(): sent request to - ");
  Serial.print(request_str);
  Serial.print(" - with POST message - ");
  Serial.println(message);
#endif

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

String iot_server_query_commands()
{
  HTTPClient http;
  String request_str = (String)IOT_SERVER_ADDR + "IOTAPI/" + "QueryServerForCommands";
  http.begin(request_str);
  http.addHeader("Content-Type", "application/json");
  String message = "{\"auth_code\":\"" + (String)auth_code + "\"" + ", \"iotid\":" + IOT_SERVER_ID + "}";
  int rtval = http.POST(message);

#ifdef DEBUG_FLAG
  Serial.print("iot_server_query_commands(): sent request to - ");
  Serial.print(request_str);
  Serial.print(" - with POST message - ");
  Serial.println(message);
#endif

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

#ifdef DEBUG_FLAG
  Serial.print("iot_server_shutdown(): sent request to - ");
  Serial.print(request_str);
  Serial.print(" - with POST message - ");
  Serial.println(message);
#endif

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