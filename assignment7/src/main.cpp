#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#define DEBUG_FLAG 1

// Define the tasks
void DS_TASK(void *pvParameters); // DIP Switch Task
void SM_TASK(void *pvParameters); // Stepper Motor Task
void HT_TASK(void *pvParameters); // Humidity Temperature Task
void DR_TASK(void *pvParameters); // Display Driver Task
void DP_TASK(void *pvParameters); // Display Task
void setup();
void loop();

// Define the Semaphores
SemaphoreHandle_t DP_SEMAPHORE;

// Define the Queues
QueueHandle_t LQ, RQ, DRQ; // Left Queue, Right Queue, Driver Queue

// the setup function runs once when you press reset or power the board
void setup()
{

  Serial.begin(115200);

  // Create the Semaphores
  DP_SEMAPHORE = xSemaphoreCreateBinary();
  if (DP_SEMAPHORE == NULL)
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
    bool DP;
  };

  // Define char array for display
  char display_arr[3];

  LQ = xQueueCreate(1, sizeof(struct display));
  if (LQ == NULL)
  {
    for (;;)
      Serial.println(F("LQ: Creation Error, not enough heap mem!"));
  }

  RQ = xQueueCreate(1, sizeof(struct display));
  if (RQ == NULL)
  {
    for (;;)
      Serial.println(F("RQ: Creation Error, not enough heap mem!"));
  }

  DRQ = xQueueCreate(1, sizeof(display_arr));
  if (DRQ == NULL)
  {
    for (;;)
      Serial.println(F("DRQ: Creation Error, not enough heap mem!"));
  }

  BaseType_t xDS_RTVAL = xTaskCreate(
      DS_TASK, "DS_TASK",
      256 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY // priority
      ,
      NULL);

  if (xDS_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("DS_TASK: Creation Error, not enough heap or stack!"));
  }

  BaseType_t xSM_RTVAL = xTaskCreate(
      SM_TASK, "SM_TASK",
      256 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY // priority
      ,
      NULL);

  if (xSM_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("SM_TASK: Creation Error, not enough heap or stack!"));
  }

  BaseType_t xHT_RTVAL = xTaskCreate(
      HT_TASK, "HT_TASK",
      256 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY // priority
      ,
      NULL);

  if (xHT_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("HT_TASK: Creation Error, not enough heap or stack!"));
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

void DS_TASK(void *pvParameters) // This is a task.
{
// Give our pins human names
#define DIP0 53
#define DIP1 51
#define DIP2 49
#define DIP3 47
#define DIP4 45
#define DIP5 43
#define DIP6 41
#define DIP7 39

  // make pins input
  pinMode(DIP0, INPUT);
  pinMode(DIP1, INPUT);
  pinMode(DIP2, INPUT);
  pinMode(DIP3, INPUT);
  pinMode(DIP4, INPUT);
  pinMode(DIP5, INPUT);
  pinMode(DIP6, INPUT);
  pinMode(DIP7, INPUT);

  for (;;)
  {
#if DEBUG_FLAG
    // Note: Our DIP switch shows value 1 for off, value 0 for on
    Serial.print(F("DIP Switch Reading: "));
    byte DIPSW = (digitalRead(DIP7) << 7) | (digitalRead(DIP6) << 6) | (digitalRead(DIP5) << 5) | (digitalRead(DIP4) << 4) | (digitalRead(DIP3) << 3) | (digitalRead(DIP2) << 2) | (digitalRead(DIP1) << 1) | (digitalRead(DIP0));
    Serial.print(DIPSW);
    Serial.print(" : ");
    Serial.print(digitalRead(DIP7));
    Serial.print(digitalRead(DIP6));
    Serial.print(digitalRead(DIP5));
    Serial.print(digitalRead(DIP4));
    Serial.print(digitalRead(DIP3));
    Serial.print(digitalRead(DIP2));
    Serial.print(digitalRead(DIP1));
    Serial.println(digitalRead(DIP0));
    vTaskDelay(pdMS_TO_TICKS(500));
#endif
  }
}

void SM_TASK(void *pvParameters) // This is a task.
{

  for (;;)
  {
  }
}

void HT_TASK(void *pvParameters) // This is a task.
{
  for (;;)
  {
  }
}

