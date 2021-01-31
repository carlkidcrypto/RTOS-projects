#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <swRTC.h>

// define two tasks for Blink & AnalogRead
void TaskBlink(void *pvParameters);
void TaskAnalogRead(void *pvParameters);
void setup();
void loop();

swRTC rtc;

// the setup function runs once when you press reset or power the board
void setup()
{
  rtc.stopRTC();
  rtc.setTime(2,16,50);
  rtc.setDate(31,1,2021);
  rtc.startRTC();
  // Now set up two tasks to run independently.
  xTaskCreate(
      TaskBlink, "Blink" // A name just for humans
      ,
      128 // Stack size
      ,
      NULL, 2 // priority
      ,
      NULL);

  xTaskCreate(
      TaskAnalogRead, "AnalogRead", 256 // This stack size can be checked & adjusted by reading Highwater
      ,
      NULL, 1 // priority
      ,
      NULL);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBlink(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  // initialize digital pin 13 as an output.
  pinMode(13, OUTPUT);

  for (;;) // A Task shall never return or exit.
  {
    digitalWrite(13, HIGH);                // turn the LED on (HIGH is the voltage level)
    vTaskDelay(1000 / portTICK_PERIOD_MS); // wait for one second
    digitalWrite(13, LOW);                 // turn the LED off by making the voltage LOW
    vTaskDelay(1000 / portTICK_PERIOD_MS); // wait for one second
  }
}

void TaskAnalogRead(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  // initialize serial communication at 11520 bits per second:
  Serial.begin(115200);

  for (;;)
  {
    // Create a string to hold our info
    char mystr[60];
    // Add time to mystr
    sprintf(mystr, "My time: %d:%d:%d My date: %d/%d/%d", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds(), rtc.getMonth(), rtc.getDay(), rtc.getYear());
    // read the input on analog pin 0:
    int sensorValue = analogRead(A0);
    // Add value to mystr
    sprintf(mystr + strlen(mystr), " My sensor value: %d", sensorValue);
    // Print out mystr
    Serial.println(mystr);
    // Task Delay for ~1 second = 1000ms
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}