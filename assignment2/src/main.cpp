#include <Arduino.h>

#define LED 13

// Func definitions
void setup();
void loop();
void Morse_S();
void Morse_O();
void Morse_Space_Between_Letters();
void Morse_Space_Between_Words();
void Morse_dot();
void Morse_dash();

void setup() {

  // init our LED on pin 13
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
}

void loop() {
  /* Let's print out SOS in morse code
  ** Rules for morse code
  ** 1. The length of a dot is one unit
  ** 2. A dash is three units
  ** 3. The space between parts of the same letter is one unit.
  ** 4. The space between letters is three units
  ** 5. The space between words is seven units
  ** SOS --> S(dot dot dot) Space(three units) S(dash dash dash) Space(three units) S(dot dot dot)
  ** Let our unit be 1 second.
  ** https://en.wikipedia.org/wiki/Morse_code#/media/File:International_Morse_Code.svg
  */


  Morse_S();
  Morse_Space_Between_Letters();
  Morse_O();
  Morse_Space_Between_Letters();
  Morse_S();
  Morse_Space_Between_Words();
}

void Morse_S()
{
  Morse_dot();
  delay(1000);
  Morse_dot();
  delay(1000);
  Morse_dot();
}

void Morse_O()
{
  Morse_dash();
  delay(1000);
  Morse_dash();
  delay(1000);
  Morse_dash();
}

void Morse_Space_Between_Letters()
{ 
  digitalWrite(LED, LOW);
  delay(3000);
}

void Morse_Space_Between_Words()
{ 
  digitalWrite(LED, LOW);
  delay(7000);
}

void Morse_dot()
{
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
}

void Morse_dash()
{
  digitalWrite(LED, HIGH);
  delay(3000);
  digitalWrite(LED, LOW);
}