#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>
#include <ClosedCube_HDC1080.h>
#include <Adafruit_NeoPixel.h>

#define DEBUG_FLAG 0
#define NEO_PIXEL_PIN 21
#define NUM_LEDS 4
#define IN1 33
#define IN2 4
#define IN3 32
#define IN4 14

const char *SSID = "";
const char *PASSWORD = "";
const char *IOT_SERVER_ADDR = "";
const char *IOT_SERVER_KEY = "";
const char *IOT_SERVER_ID = "";
const char *auth_code = "";
WebServer server(80);

/***** Begin: Define tasks/functions *****/
void handleRoot();
void handleNotFound();
void setup();
void loop();
String iot_server_detect();
String iot_server_register();
String iot_server_send_data(double data_temp, double data_humidity);
String iot_server_query_commands();
String iot_server_shutdown();
void WEB_SERVER_TASK(void *pvParameters);
void IOT_TASK(void *pvParameters);
void NEO_PIXEL_TASK(void *pvParameters);
void HUMI_TEMP_TASK(void *pvParameters);
void STEPPER_MOTOR_TASK(void *pvParameters);
/***** End: Define tasks/functions *****/

/***** Begin: Define Semaphores/Queues *****/

// Create the semaphores for humi/temp
SemaphoreHandle_t HT_SEMAPHORE;

// Create the stepper motor, and humi/temp queues
QueueHandle_t HT_QUEUE, SM_QUEUE, NP_QUEUE;
/***** end: efine Semaphores/Queues *****/

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

// Define the NeoPixel struct
struct NeoPixel
{
    struct PIXEL
    {
        struct RGBW
        {
            // Note: the value set for each color is also the brightness level. 255 max, 0 off
            byte red;
            byte green;
            byte blue;
            byte white;
        };
        RGBW rgbw;
    };
    PIXEL one, two, three, four;

    bool rainbow_mode;
};