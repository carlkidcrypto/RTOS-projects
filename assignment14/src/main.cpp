#include <Arduino.h>
#include "PixelFunctions.h"

#define PIN 21
#define NUM_LEDS 4
#define BRIGHTNESS 10

void setup() {

 dumpSysInfo();
  getMaxMalloc(1 * 1024, 16 * 1024 * 1024);

  if (digitalLeds_initStrands(STRANDS, STRANDCNT)) {
    Serial.println("Init FAILURE: halting");
    while (true) {};
  }
  for (int i = 0; i < STRANDCNT; i++) {
    strand_t * pStrand = &STRANDS[i];
    Serial.print("Strand ");
    Serial.print(i);
    Serial.print(" = ");
    Serial.print((uint32_t)(pStrand->pixels), HEX);
    Serial.println();
#if DEBUG_ESP32_DIGITAL_LED_LIB
    dumpDebugBuffer(-2, digitalLeds_debugBuffer);
#endif
    digitalLeds_resetPixels(pStrand);
#if DEBUG_ESP32_DIGITAL_LED_LIB
    dumpDebugBuffer(-1, digitalLeds_debugBuffer);
#endif
  }

}

void loop() {
  // Some example procedures showing how to display to the pixels:
  strand_t * pStrand = &STRANDS[0];
  //rainbow(pStrand, 0, 2000);
  One_At_A_Time_RGBW(pStrand, NUM_LEDS, BRIGHTNESS);
}
