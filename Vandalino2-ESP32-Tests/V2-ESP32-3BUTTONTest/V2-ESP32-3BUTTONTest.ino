#define BUTTONS1 27
#define BUTTONS2 33
#define BUTTONS3 14



void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTONS1, INPUT);
  pinMode(BUTTONS2, INPUT);
  pinMode(BUTTONS3, INPUT);


  Serial.begin(115200);
  Serial.println("\nV2-ESP32-DIP8Test");
}

void loop() {
  // put your main code here, to run repeatedly:


  Serial.print(digitalRead(BUTTONS1));
  Serial.print(digitalRead(BUTTONS2));
  Serial.println(digitalRead(BUTTONS3));
  delay(500);
}
