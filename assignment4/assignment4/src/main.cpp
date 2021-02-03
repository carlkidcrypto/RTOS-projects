#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>

// Define the tasks
void LED_TASK(void *pvParameters);
void CNT_TASK(void *pvParameters);
void DR_TASK(void *pvParameters);
void DP_TASK(void *pvParameters);
void setup();
void loop();

// the setup function runs once when you press reset or power the board
void setup()
{

  Serial.begin(115200);
  // Define the Semaphores
  SemaphoreHandle_t LED_SEMAPHORE;
  SemaphoreHandle_t DP_SEMAPHORE;

  // Create the Semaphores
  LED_SEMAPHORE = xSemaphoreCreateBinary();
  if (LED_SEMAPHORE == NULL)
  {
    for (;;)
      Serial.println(F("LED_SEMAPHORE: Creation Error, no enough heap mem!"));
  }

  DP_SEMAPHORE = xSemaphoreCreateBinary();
  if (LED_SEMAPHORE == NULL)
  {
    for (;;)
      Serial.println(F("DP_SEMAPHORE: Creation Error, not enough heap mem!"));
  }

  BaseType_t xCNT_RTVAL = xTaskCreate(
      CNT_TASK, "CNT_TASK" // The counter task, 0-42, then 42-0
      ,
      128 // Stack size
      ,
      (void *)LED_SEMAPHORE // parameters
      ,
      1 // priority
      ,
      NULL);

  if (xCNT_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("CNT_TASK: Creation Error, not enough heap or stack!"));
  }

  BaseType_t xLED_RTVAL = xTaskCreate(
      LED_TASK, "LED_TASK" // Responsible for blinking LED on pin D13
      ,
      128 // Stack size
      ,
      (void *)LED_SEMAPHORE // parameters
      ,
      1 // priority
      ,
      NULL);

  if (xLED_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("LED_TASK: Creation Error, not enough heap or stack!"));
  }

  BaseType_t xDR_RTVAL = xTaskCreate(
      DR_TASK, "DR_TASK" // The Driver task.
      ,
      128 // Stack size
      ,
      (void *)&DP_SEMAPHORE // parameters
      ,
      1 // priority
      ,
      NULL);

  if (xDR_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("DR_TASK: Creation Error, not enough heap or stack!"));
  }

  BaseType_t xDP_RTVAL = xTaskCreate(
      DP_TASK, "DP_TASK" // The display task.
      ,
      128 // Stack size
      ,
      (void *)&DP_SEMAPHORE // parameters
      ,
      1 // priority
      ,
      NULL);

  if (xDP_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("DP_TASK: Creation Error, not enough heap or stack!"));
  }

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
  Serial.println(F("setup: Reached end of setup! Success!"));
}

void LED_TASK(void *pvParameters) // This is a task.
{
  // Get pvParameters out
  SemaphoreHandle_t led_semaphore = *((SemaphoreHandle_t *)pvParameters);

  // initialize digital pin 13 as an output.
  pinMode(13, OUTPUT);

  for (;;)
  {
    // Let's check if the semaphore is available
    if (xSemaphoreTake(led_semaphore, 0) == pdTRUE)
    {
      Serial.println(F("LED_TASK: semaphore taken!"));
      // we got it, let us do some stuff
      byte status = digitalRead(13);
      if (status)
        digitalWrite(13, LOW);
      else
        digitalWrite(13, HIGH);
    }
  }
}

void CNT_TASK(void *pvParameters) // This is a task.
{
  // Get pvParameters out
  SemaphoreHandle_t led_semaphore = *((SemaphoreHandle_t *)pvParameters);

  // First thing we do is take the semaphore
  xSemaphoreTake(led_semaphore, 0);

  for (;;)
  {
      // we got it, let us do some stuff

      vTaskDelay(pdMS_TO_TICKS(500));
      // we are done, give back the semaphore
      Serial.println(F("CNT_TASK: semaphore given!"));
      xSemaphoreGive(led_semaphore);
  }
}

void DR_TASK(void *pvParameters) // This is a task.
{
  // Get pvParameters out
  SemaphoreHandle_t *dp_semaphore;
  dp_semaphore = (SemaphoreHandle_t *)pvParameters;

  for (;;)
  {
  }
}

void DP_TASK(void *pvParameters) // This is a task.
{
  // Get pvParameters out
  SemaphoreHandle_t *dp_semaphore;
  dp_semaphore = (SemaphoreHandle_t *)pvParameters;

  for (;;)
  {
  }
}

void loop()
{
  // Empty. Things are done in Tasks.
}
