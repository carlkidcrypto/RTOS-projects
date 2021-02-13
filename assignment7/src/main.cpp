#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <Stepper.h>
#include <ClosedCube_HDC1080.h>
#define DEBUG_FLAG 0

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
SemaphoreHandle_t DP_SEMAPHORE;

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

  HTQ = xQueueCreate(5, sizeof(struct humi_temp));
  if (HTQ == NULL)
  {
    for (;;)
      Serial.println(F("HTQ: Creation Error, not enough heap mem!"));
  }

  SMQ = xQueueCreate(5, sizeof(struct stepper_motor));
  if (SMQ == NULL)
  {
    for (;;)
      Serial.println(F("SMQ: Creation Error, not enough heap mem!"));
  }

  DSQ = xQueueCreate(5, sizeof(byte));
  if (DSQ == NULL)
  {
    for (;;)
      Serial.println(F("DSQ: Creation Error, not enough heap mem!"));
  }

  BaseType_t xDS_RTVAL = xTaskCreate(
      DS_TASK, "DS_TASK", // The DIP Switch Task
      256 // Stack size
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
      tskIDLE_PRIORITY  // priority
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
      tskIDLE_PRIORITY // priority
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
      tskIDLE_PRIORITY + 2 // priority
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
        vTaskDelay(pdMS_TO_TICKS(250));
      }

      else
      {
#if DEBUG_FLAG
        Serial.print(F("DS_TASK: Failure sending value to DSQ! DSQ FULL! - "));
        Serial.println(DIPSW);
#endif

        // We didn't send our value, delay for next reading
        vTaskDelay(pdMS_TO_TICKS(250));
      }
    }
    else
      // DSQ failure, yield
      taskYIELD();
  }
}

void SM_TASK(void *pvParameters) // This is a task.
{
// Give our pins human names
#define IN1 30
#define IN2 32
#define IN3 34
#define IN4 36

// Define the max steps per rotation. See data sheet
#define MAXSTEPS 2048

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
  Stepper my_motor = Stepper(MAXSTEPS, IN1, IN3, IN2, IN4);
  //my_motor.setSpeed(sm.RPM);
  //my_motor.step(MAXSTEPS); // pos is forward, neg is reverse

  for (;;)
  {
    // Check the SMQ
    if(SMQ != NULL)
    {
      if (xQueueReceive(SMQ, &sm, (TickType_t)0) == pdTRUE)
      {
#if DEBUG_FLAG
        Serial.print(F("SM_TASK: Success read from SMQ - "));
        Serial.print(sm.forward);
        Serial.println(sm.RPM);
#endif

        // We read our value, delay for next value
        vTaskDelay(pdMS_TO_TICKS(250));
      }
      else
      {
#if DEBUG_FLAG
        Serial.println(F("SM_TASK: Failure reading from SMQ! SMQ Empty!"));
#endif
        // We didn't read our value, delay for next value
        vTaskDelay(pdMS_TO_TICKS(250));
      }
    }
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

  // Create an object our sensor
  ClosedCube_HDC1080 hdc1080;

  // Start up the sensor at i2c address
  hdc1080.begin(0x40);
  // Set sensor resolution, humidity, temp
  hdc1080.setResolution(HDC1080_RESOLUTION_14BIT, HDC1080_RESOLUTION_14BIT);

  for (;;)
  { 
  
	Serial.print("T=");
	Serial.print(hdc1080.readTemperature());
	Serial.print("C, RH=");
	Serial.print(hdc1080.readHumidity());
	Serial.println("%");
  vTaskDelay(pdMS_TO_TICKS(250));
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

void CONT_TASK(void *pvParameters)
{
  // Define char array for display
  char display_arr[3];

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

  // Define the byte to hol DIP Switch values
  byte DIPSW = 255;

  for (;;)
  {
    // Check all the queues and make sure they are valid
    if ((DSQ != NULL) && (SMQ != NULL) && (DRQ != NULL) && (HTQ !=NULL))
    {
      // Read from the DSQ
      if (xQueueReceive(DSQ, &DIPSW, (TickType_t)0) == pdTRUE)
      {
#if DEBUG_FLAG
        Serial.print(F("CONT_TASK: Success read from DSQ - "));
        Serial.println(DIPSW);
#endif
      }
      else
      {
#if DEBUG_FLAG
        Serial.println(F("CONT_TASK: Failure reading from DSQ! DSQ Empty!"));
#endif
      }

      // Read from HTQ
      if (xQueueReceive(HTQ, &ht, (TickType_t)0) == pdTRUE)
      {
#if DEBUG_FLAG
        Serial.print(F("CONT_TASK: Success read from HTQ - "));
        Serial.print(ht.humidity);
        Serial.println(ht.temperature);
#endif
      }
      else
      {
#if DEBUG_FLAG
        Serial.println(F("CONT_TASK: Failure reading from HTQ! HTQ Empty!"));
#endif
      }

      // Calculate values to send to DRQ and SMQ

      

    }

    // Delay for other tasks
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
void loop()
{
  // Empty. Things are done in Tasks.
}
