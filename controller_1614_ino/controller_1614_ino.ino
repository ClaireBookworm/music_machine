// ATtiny1614 - append device ID after forwarding upstream data

#define DEVICE_ID 0x42  // change per device

void setup() {
  Serial.begin(57600);
}

void loop() {
  // forward everything from upstream first
  while (Serial.available()) {
    Serial.write(Serial.read());
  }
  
  // then append our id
  Serial.write(DEVICE_ID);
  
  delay(10);  // ~100hz
}