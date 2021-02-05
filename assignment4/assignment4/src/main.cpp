#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#define DEBUG_FLAG 0

// Define the tasks
void LED_TASK(void *pvParameters);
void CNT_TASK(void *pvParameters);
void DR_TASK(void *pvParameters);
void DP_TASK(void *pvParameters);
void setup();
void loop();

// Define the Semaphores
SemaphoreHandle_t LED_SEMAPHORE, DP_SEMAPHORE;

// Define the Queues
QueueHandle_t LQ, RQ, CNTQ;

// the setup function runs once when you press reset or power the board
void setup()
{

  Serial.begin(115200);

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

  // Define the structs
  struct display
  {
    bool A;
    bool B;
    bool C;
    bool D;
    bool E;
    bool F;
    bool G;
  };

  LQ = xQueueCreate(10, sizeof(struct display));
  if (LQ == NULL)
  {
    for (;;)
      Serial.println(F("LQ: Creation Error, not enough heap mem!"));
  }

  RQ = xQueueCreate(10, sizeof(struct display));
  if (RQ == NULL)
  {
    for (;;)
      Serial.println(F("RQ: Creation Error, not enough heap mem!"));
  }

  CNTQ = xQueueCreate(1, sizeof(byte));
  if (CNTQ == NULL)
  {
    for (;;)
      Serial.println(F("CNTQ: Creation Error, not enough heap mem!"));
  }

  // Send stuff to temp queue TQ
  //xQueueSend(TQ, &LED_SEMAPHORE, 0);

  BaseType_t xCNT_RTVAL = xTaskCreate(
      CNT_TASK, "CNT_TASK" // The counter task, 0-42, then 42-0
      ,
      256 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 2 // priority
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
      256 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY // priority
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
      256 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 1 // priority
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
      256 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 1 // priority
      ,
      NULL);

  if (xDP_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("DP_TASK: Creation Error, not enough heap or stack!"));
  }

// Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
#if DEBUG_FLAG
  Serial.println(F("setup: Reached end of setup! Success!"));
#endif
}

void LED_TASK(void *pvParameters) // This is a task.
{

  // initialize digital pin 13 as an output.
  pinMode(13, OUTPUT);

  for (;;)
  {
    // Let's check if the semaphore is available
    if (xSemaphoreTake(LED_SEMAPHORE, 0) == pdTRUE)
    {
#if DEBUG_FLAG
      Serial.println(F("LED_TASK: semaphore taken!"));
#endif
      // we got it, let us do some stuff
      byte status = digitalRead(13);
      if (status)
        digitalWrite(13, LOW);
      else
        digitalWrite(13, HIGH);
    }
    else
      // Semaphore isn't available lets yield for other tasks
      taskYIELD();
  }
}

void CNT_TASK(void *pvParameters) // This is a task.
{
  // Our Counter Variable
  byte Count = 0;
  // Are we counting
  bool Forward = true;

  for (;;)
  {
    if (Forward)
      Count++;
    else
      Count--;

    if (Forward == true && Count >= 42)
      Forward = false;
    else if (Forward == false && Count == 0)
      Forward = true;
#if DEBUG_FLAG
    Serial.print(F("CNT_TASK: Sending number to CNTQ - "));
    Serial.println(Count);
    Serial.println(F("CNT_TASK: semaphore given!"));
#endif
    // we are done, give semaphore, and delay
    xSemaphoreGive(LED_SEMAPHORE);

    // we sent the number to the CNTQ (counter queue)
    if (CNTQ != NULL)
    {
      // Block for 10 ticks if CNTQ is full
      xQueueSend(CNTQ, &Count, (TickType_t)10);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void DR_TASK(void *pvParameters) // This is a task.
{
  byte number_to_display;
  // Define the struct
  struct display
  {
    bool A;
    bool B;
    bool C;
    bool D;
    bool E;
    bool F;
    bool G;
  };

  struct display left;
  struct display right;

  for (;;)
  {
    if (CNTQ != NULL)
    {
      if (xQueueReceive(CNTQ, &number_to_display, (TickType_t)10) == pdPASS)
      {
#if DEBUG_FLAG
        Serial.print(F("DR_TASK: Received number from CNTQ - "));
        Serial.println(number_to_display);
#endif

        // Let's encode the number we got into the display struct
        if (number_to_display == 0)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 1)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 2)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = false;
          right.G = true;
        }
        else if (number_to_display == 3)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = false;
          right.G = true;
        }
        else if (number_to_display == 4)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 5)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 6)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 7)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 8)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 9)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 10)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
        }
        else if (number_to_display == 11)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 12)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = false;
          right.G = true;
        }
        else if (number_to_display == 13)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = false;
          right.G = true;
        }
        else if (number_to_display == 14)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 15)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 16)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 17)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 18)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 19)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 20)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
        }
        else if (number_to_display == 21)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 22)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = false;
          right.G = true;
        }
        else if (number_to_display == 23)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = false;
          right.G = true;
        }
        else if (number_to_display == 24)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 25)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 26)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 27)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 28)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 29)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 30)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
        }
        else if (number_to_display == 31)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 32)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = false;
          right.G = true;
        }
        else if (number_to_display == 33)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = false;
          right.G = true;
        }
        else if (number_to_display == 34)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 35)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 36)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 37)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 38)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 39)
        {
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
        }
        else if (number_to_display == 40)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = true;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
        }
        else if (number_to_display == 41)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = true;
          left.G = true;

          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
        }
        else if (number_to_display == 42)
        {
          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = true;
          left.G = true;

          // Load right display
          right.A = true;
          right.B = true;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = false;
          right.G = true;
        }

        // Once we get here left and right should be loaded
        if (LQ != NULL && RQ != NULL)
        {
          // load both queues up
          xQueueSend(LQ, &left, (TickType_t)10);
          xQueueSend(RQ, &right, (TickType_t)10);

// Now we give DP_SEMAPHORE
#if DEBUG_FLAG
          Serial.println(F("DR_TASK: semaphore given!"));
#endif
          xSemaphoreGive(DP_SEMAPHORE);
        }
        else
          taskYIELD();
      }
      else
        taskYIELD();
    }
    else
      taskYIELD();
  }
}

