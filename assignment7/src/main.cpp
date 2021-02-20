#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <AccelStepper.h>
#include <ClosedCube_HDC1080.h>
#define DEBUG_FLAG 1

// Define the tasks
void DS_TASK(void *pvParameters);   // DIP Switch Task
void SM_TASK(void *pvParameters);   // Stepper Motor Task
void HT_TASK(void *pvParameters);   // Humidity Temperature Task
void DR_TASK(void *pvParameters);   // Display Driver Task
void DP_TASK(void *pvParameters);   // Display Task
void CONT_TASK(void *pvParameters); // Controller Task
void setup();
void loop();

// Define the Semaphores
SemaphoreHandle_t DP_SEMAPHORE, DS_SEMAPHORE, SM_SEMAPHORE, HT_SEMAPHORE;

// Define the Queues
QueueHandle_t LQ, RQ, DRQ, HTQ, SMQ, DSQ; // Left Queue, Right Queue, Driver Queue, Humi/Temp Queue, Stepper Motor Queue, Dip Switch Queue

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

  DS_SEMAPHORE = xSemaphoreCreateBinary();
  if (DS_SEMAPHORE == NULL)
  {
    for (;;)
      Serial.println(F("DS_SEMAPHORE: Creation Error, not enough heap mem!"));
  }

  SM_SEMAPHORE = xSemaphoreCreateBinary();
  if (SM_SEMAPHORE == NULL)
  {
    for (;;)
      Serial.println(F("SM_SEMAPHORE: Creation Error, not enough heap mem!"));
  }

  HT_SEMAPHORE = xSemaphoreCreateBinary();
  if (HT_SEMAPHORE == NULL)
  {
    for (;;)
      Serial.println(F("HT_SEMAPHORE: Creation Error, not enough heap mem!"));
  }

  // Define the display struct
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

  // Define the stepper motor struct
  struct stepper_motor
  {
    bool forward;
    byte RPM;
  };

  // Define the humi/temp struct
  struct humi_temp
  {
    double humidity;
    double temperature;
  };

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

  DRQ = xQueueCreate(5, sizeof(display_arr));
  if (DRQ == NULL)
  {
    for (;;)
      Serial.println(F("DRQ: Creation Error, not enough heap mem!"));
  }

  HTQ = xQueueCreate(1, sizeof(struct humi_temp));
  if (HTQ == NULL)
  {
    for (;;)
      Serial.println(F("HTQ: Creation Error, not enough heap mem!"));
  }

  SMQ = xQueueCreate(1, sizeof(struct stepper_motor));
  if (SMQ == NULL)
  {
    for (;;)
      Serial.println(F("SMQ: Creation Error, not enough heap mem!"));
  }

  DSQ = xQueueCreate(1, sizeof(byte));
  if (DSQ == NULL)
  {
    for (;;)
      Serial.println(F("DSQ: Creation Error, not enough heap mem!"));
  }

  BaseType_t xDS_RTVAL = xTaskCreate(
      DS_TASK, "DS_TASK", // The DIP Switch Task
      256                 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 1 // priority
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
      tskIDLE_PRIORITY + 1 // priority
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
      tskIDLE_PRIORITY + 1 // priority
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
      tskIDLE_PRIORITY + 2 // priority
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
      tskIDLE_PRIORITY + 2 // priority
      ,
      NULL);

  if (xDP_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("DP_TASK: Creation Error, not enough heap or stack!"));
  }

  BaseType_t xCONT_RTVAL = xTaskCreate(
      CONT_TASK, "CONT_TASK" // The Controller Task
      ,
      256 // Stack size
      ,
      NULL // parameters
      ,
      tskIDLE_PRIORITY + 3 // priority
      ,
      NULL);

  if (xCONT_RTVAL != pdPASS)
  {
    for (;;)
      Serial.println(F("CONT_TASK: Creation Error, not enough heap or stack!"));
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

    // Check for the semaphore, is it there
    if (xSemaphoreTake(DS_SEMAPHORE, 0) == pdTRUE)
    {

      // Lets read the DIP Switch into a byte. bits are read from right to left, 7-0
      byte DIPSW = (digitalRead(DIP7) << 7) | (digitalRead(DIP6) << 6) | (digitalRead(DIP5) << 5) | (digitalRead(DIP4) << 4) | (digitalRead(DIP3) << 3) | (digitalRead(DIP2) << 2) | (digitalRead(DIP1) << 1) | (digitalRead(DIP0));

#if DEBUG_FLAG
      // Note: Our DIP switch shows value 1 for off, value 0 for on
      Serial.print(F("DS_TASK: DIP Switch Reading: "));
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
#endif

      // Send read value to DSQ
      if (DSQ != NULL)
      {
        if (xQueueSendToBack(DSQ, &DIPSW, (TickType_t)0) == pdTRUE)
        {
#if DEBUG_FLAG
          Serial.print(F("DS_TASK: Success sent value to DSQ! - "));
          Serial.println(DIPSW);
#endif

          // We sent our value, delay for next reading
          vTaskDelay(pdMS_TO_TICKS(100));
        }

        else
        {
#if DEBUG_FLAG
          Serial.print(F("DS_TASK: Failure sending value to DSQ! DSQ FULL! - "));
          Serial.println(DIPSW);
#endif

          // We didn't send our value, delay for next reading
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
      else
        // DSQ failure
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    // We don't have the semaphore
    else
      vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void SM_TASK(void *pvParameters) // This is a task.
{
// Give our pins human names
#define IN1 30
#define IN2 32
#define IN3 34
#define IN4 36

  // make pins input
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Define the stepper motor struct
  struct stepper_motor
  {
    bool forward;
    byte RPM;
  };

  // init sm to default values
  stepper_motor sm;
  sm.forward = true;
  sm.RPM = 15;

  // Create a stepper motor object
  AccelStepper my_motor(4, IN1, IN3, IN2, IN4);
  // Set max steps per second, roughly 16 RPM = STEPS/60 (SECONDS IN MIN)
  my_motor.setMaxSpeed(1000);
  // Set current position to 0
  my_motor.setCurrentPosition(0);

  for (;;)
  {
    my_motor.run();

    // Check for the semaphore, is it there
    if (xSemaphoreTake(SM_SEMAPHORE, 0) == pdTRUE)
    {

      // Check the SMQ
      if (SMQ != NULL)
      {
        if (xQueueReceive(SMQ, &sm, (TickType_t)0) == pdTRUE)
        {
#if DEBUG_FLAG
          Serial.print(F("SM_TASK: Success read from SMQ - "));
          Serial.print(sm.forward);
          Serial.print(F(" "));
          Serial.println(sm.RPM);
#endif
          if (sm.forward == true)
          { // In steps per second, RPM * 60 = Steps per second
            while (my_motor.currentPosition() != 4096)
            {
              my_motor.setSpeed(sm.RPM * 60);
              my_motor.runSpeed();
            }
          }
          else
          { // Negative is counter clockwise
            while (my_motor.currentPosition() != -4096)
            {
              my_motor.setSpeed(-(sm.RPM * 60));
              my_motor.runSpeed();
            }
          }
        }
        else
        {
#if DEBUG_FLAG
          Serial.println(F("SM_TASK: Failure reading from SMQ! SMQ Empty!"));
#endif
          // We didn't read our value, delay for next value
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
    }
    // We didn't get the semaphore
    else
      vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void HT_TASK(void *pvParameters) // This is a task.
{
  // Define the humi/temp struct
  struct humi_temp
  {
    double humidity;
    double temperature;
  };

  humi_temp HT_READINGS;

  // Create an object our sensor
  ClosedCube_HDC1080 hdc1080;

  // Start up the sensor at i2c address
  hdc1080.begin(0x40);
  // Set sensor resolution, humidity, temp
  hdc1080.setResolution(HDC1080_RESOLUTION_14BIT, HDC1080_RESOLUTION_14BIT);

  for (;;)
  {
    // Check for the semaphore, is it there
    if (xSemaphoreTake(HT_SEMAPHORE, 0) == pdTRUE)
    {
      // get the humi/temp values
      HT_READINGS.temperature = hdc1080.readTemperature();
      HT_READINGS.humidity = hdc1080.readHumidity();

      // Send read value to HTQ
      if (HTQ != NULL)
      {
        if (xQueueSendToBack(HTQ, &HT_READINGS, (TickType_t)0) == pdTRUE)
        {
#if DEBUG_FLAG
          Serial.print(F("HT_TASK: Success sent value to HTQ! - "));
          Serial.print(HT_READINGS.humidity);
          Serial.print(" - ");
          Serial.println(HT_READINGS.temperature);
#endif

          // We sent our value, delay for next reading
          vTaskDelay(pdMS_TO_TICKS(100));
        }

        else
        {
#if DEBUG_FLAG
          Serial.print(F("HT_TASK: Failure sending value to HTQ! HTQ FULL! - "));
          Serial.print(HT_READINGS.humidity);
          Serial.print(" - ");
          Serial.println(HT_READINGS.temperature);
#endif

          // We didn't send our value, delay for next reading
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
      else
        // DSQ failure
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    else
      vTaskDelay(pdMS_TO_TICKS(100));
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
      if (xQueueReceive(DRQ, &display_arr, (TickType_t)0) == pdPASS)
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
          xQueueSend(LQ, &left, (TickType_t)0);
          xQueueSend(RQ, &right, (TickType_t)0);

// Now we give DP_SEMAPHORE
#if DEBUG_FLAG
          Serial.println(F("DR_TASK: semaphore given!"));
#endif
          xSemaphoreGive(DP_SEMAPHORE);
        }
        else
          vTaskDelay(pdMS_TO_TICKS(100));
      }
      else
        // failed to get item out of queue, Yield
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    else
      // Queue is full or something... Yield
      vTaskDelay(pdMS_TO_TICKS(100));
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
        if ((xQueueReceive(LQ, &left, (TickType_t)0) == pdPASS) && (xQueueReceive(RQ, &right, (TickType_t)0) == pdPASS))
        {
#if DEBUG_FLAG
          Serial.print(F("DP_TASK: Received from  LQ - "));
          Serial.print(left.A);
          Serial.print(left.B);
          Serial.print(left.C);
          Serial.print(left.D);
          Serial.print(left.E);
          Serial.print(left.F);
          Serial.print(left.G);
          Serial.println(left.DP);

          Serial.print(F("DP_TASK: Received from  RQ - "));
          Serial.print(right.A);
          Serial.print(right.B);
          Serial.print(right.C);
          Serial.print(right.D);
          Serial.print(right.E);
          Serial.print(right.F);
          Serial.print(right.G);
          Serial.println(right.DP);
#endif
        }
        else
          // We tried to grab the number but failed.  Yield anyway
          vTaskDelay(1);
      }
      else
        // Nothing for us to do, yield again
        vTaskDelay(1);
    }
    else
      // Semaphore isn't available lets yield for other tasks
      vTaskDelay(1);
  }
}

void CONT_TASK(void *pvParameters)
{
  // Define char array for display
  char display_arr[3];
  sprintf(display_arr, "%02d", 0);

  // Define the stepper motor struct
  struct stepper_motor
  {
    bool forward;
    byte RPM;
  };

  // init sm to some default values
  stepper_motor sm;
  sm.forward = true;
  sm.RPM = 15;

  // Define the humi/temp struct
  struct humi_temp
  {
    double humidity;
    double temperature;
  };

  // init ht to some default values
  humi_temp ht;
  ht.humidity = 0;
  ht.temperature = 0;

  // Define the byte to hold DIP Switch values
  byte DIPSW = 255;

  // Map valid DIP Switch inputs
  enum states
  {
    OVERLOAD = 0,
    DPSW4 = 247,
    DPSW2and3 = 249,
    DPSW3 = 251,
    DPSW2 = 253,
    DPSW1_ON = 254,
    DPSW1_OFF = 255
  };

  // Create the FSM
  states FSM;
  // init FSM
  FSM = states(DIPSW);

  for (;;)
  {
    // Check all the queues and make sure they are valid
    if ((DSQ != NULL) && (SMQ != NULL) && (DRQ != NULL) && (HTQ != NULL))
    {
      // Grab fresh data from DSQ and HTQ
      if ((xQueueReceive(DSQ, &DIPSW, (TickType_t)0) == pdTRUE) && (xQueueReceive(HTQ, &ht, (TickType_t)0) == pdTRUE))
      {
#if DEBUG_FLAG
        Serial.println(F("CONT_TASK: Success read from DSQ and HTQ"));
        Serial.print(F("CONT_TASK: DIPSW - "));
        Serial.println(DIPSW);
        Serial.print(F("CONT_TASK: Temp, Humi - "));
        Serial.print(ht.temperature);
        Serial.print(F(" "));
        Serial.println(ht.humidity);
#endif

        // We have new values, we need to find any changes
        // Update the FSM state from the DIP Switch value
        FSM = states(DIPSW);

        // check the FSM value and do stuff.
        switch (FSM)
        {
        case OVERLOAD:
#if DEBUG_FLAG
          Serial.println(F("CONT_TASK: Entered OVERLOAD state!"));
#endif
          break;

        case DPSW1_ON:
#if DEBUG_FLAG
          Serial.println(F("CONT_TASK: Entered DPSW1_ON state!"));
#endif
          // Move stepper on humidity / Off on Temperature
          sm.forward = true;
          if (ht.humidity >= 0.00 && ht.humidity <= 10.00)
            sm.RPM = 3;

          else if (ht.humidity >= 10.01 && ht.humidity <= 20.00)
            sm.RPM = 6;

          else if (ht.humidity >= 30.01 && ht.humidity <= 40.00)
            sm.RPM = 9;

          else if (ht.humidity >= 50.01 && ht.humidity <= 60.00)
            sm.RPM = 12;

          else
            sm.RPM = 15;

          // Send to SMQ
          if (xQueueSendToBack(SMQ, &sm, (TickType_t)0) == pdTRUE)
          {
#if DEBUG_FLAG
            Serial.print(F("CONT_TASK: Success sending to SMQ - "));
            Serial.print(sm.forward);
            Serial.print(F(" "));
            Serial.println(sm.RPM);
#endif
          }

          // Send number to DRQ
          sprintf(display_arr, "%02d", int(ht.humidity));
          if (xQueueSendToBack(DRQ, &display_arr, (TickType_t)0) == pdTRUE)
          {
#if DEBUG_FLAG
            Serial.print(F("CONT_TASK: Success sending to DRQ - "));
            Serial.println(display_arr);

#endif
            // We have finished with this state! Give Semaphore to SM_TASK
            xSemaphoreGive(SM_SEMAPHORE);
          }

          break;

        case DPSW1_OFF:
#if DEBUG_FLAG
          Serial.println(F("CONT_TASK: Entered DPSW1_OFF state!"));
#endif
          // Move stepper on humidity / Off on Temperature
          // NOTE: Max RPM is 15 since we are only running on 5 V
          // also note temp is in degrees Celcius
          if (ht.temperature >= 0.00 && ht.temperature <= 10.00)
            sm.RPM = 3;

          else if (ht.temperature >= 10.01 && ht.temperature <= 20.00)
            sm.RPM = 6;

          else if (ht.temperature >= 20.01 && ht.temperature <= 30.00)
            sm.RPM = 9;

          else if (ht.temperature >= 30.01 && ht.temperature <= 40.00)
            sm.RPM = 12;

          else
            sm.RPM = 15;

          if (xQueueSendToBack(SMQ, &sm, (TickType_t)0) == pdTRUE)
          {
#if DEBUG_FLAG
            Serial.print(F("CONT_TASK: Success sending to SMQ - "));
            Serial.print(sm.forward);
            Serial.print(F(" "));
            Serial.println(sm.RPM);
#endif
            // We have finished with this state! Give Semaphore to SM_TASK
            xSemaphoreGive(SM_SEMAPHORE);
          }

          // Send number to DRQ
          sprintf(display_arr, "%02d", int(ht.temperature));
          if (xQueueSendToBack(DRQ, &display_arr, (TickType_t)0) == pdTRUE)
          {
#if DEBUG_FLAG
            Serial.print(F("CONT_TASK: Success sending to DRQ - "));
            Serial.println(display_arr);
            Serial.println(F("CONT_TASK: Giving DP_SEMAPHORE!"));

#endif
            xSemaphoreGive(DP_SEMAPHORE);
          }

          break;

        case DPSW2:
#if DEBUG_FLAG
          Serial.println(F("CONT_TASK: Entered DPSW2 state!"));
#endif
          // Overrides 1 and just continuously moves stepper clockwise
          sm.forward = true;
          sm.RPM = 15;
          // Send to SMQ
          if (xQueueSendToBack(SMQ, &sm, (TickType_t)0) == pdTRUE)
          {
#if DEBUG_FLAG
            Serial.print(F("CONT_TASK: Success sending to SMQ - "));
            Serial.print(sm.forward);
            Serial.print(F(" "));
            Serial.println(sm.RPM);
#endif
            // We have finished with this state! Give Semaphore to SM_TASK
            xSemaphoreGive(SM_SEMAPHORE);
          }

          break;

        case DPSW3:
#if DEBUG_FLAG
          Serial.println(F("CONT_TASK: Entered DPSW3 state!"));
#endif
          // Override's 1 and just continuously moves stepper counterclockwise
          sm.forward = false;
          sm.RPM = 15;
          // Send to SMQ
          if (xQueueSendToBack(SMQ, &sm, (TickType_t)0) == pdTRUE)
          {
#if DEBUG_FLAG
            Serial.print(F("CONT_TASK: Success sending to SMQ - "));
            Serial.print(sm.forward);
            Serial.print(F(" "));
            Serial.println(sm.RPM);
#endif
            // We have finished with this state! Give Semaphore to SM_TASK
            xSemaphoreGive(SM_SEMAPHORE);
          }

          break;

        case DPSW4:
#if DEBUG_FLAG
          Serial.println(F("CONT_TASK: Entered DPSW4 state!"));
#endif
          // stops everything and overrides DP1, DP2 and DP3
          sm.RPM = 0;
          sm.forward = true;

          // Send to SMQ
          if (xQueueSendToBack(SMQ, &sm, (TickType_t)0) == pdTRUE)
          {
#if DEBUG_FLAG
            Serial.print(F("CONT_TASK: Success sending to SMQ - "));
            Serial.print(sm.forward);
            Serial.print(F(" "));
            Serial.println(sm.RPM);
#endif
          // We have finished with this state! Give Semaphore to SM_TASK
          xSemaphoreGive(SM_SEMAPHORE);
          }
          break;

        case DPSW2and3:
#if DEBUG_FLAG
          Serial.println(F("CONT_TASK: Entered DPSW2and3 state!"));
#endif
          // then do one revolution clockwise and one counterclockwise and repeat

          // do one revolution at max speed
          sm.RPM = 15;
          sm.forward = true;

          // send to SMQ and delay a bit. Wait for completion.
          // send to SMQ
          break;

        default:
#if DEBUG_FLAG
          Serial.println(F("CONT_TASK: Entered default state!"));
#endif
          // invalid inputs don't change state
          // Send to SMQ
          if (xQueueSendToBack(SMQ, &sm, (TickType_t)0) == pdTRUE)
          {
#if DEBUG_FLAG
            Serial.print(F("CONT_TASK: Success sending to SMQ - "));
            Serial.print(sm.forward);
            Serial.print(F(" "));
            Serial.println(sm.RPM);
#endif
            // We have finished with this state! Give Semaphore to SM_TASK
            xSemaphoreGive(SM_SEMAPHORE);
          }

          break;
        } // End of switch statement
      } // End of XQueueReceive
      // We couldn't grab fresh data so request it
      else
      {
#if DEBUG_FLAG
        Serial.println(F("CONT_TASK: Giving semaphores - DS, HT"));
#endif
        xSemaphoreGive(DS_SEMAPHORE);
        xSemaphoreGive(HT_SEMAPHORE);
        // Delay for other tasks
        vTaskDelay(pdMS_TO_TICKS(500));
      }
    }
  }
}
void loop()
{
  // Empty. Things are done in Tasks.
}
