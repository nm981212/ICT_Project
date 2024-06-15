#define GasPin A0

void setup() {
  pinMode(GasPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println((analogRead(GasPin)));
  delay(1000);
}