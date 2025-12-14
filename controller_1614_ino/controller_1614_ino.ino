void setup() {
  Serial.begin(9600);   // uses PB2/PB3 when pinout=alternate
}

void loop() {
  Serial.write(0x01);   // slave addr
  Serial.write(0xA0);   // turn LED ON
  delay(500);

  Serial.write(0x01);
  Serial.write(0xA1);   // turn LED OFF
  delay(500);
}