void DP_TASK(void *pvParameters) // This is a task.
{
  // Init segments A - G pins as outputs
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);

  // Init left and right display as outputs
  pinMode(44, OUTPUT);
  pinMode(46, OUTPUT);

  // Start with all Zeros 00
  digitalWrite(44, LOW);
  digitalWrite(46, LOW);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);

  // Define the struct
  struct display
  {
    bool A;
    bool B;
    bool C;
    bool D;
    bool E;
    bool F;
    bool G;
  };

  // Create left and right struct
  struct display left;
  struct display right;
  // init struct to false;
  left.A = false;
  left.B = false;
  left.C = false;
  left.D = false;
  left.E = false;
  left.F = false;
  left.G = false;

  right.A = false;
  right.B = false;
  right.C = false;
  right.D = false;
  right.E = false;
  right.F = false;
  right.G = false;

  for (;;)
  {
    // Clear both
    digitalWrite(44, HIGH);
    digitalWrite(46, HIGH);

    // Set left display
    digitalWrite(44, LOW);
    digitalWrite(4, left.A);
    digitalWrite(5, left.B);
    digitalWrite(6, left.C);
    digitalWrite(7, left.D);
    digitalWrite(8, left.E);
    digitalWrite(9, left.F);
    digitalWrite(10, left.G);
    vTaskDelay(pdMS_TO_TICKS(17));
    digitalWrite(44, HIGH);

    // Set right display
    digitalWrite(46, LOW);
    digitalWrite(4, right.A);
    digitalWrite(5, right.B);
    digitalWrite(6, right.C);
    digitalWrite(7, right.D);
    digitalWrite(8, right.E);
    digitalWrite(9, right.F);
    digitalWrite(10, right.G);
    vTaskDelay(pdMS_TO_TICKS(17));
    digitalWrite(46, HIGH);
  
    // Let's check if the semaphore is available
    if (xSemaphoreTake(DP_SEMAPHORE, 0) == pdTRUE)
    {

#if DEBUG_FLAG
      Serial.println(F("DP_TASK: semaphore taken!"));
#endif
      if (LQ != NULL && RQ != NULL)
      {
        // Check and grab data from both left and right queues
        if ((xQueueReceive(LQ, &left, (TickType_t)10) == pdPASS) && (xQueueReceive(RQ, &right, (TickType_t)10) == pdPASS))
        {
#if DEBUG_FLAG
          Serial.print(F("DP_TASK: Received from  LQ - "));
          Serial.print(left.A);
          Serial.print(left.B);
          Serial.print(left.C);
          Serial.print(left.D);
          Serial.print(left.E);
          Serial.print(left.F);
          Serial.println(left.G);

          Serial.print(F("DP_TASK: Received from  RQ - "));
          Serial.print(right.A);
          Serial.print(right.B);
          Serial.print(right.C);
          Serial.print(right.D);
          Serial.print(right.E);
          Serial.print(right.F);
          Serial.println(right.G);
#endif
        }
      }
    }
    else
      // Semaphore isn't available lets yield for other tasks
      taskYIELD();
  }
}

void loop()
{
  // Empty. Things are done in Tasks.
}