void DR_TASK(void *pvParameters) // This is a task.
{
  // Define display array
  char display_arr[3];

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
    bool DP;
  };

  struct display left;
  struct display right;

  for (;;)
  {
    if (DRQ != NULL)
    {
      if (xQueueReceive(DRQ, &display_arr, (TickType_t)10) == pdPASS)
      {
#if DEBUG_FLAG
        Serial.print(F("DR_TASK: Received number from DRQ - "));
        Serial.println(display_arr);
#endif

        // Let's encode the left digit first
        switch (display_arr[0])
        {

        /***** BEGIN: Cases for all base ten digits 0 - 9 *****/
        case '0':

          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;
          left.DP = false;
          break;

        case '1':

          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;
          left.DP = false;
          break;
        case '2':

          // Load left display
          left.A = true;
          left.B = true;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = false;
          left.G = true;
          left.DP = false;
          break;
        case '3':

          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = false;
          left.G = true;
          left.DP = false;
          break;
        case '4':

          // Load left display
          left.A = false;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;
        case '5':
          // Load left display
          left.A = true;
          left.B = false;
          left.C = true;
          left.D = true;
          left.E = false;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;
        case '6':
          // Load left display
          left.A = true;
          left.B = false;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;
        case '7':
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;
          left.DP = false;
          break;
        case '8':

          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;
        case '9':

          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = false;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;

          /***** END: Cases for all base ten digits 0 - 9 *****/

          /***** BEGIN: Cases for all base sixteen digits A - F *****/
          // Upper Case Cases
        case 'A':
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;

        case 'B':
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = true;
          break;

        case 'C':
          // Load left display
          left.A = true;
          left.B = false;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;
          left.DP = false;
          break;

        case 'D':
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;
          left.DP = true;
          break;

        case 'E':
          // Load left display
          left.A = true;
          left.B = false;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;

        case 'F':
          // Load left display
          left.A = true;
          left.B = false;
          left.C = false;
          left.D = false;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;

        // Lower Case Cases
        case 'a':
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = false;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;

        case 'b':
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = true;
          break;

        case 'c':
          // Load left display
          left.A = true;
          left.B = false;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;
          left.DP = false;
          break;

        case 'd':
          // Load left display
          left.A = true;
          left.B = true;
          left.C = true;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = false;
          left.DP = true;
          break;

        case 'e':
          // Load left display
          left.A = true;
          left.B = false;
          left.C = false;
          left.D = true;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;

        case 'f':
          // Load left display
          left.A = true;
          left.B = false;
          left.C = false;
          left.D = false;
          left.E = true;
          left.F = true;
          left.G = true;
          left.DP = false;
          break;
          /***** END: Cases for all base sixteen digits A - F *****/

        // Nothing Matched
        default:
          // Load left display
          left.A = false;
          left.B = false;
          left.C = false;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;
          left.DP = false;
          break;
        }

        // Let's encode the second digit last
        switch (display_arr[1])
        {

        /***** BEGIN: Cases for all base ten digits 0 - 9 *****/
        case '0':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
          right.DP = false;
          break;

        case '1':
          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
          right.DP = false;
          break;
        case '2':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = false;
          right.G = true;
          right.DP = false;
          break;
        case '3':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = false;
          right.G = true;
          right.DP = false;
          break;
        case '4':
          // Load right display
          right.A = false;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;
        case '5':
          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = false;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;
        case '6':
          // Load right display
          right.A = true;
          right.B = false;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;
        case '7':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = false;
          right.G = false;
          right.DP = false;
          break;
        case '8':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;
        case '9':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = false;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;

          /***** END: Cases for all base ten digits 0 - 9 *****/

          /***** BEGIN: Cases for all base sixteen digits A - F *****/

        // Upper Case Cases
        case 'A':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;

        case 'B':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = true;
          break;

        case 'C':
          // Load right display
          right.A = true;
          right.B = false;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
          right.DP = false;
          break;

        case 'D':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
          right.DP = false;
          break;

        case 'E':
          // Load right display
          right.A = true;
          right.B = false;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;

        case 'F':
          // Load right display
          right.A = true;
          right.B = false;
          right.C = false;
          right.D = false;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;

        // Lower Case Cases
        case 'a':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = false;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;

        case 'b':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = true;
          break;

        case 'c':
          // Load right display
          right.A = true;
          right.B = false;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
          right.DP = false;
          break;

        case 'd':
          // Load right display
          right.A = true;
          right.B = true;
          right.C = true;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = false;
          right.DP = true;
          break;

        case 'e':
          // Load right display
          right.A = true;
          right.B = false;
          right.C = false;
          right.D = true;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;

        case 'f':
          // Load right display
          right.A = true;
          right.B = false;
          right.C = false;
          right.D = false;
          right.E = true;
          right.F = true;
          right.G = true;
          right.DP = false;
          break;

          /***** END: Cases for all base sixteen digits A - F *****/

        // Nothing Matched
        default:
          // Load left display
          left.A = false;
          left.B = false;
          left.C = false;
          left.D = false;
          left.E = false;
          left.F = false;
          left.G = false;
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
        // failed to get item out of queue, Yield
        taskYIELD();
    }
    else
      // Queue is full or something... Yield
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
  pinMode(11, OUTPUT);

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
    bool DP;
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
  left.DP = false;

  right.A = false;
  right.B = false;
  right.C = false;
  right.D = false;
  right.E = false;
  right.F = false;
  right.G = false;
  right.DP = false;

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
    digitalWrite(11, left.DP);
    vTaskDelay(1);
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
    digitalWrite(11, right.DP);
    vTaskDelay(1);
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
          Serial.println(left.DP);

          Serial.print(F("DP_TASK: Received from  RQ - "));
          Serial.print(right.A);
          Serial.print(right.B);
          Serial.print(right.C);
          Serial.print(right.D);
          Serial.print(right.E);
          Serial.print(right.F);
          Serial.println(right.G);
          Serial.println(right.DP);
#endif
        }
        else
          // We tried to grab the number but failed.  Yield anyway
          taskYIELD();
      }
      else
        // Nothing for us to do, yield again
        taskYIELD();
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
