#include <stdio.h>
#include "sdkconfig.h"
#include <Arduino.h>
#include <PixelFunctions.h>

#define PIN 21
#define NUM_LEDS 4
#define BRIGHTNESS 50

void setup();
void loop();

void setup() {

 dumpSysInfo();
  getMaxMalloc(1 * 1024, 16 * 1024 * 1024);

  if (digitalLeds_initStrands(STRANDS, STRANDCNT)) {
    printf("Init FAILURE: halting");
    while (true) {};
  }
  for (int i = 0; i < STRANDCNT; i++) {
    strand_t * pStrand = &STRANDS[i];
    printf("Strand ");
    printf("%d", i);
    printf(" = ");
    printf("%X", (uint32_t)(pStrand->pixels));
    printf("\n");
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
  rainbow(pStrand, 0, 2000);
}
