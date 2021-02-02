#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

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
  xTaskCreate(
      LED_TASK, "LED_TASK" // Responsible for blinking LED on pin D13
      ,
      128 // Stack size
      ,
      NULL, 2 // priority
      ,
      NULL);

  xTaskCreate(
      CNT_TASK, "CNT_TASK", // The counter task, 0-42, then 42-0
      128 // stack size
      ,
      NULL, 1 // priority
      ,
      NULL);

  xTaskCreate(
      DR_TASK, "DR_TASK", // The Driver task.
      128 // stack size
      ,
      NULL, 1 // priority
      ,
      NULL);

  xTaskCreate(
      DP_TASK, "DP_TASK", // The display task.
      128 // stack size
      ,
      NULL, 1 // priority
      ,
      NULL);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void LED_TASK(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  for (;;)
  {
  }
}

void CNT_TASK(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  for (;;)
  {
  }
}

void DR_TASK(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  for (;;)
  {
  }
}

void DP_TASK(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  for (;;)
  {
  }
}

void loop()
{
  // Empty. Things are done in Tasks.
}
