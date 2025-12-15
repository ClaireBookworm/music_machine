// ATtiny1614 - append device ID after forwarding upstream data

#define DEVICE_ID 0x04  // change per device

void setup() {
  Serial.begin(115200, SERIAL_8E2);
  delay(100);
}

void loop() {
  // forward everything from upstream first
  // while (Serial.available()) {
  //   Serial.write(Serial.read());
  // }
  
  // then append our id
  Serial.write(DEVICE_ID);
  
  delay(100);  // ~100hz
}