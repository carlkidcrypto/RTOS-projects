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
  Serial.begin(115200);

  byte hour = 0;
  byte min = 0;
  byte sec = 0;
  byte month = 0;
  byte day = 0;
  int year = 0;

  //Sun Jan 31 15:17:02 2021
  char compile_timestamp[] = __TIMESTAMP__;
  Serial.println(compile_timestamp);
  char *ptr = strtok(compile_timestamp, " ");
  char *temparray[5];
  int i=0;
  const char *months_arr[12] = {"Jan\0", "Feb\0", "Mar\0", "Apr\0", "May\0", "Jun\0", "Jul\0", "Aug\0", "Sep\0", "Oct\0", "Nov\0", "Dec\0"};
  while(ptr != NULL)
  {
    temparray[i++] = ptr;
    ptr = strtok(NULL, " ");
  }

  //for (i=0;i<4;i++)
  //  Serial.println(temparray[i]);

  for(unsigned int i=0;i<sizeof(*months_arr);i++)
  {
    // Find the month
    if(strcmp(temparray[1],months_arr[i]) == 0)
      month = i+1;
  }

  // Calc the day. Will be always be 0 - 31.
  int day_tens_place = ((int)temparray[2][0] - 48) * 10;
  int day_ones_place = ((int)temparray[2][1] - 48) * 1;
  day = day_tens_place + day_ones_place;

  // Calc the year. Will always be xxxx. First we convert to int and then subtract ASCII offset
  int year_thousands_place = ((int)temparray[4][0] - 48) * 1000;
  int year_hundreds_place = ((int)temparray[4][1] - 48) * 100;
  int year_tens_place = ((int)temparray[4][2] - 48) * 10;
  int year_ones_place = ((int)temparray[4][3] - 48) * 1;
  year = year_thousands_place + year_hundreds_place + year_tens_place + year_ones_place;

  // Now we need to parse and convert the time field. hr:min:secs, xx:xx:xx
  char *ptr2 = strtok(temparray[3], ":");
  char *temparray2[4];
  int k=0;
  while(ptr2 != NULL)
  {
    temparray2[k++] = ptr2;
    ptr2 = strtok(NULL, ":");
  }

  int hour_tens_place = ((int)temparray2[0][0] - 48) * 10;
  int hour_ones_place = ((int)temparray2[0][1] - 48) * 1;

  int minute_tens_place = ((int)temparray2[1][0] - 48) * 10;
  int minute_ones_place = ((int)temparray2[1][1] - 48) * 1;

  int second_tens_place = ((int)temparray2[2][0] - 48) * 10;
  int second_ones_place = ((int)temparray2[2][1] - 48) * 1;

  hour = hour_tens_place + hour_ones_place;
  min = minute_tens_place + minute_ones_place;
  sec = second_tens_place + second_ones_place;

  rtc.stopRTC();
  rtc.setTime(hour,min,sec);
  rtc.setDate(day,month,year);
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