#include <Arduino.h>

#define SIG_PIN PIN_PB2   // your board's TX pad == PB2

void setup() {
  pinMode(SIG_PIN, OUTPUT);
  digitalWrite(SIG_PIN, HIGH);  // constant high while powered
}

void loop() { /* nada */ }