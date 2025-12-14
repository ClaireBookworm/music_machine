// ATtiny1614 - sends track control bytes via UART TX
// upload using megaTinyCore board package

#define TX_PIN PIN_PA1  // default TX pin for ATtiny1614

// example: 4 buttons to control which tracks play
#define BTN1 PIN_PA4
#define BTN2 PIN_PA5
#define BTN3 PIN_PA6
#define BTN4 PIN_PA7

void setup() {
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BTN4, INPUT_PULLUP);
  
  Serial.begin(9600);  // UART on TX/RX pins
}

void loop() {
  // read button states (active low)
  bool track1 = !digitalRead(BTN1);
  bool track2 = !digitalRead(BTN2);
  bool track3 = !digitalRead(BTN3);
  bool track4 = !digitalRead(BTN4);
  
  // pack into single byte: bit0=track1, bit1=track2, etc
  uint8_t state = (track1 << 0) | (track2 << 1) | (track3 << 2) | (track4 << 3);
  
  Serial.write(state);  // send state byte
  delay(10);  // send ~100 times/sec
}