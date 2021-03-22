#define DIP0 34
#define DIP1 39
#define DIP2 36
#define DIP3 15
#define DIP4 A0

void setup() {
  // put your setup code here, to run once:
  pinMode(DIP0, INPUT);
  pinMode(DIP1, INPUT);
  pinMode(DIP2, INPUT);
  pinMode(DIP3, INPUT);
  pinMode(DIP4, INPUT);

  Serial.begin(115200);
  Serial.println("\nV2-DIP8Test");
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.print(digitalRead(DIP0));
  Serial.print(digitalRead(DIP1));
  Serial.print(digitalRead(DIP2));
  Serial.print(digitalRead(DIP3));
  Serial.println(digitalRead(DIP4));
  delay(500);
}
