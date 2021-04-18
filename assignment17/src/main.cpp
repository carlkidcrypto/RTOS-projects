#include <main.h>

void setup()
{
  Serial.begin(115200);
  /***** Begin: Setup the wifi stuff *****/
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
#if DEBUG_FLAG
  Serial.println("");
#endif

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#if DEBUG_FLAG
    Serial.print(".");
#endif
  }

#if DEBUG_FLAG
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  /***** End: Setup the wifi stuff *****/

  /***** Begin: Setup the Semaphores/Queues *****/
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

  NP_QUEUE = xQueueCreate(25, sizeof(struct NeoPixel));
  if (NP_QUEUE == NULL)
  {
    for (;;)
      Serial.println(F("NP_QUEUE: Creation Error, not enough heap mem!"));
  }
  /***** End: Setup the Semaphores/Queues *****/

  /***** Begin: Tasks are created here *****/
  BaseType_t xWST_rtval = xTaskCreate(
      WEB_SERVER_TASK, "WEB_SERVER_TASK" // The Web Server Task
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
      Serial.println(F("WEB_SERVER_TASK: Creation Error, not enough heap or stack!"));
  }

  // Wait 2.5 seconds for wifi web server task to be setup
  vTaskDelay(pdMS_TO_TICKS(2500));

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
      Serial.println(F("IOT_TASK: Creation Error, not enough heap or stack!"));
  }

  // Wait 2.5 seconds for IOT task to be setup
  vTaskDelay(pdMS_TO_TICKS(2500));

  BaseType_t xNPT_RTVAL = xTaskCreate(
      NEO_PIXEL_TASK, "NEO_PIXEL_TASK_TASK", // NeoPixel Task
      2048                                   // Stack size
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
      2048                       // Stack size
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

  BaseType_t xSMT_RTVAL = xTaskCreate(
      STEPPER_MOTOR_TASK, "STEPPER_MOTOR_TASK", // STEPPER MOTOR TASK
      1024                                      // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 1 // priority
      ,
      NULL);

  if (xSMT_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("STEPPER_MOTOR_TASK: Creation Error, not enough heap or stack!"));
  }

  /***** End: Tasks are created here *****/
}

