/*

*/
#define LED_BUILTIN 13

#define SevenSegCC1 44
#define SevenSegCC2 46

#define SevenSegA 4
#define SevenSegB 5
#define SevenSegC 6
#define SevenSegD 7
#define SevenSegE 8
#define SevenSegF 9
#define SevenSegG 10
#define SevenSegDP 11

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(SevenSegA, OUTPUT);
  pinMode(SevenSegB, OUTPUT);
  pinMode(SevenSegC, OUTPUT);
  pinMode(SevenSegD, OUTPUT);
  pinMode(SevenSegE, OUTPUT);
  pinMode(SevenSegF, OUTPUT);
  pinMode(SevenSegG, OUTPUT);
  pinMode(SevenSegDP, OUTPUT);

  pinMode(SevenSegCC1, OUTPUT);
  pinMode(SevenSegCC2, OUTPUT);



}

// the loop function runs over and over again forever
void loop() {
  // CC1 Zero
  // CC2 Zero
  digitalWrite(SevenSegCC1, LOW);
  digitalWrite(SevenSegCC2, LOW);

  delay(500);                       // wait for a second
  digitalWrite(SevenSegA, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegA, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegB, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegB, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegC, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegC, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegD, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegD, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegE, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegE, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegF, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegF, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegG, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegG, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegDP, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegDP, LOW);    // turn the LED On by making the voltage LOW
}