void WEB_SERVER_TASK(void *pvparameters)
{
  /***** Begin: Setup callback functions for webserver *****/
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
#if DEBUG_FLAG
  Serial.println("HTTP server started");
#endif
  /***** End: Setup callback functions for webserver *****/

  stepper_motor SM;
  SM.forward = true;
  SM.RPM = 0;

  for (;;)
  {

    server.handleClient();
    // Check for GET Args.
    if (server.arg("dir") != "\0")
    {
      // We have got something. Let's adjust Stepper Motor direction.
      String argval = server.arg("dir");
#if DEBUG_FLAG
      Serial.println(argval);
#endif

      if (argval == "true")
      {
        SM.forward = true;
      }
      else if (argval == "false")
      {
        SM.forward = false;
      }
      else
      {
        // Not a valid option
      }
    }

    if (server.arg("rpm") != "\0")
    {
      // We have got something. Let's adjust Stepper Motor RPM.
      String argval = server.arg("rpm");
#if DEBUG_FLAG
      Serial.println(argval);
#endif
      SM.RPM = argval.toInt();
    }

    if (server.arg("stop") != "\0")
    {
      // We have got something. Let's adjust Stepper Motor RPM.
      String argval = server.arg("stop");
#if DEBUG_FLAG
      Serial.println(argval);
#endif
      if (argval == "true")
      {
        SM.RPM = 0;
      }
      else
      {
        // Not a valid option
      }
    }

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
    neopixel.four.rgbw.blue = 25;
    neopixel.four.rgbw.green = 0;
    neopixel.four.rgbw.white = 0;

    neopixel.rainbow_mode = false;

    // Send to NPQ
    if (xQueueSendToBack(NP_QUEUE, &neopixel, (TickType_t)0) == pdTRUE)
    {
#if DEBUG_FLAG
      Serial.println(F("WEB_SERVER_TASK: Success sending to NPQ!"));
#endif
    }

    // Send to SMQ
    if (xQueueSendToBack(SM_QUEUE, &SM, (TickType_t)0) == pdTRUE)
    {
#if DEBUG_FLAG
      Serial.println(F("WEB_SERVER_TASK: Success sending to SMQ!"));
#endif
    }

    // Delay for other tasks
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void IOT_TASK(void *pvParameters)
{
  /***** Begin: Setup local vars for task *****/
  double check_interval_secs = 60 * 5; // default 5 minutes
  double send_interval_secs = 60 * 5;  // default 5 minutes
  humi_temp ht;
  unsigned long prev_time_val1 = millis();
  unsigned long curr_time_val1 = 0;
  unsigned long prev_time_val2 = millis();
  unsigned long curr_time_val2 = 0;
  unsigned long time_diff1 = 0;
  unsigned long time_diff2 = 0;
  String results = "";
  DynamicJsonDocument DJD_Obj(1024);
  stepper_motor SM;
  SM.forward = true;
  SM.RPM = 0;
  xSemaphoreTake(HT_SEMAPHORE, 0);
  /***** End: Setup local vars for task *****/

  /***** Begin: IOT Server Stuff *****/

  // 1: Check we can access the IOT SERVER
  results = iot_server_detect();
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
    deserializeJson(DJD_Obj, results);
    // Check the results
    JsonArray Commands = DJD_Obj["commands"];
    for (JsonObject item : Commands)
    {
      const char *command = item["command"];
#if DEBUG_FLAG
      Serial.print("IOT_TASK: JSON Response decoded - ");
      Serial.println(command);
#endif

      if (strcmp(command, "RotQCCW") == 0)
      {
      }
      else if (strcmp(command, "Flash") == 0)
      {
        // Don't do anything
      }
      else if (strcmp(command, "SendNow") == 0)
      {
      }
      else if (strcmp(command, "RotQCW") == 0)
      {
      }
      else if (strcmp(command, "SetCheckFreq") == 0)
      {
      }
      else if (strcmp(command, "SetSendFreq") == 0)
      {
      }
      else
      {
        // not a valid command. Nothing changes continue
      }
    }
  }
  else
  {
    // Not good, restart the ESP
    ESP.restart();
  }

  DJD_Obj.clear(); // Lastly clear the  dynamic json object.
  results = "";    // clear results string.
  /***** End: IOT Server Stuff *****/

  for (;;)
  {
    // Read and check timer. If our send interval is over send semaphore to HT_SEMAPHORE
    curr_time_val1 = millis();
    time_diff1 = curr_time_val1 - prev_time_val1;
    if ((time_diff1 / 1000) >= send_interval_secs)
    {
      prev_time_val1 = curr_time_val1;
      // Sent the semaphore
      xSemaphoreGive(HT_SEMAPHORE);
    }
    else
    {
      // If we get here a semaphore must been given. Or send interval is not over yet.
      if (xQueueReceive(HT_QUEUE, &ht, (TickType_t)0) == pdTRUE)
      {
        // We received something, so we send it.
        results = iot_server_send_data(ht.temperature, ht.humidity);
        if (results != "\0")
        {
          // We good
        }
        else
        {
// Not good, data not sent. Yield and try again.
#if DEBUG_FLAG
          Serial.println("IOT_TASK: Error something went wrong. Yielding, line 407!");
#endif
          taskYIELD();
        }
        // clear the results and stuff
        DJD_Obj.clear();
        results = "";
      }
    }

    curr_time_val2 = millis();
    time_diff2 = curr_time_val2 - prev_time_val2;
    if ((time_diff2 / 1000) >= check_interval_secs)
    {
      prev_time_val2 = curr_time_val2;
      // If we get here then we need to check for iot server commands
      results = iot_server_query_commands();
      if (results != "\0")
      {
        // We good
        DeserializationError err = deserializeJson(DJD_Obj, results);
        if (err)
        {
          #if DEBUG_FLAG
          Serial.print("IOT_TASK: DeserializationError - ");
          Serial.println(err.c_str());
          #endif
        }
        else
        {
          JsonArray Commands = DJD_Obj["commands"];
          for (auto item : Commands)
          {
            const char *command = item["command"];
            double value = item["seconds"];

#if DEBUG_FLAG
            Serial.print("IOT_TASK: JSON Response decoded - ");
            Serial.print(command);
            Serial.println(value);
#endif

            if (strcmp(command, "RotQCCW") == 0)
            {
              SM.forward = false;
              SM.RPM = 4;
              // Send to SMQ
              if (xQueueSendToBack(SM_QUEUE, &SM, (TickType_t)0) == pdTRUE)
              {
#if DEBUG_FLAG
                Serial.println(F("IOT_TASK: Success sending to SMQ!"));
#endif
              }
            }
            else if (strcmp(command, "Flash") == 0)
            {
              // Don't do anything
            }
            else if (strcmp(command, "SendNow") == 0)
            {
            }
            else if (strcmp(command, "RotQCW") == 0)
            {
              SM.forward = true;
              SM.RPM = 4;
              // Send to SMQ
              if (xQueueSendToBack(SM_QUEUE, &SM, (TickType_t)0) == pdTRUE)
              {
#if DEBUG_FLAG
                Serial.println(F("IOT_TASK: Success sending to SMQ!"));
#endif
              }
            }
            else if (strcmp(command, "SetCheckFreq") == 0)
            {
              // If it comes from the server it's in seconds
              check_interval_secs = value;

              #if DEBUG_FLAG
              Serial.print("IOT_TASK - Setting check interval too....: ");
              Serial.println(check_interval_secs);
              #endif
            }
            else if (strcmp(command, "SetSendFreq") == 0)
            {
              // If it comes from the server it's in seconds
              send_interval_secs = value;
              #if DEBUG_FLAG
              Serial.print("IOT_TASK - Setting send interval too....: ");
              Serial.println(send_interval_secs);
              #endif
            }
            else
            {
              // not a valid command. Nothing changes continue
            }
          }
        }

        // clear the results stuff.
        DJD_Obj.clear();
        results = "";
      }
      else
      {
// Not good, data not sent. Yield and try again.
#if DEBUG_FLAG
        Serial.println("IOT_TASK: Error something went wrong. Yielding, line 490!");
#endif
        taskYIELD();
      }
    }

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

    neopixel.three.rgbw.red = 25;
    neopixel.three.rgbw.blue = 0;
    neopixel.three.rgbw.green = 0;
    neopixel.three.rgbw.white = 0;

    neopixel.four.rgbw.red = 0;
    neopixel.four.rgbw.blue = 0;
    neopixel.four.rgbw.green = 0;
    neopixel.four.rgbw.white = 0;

    neopixel.rainbow_mode = false;

    // Send to NPQ
    if (xQueueSendToBack(NP_QUEUE, &neopixel, (TickType_t)0) == pdTRUE)
    {
#if DEBUG_FLAG
      Serial.println(F("IOT_TASK: Success sending to NPQ!"));
#endif
    }

    // Delay for other tasks.
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
  strip.setBrightness(50);
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
    } // End check for NP_QUEUE null

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
    }

    // Delay for other tasks
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
  xSemaphoreTake(HT_SEMAPHORE, 0);

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

    NeoPixel neopixel;
    // Set NeoPixels Color & brightness
    neopixel.one.rgbw.red = 0;
    neopixel.one.rgbw.blue = 0;
    neopixel.one.rgbw.green = 0;
    neopixel.one.rgbw.white = 75;

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

    // Send to NPQ
    if (xQueueSendToBack(NP_QUEUE, &neopixel, (TickType_t)0) == pdTRUE)
    {
#if DEBUG_FLAG
      Serial.println(F("HUMI_TEMP_TASK: Success sending to NPQ!"));
#endif
    }

    // Delay for other tasks
    vTaskDelay(pdMS_TO_TICKS(500));
  } // End of for loop
}

void STEPPER_MOTOR_TASK(void *pvParameters)
{
  // make pins input
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // init sm to default values
  stepper_motor sm;
  sm.forward = true;
  // Note: Steps per second = RPM * 2048 / 60
  sm.RPM = 0;

  // Create a stepper motor object
  // Set the motor to full step mode, 2048 total steps. (that what the 4 means) (8 means half step mode)
  AccelStepper my_motor(4, IN1, IN3, IN2, IN4);
  // Set max steps per second, roughly 4 RPM = 5 *2048 / 60 = 136.533333
  my_motor.setMaxSpeed(136);
  // Set current position to 0
  my_motor.setCurrentPosition(0);

  for (;;)
  {
    // run the motor. These needs to run whenever possible. At best once every ~1ms
    my_motor.run();

    // Check the SM_QUEUE
    if (SM_QUEUE != NULL)
    {
      if (xQueueReceive(SM_QUEUE, &sm, (TickType_t)0) == pdTRUE)
      {
#if DEBUG_FLAG
        Serial.print(F("SM_TASK: Success read from SM_QUEUE - "));
        Serial.print(sm.forward);
        Serial.print(F(" "));
        Serial.println(sm.RPM);
#endif
        if (sm.forward == true)
        {
          // Set the motor speed in steps per second
          my_motor.setSpeed((sm.RPM * 2048) / 60);
          my_motor.moveTo(2048);
        }
        else
        {
          my_motor.setSpeed(-(sm.RPM * 2048) / 60);
          my_motor.moveTo(-2048);
        }
      } // End of receving from SM_QUEUE
      else
      {
        if (sm.forward == true)
        {
          // Set the motor speed in steps per second
          my_motor.setSpeed((sm.RPM * 2048) / 60);
          my_motor.moveTo(2048);
        }
        else
        {
          my_motor.setSpeed(-(sm.RPM * 2048) / 60);
          my_motor.moveTo(-2048);
        }
      } // Tried receiving from SM_QUEUE, but failed
    }   //End of check SM_QUEUE for null

    NeoPixel neopixel;
    // Set NeoPixels Color & brightness
    neopixel.one.rgbw.red = 0;
    neopixel.one.rgbw.blue = 0;
    neopixel.one.rgbw.green = 0;
    neopixel.one.rgbw.white = 0;

    neopixel.two.rgbw.red = 0;
    neopixel.two.rgbw.blue = 0;
    neopixel.two.rgbw.green = 20;
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

    // Send to NPQ
    if (xQueueSendToBack(NP_QUEUE, &neopixel, (TickType_t)0) == pdTRUE)
    {
#if DEBUG_FLAG
      Serial.println(F("STEPPER_MOTOR_TASK: Success sending to NPQ!"));
#endif
    }

    // Delay for other tasks
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void handleRoot()
{
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

  neopixel.four.rgbw.red = 75;
  neopixel.four.rgbw.blue = 0;
  neopixel.four.rgbw.green = 0;
  neopixel.four.rgbw.white = 0;

  neopixel.rainbow_mode = false;

  // Send to NPQ
  if (xQueueSendToBack(NP_QUEUE, &neopixel, (TickType_t)0) == pdTRUE)
  {
#if DEBUG_FLAG
    Serial.println(F("handleRoot func: Success sending to NPQ!"));
#endif
  }

  char temp[1250];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 1250,

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
    <b>IOT_SERVER_AUTH_CODE:</b> %s </p>\
    <p><a href='/?dir=true'><button style='margin:10px;' id='0' class='button'>SM CW</button></a></p>\
    <p><a href='/?dir=false'><button style='margin:10px;' id='1' class='button'>SM CCW</button></a></p>\
    <p><a href='/?rpm=1'><button style='margin:10px;' id='2' class='button'>SM 1 RPM</button></a></p>\
    <p><a href='/?rpm=2'><button style='margin:10px;' id='3' class='button'>SM 2 RPM</button></a></p>\
    <p><a href='/?rpm=3'><button style='margin:10px;' id='4' class='button'>SM 3 RPM</button></a></p>\
    <p><a href='/?rpm=4'><button style='margin:10px;' id='5' class='button'>SM 4 RPM</button></a></p>\
    <p><a href='/?stop=true'><button style='margin:10px;' id='5' class='button'>STOP SM</button></a></p>\
  </body>\
</html>",

           hr, min % 60, sec % 60, IOT_SERVER_ADDR, IOT_SERVER_KEY, IOT_SERVER_KEY, auth_code);
  server.send(200, "text/html", temp);
}

void handleNotFound()
{
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
  neopixel.four.rgbw.green = 75;
  neopixel.four.rgbw.white = 0;

  neopixel.rainbow_mode = false;

  // Send to NPQ
  if (xQueueSendToBack(NP_QUEUE, &neopixel, (TickType_t)0) == pdTRUE)
  {
#if DEBUG_FLAG
    Serial.println(F("handleNotFound func: Success sending to NPQ!"));
#endif
  }

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
}

String iot_server_detect()
{
  HTTPClient http;
  String request_str = (String)IOT_SERVER_ADDR + "IOTAPI/" + "DetectServer";
  http.begin(request_str);
  http.addHeader("Content-Type", "application/json");
  String message = "{\"key\":\"" + (String)IOT_SERVER_KEY + "\"" + "}";
  int rtval = http.POST(message);

#if DEBUG_FLAG
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

#if DEBUG_FLAG
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

#if DEBUG_FLAG
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

#if DEBUG_FLAG
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

#if DEBUG_FLAG
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