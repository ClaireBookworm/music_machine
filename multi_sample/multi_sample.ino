#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2S.h>
#include "music_files/Seventies-funk-drum-loop-109-BPM.h"
#include "music_files/Square-synth-keys-loop-112-bpm.h"
#include "music_files/Funk-groove-loop.h"
#include "music_files/Groove-loop-126-bpm.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

I2S i2s(OUTPUT);

// input pins - connected to TX pins from controller
// PIN1 = GPIO 3 (P3/D10) -> drums
// PIN2 = GPIO 4 (P4/D9) -> synth
// PIN3 = GPIO 2 (P2/D8) -> funk
// PIN4 = GPIO 1 (P1/D7) -> groove
#define PIN1 3
#define PIN2 4
#define PIN3 2
#define PIN4 1

// track position in each sample
uint32_t pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0;

const char* pin1_name = "Drums";
const char* pin2_name = "Synth Keys";
const char* pin3_name = "Funk Groove";
const char* pin4_name = "Groove";

uint32_t display_counter = 0;
#define DISPLAY_UPDATE_INTERVAL 4410  // update ~10x per second
uint32_t debug_counter = 0;
#define DEBUG_UPDATE_INTERVAL 4410  // print debug info ~10x per second

void updateDisplay(bool active1, bool active2, bool active3, bool active4) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Check if any pin is active
  bool any_active = active1 || active2 || active3 || active4;
  
  if (!any_active) {
    // Show "ready" when no pins are active (standby)
    display.setTextSize(2);
    display.setCursor(20, 24);
    display.println("ready");
  } else {
    // Show active pins
    display.setTextSize(1);
    int y = 0;
    if (active1) {
      display.setCursor(0, y);
      display.print("P"); display.print(PIN1);
      display.print(": "); display.println(pin1_name);
      y += 10;
    }
    if (active2) {
      display.setCursor(0, y);
      display.print("P"); display.print(PIN2);
      display.print(": "); display.println(pin2_name);
      y += 10;
    }
    if (active3) {
      display.setCursor(0, y);
      display.print("P"); display.print(PIN3);
      display.print(": "); display.println(pin3_name);
      y += 10;
    }
    if (active4) {
      display.setCursor(0, y);
      display.print("P"); display.print(PIN4);
      display.print(": "); display.println(pin4_name);
      y += 10;
    }
  }
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for Serial to initialize
  Serial.println("\n\n=== RP2040 Music Machine Starting ===");
  Serial.println("Serial monitor connected!");
  
  // Use INPUT if TX pins actively drive the line, INPUT_PULLDOWN if they're open-drain
  // pinMode(PIN1, INPUT_PULLDOWN)
  pinMode(PIN1, INPUT_PULLDOWN);  // TX pins typically drive HIGH/LOW, no pull needed
  pinMode(PIN2, INPUT_PULLDOWN);
  pinMode(PIN3, INPUT_PULLDOWN);
  pinMode(PIN4, INPUT_PULLDOWN);
  Serial.println("Pins configured as INPUT");
  
  Wire.setSDA(6);  // for SDA on pin 6
  Wire.setSCL(7);  // for SCL on pin 7
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Display FAIL!");
    while(1);
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 24);
  display.println("ready");
  display.display();
  
  Serial.println("Audio data loaded from header files!");
  Serial.print("  Drums: "); Serial.print(SEVENTIES_FUNK_DRUM_LOOP_109_BPM_LENGTH); Serial.println(" samples");
  Serial.print("  Synth: "); Serial.print(SQUARE_SYNTH_KEYS_LOOP_112_BPM_LENGTH); Serial.println(" samples");
  Serial.print("  Funk: "); Serial.print(FUNK_GROOVE_LOOP_LENGTH); Serial.println(" samples");
  Serial.print("  Groove: "); Serial.print(GROOVE_LOOP_126_BPM_LENGTH); Serial.println(" samples");
  Serial.println("System ready - waiting for pin input...");
  Serial.println("---");
  
  i2s.setBCLK(28);
  i2s.setDATA(27);
  i2s.setBitsPerSample(16);
  
  // Use the sample rate from headers (they should all be the same)
  if(!i2s.begin(SEVENTIES_FUNK_DRUM_LOOP_109_BPM_RATE)) {
    Serial.println("I2S FAILED!");
    while(1);
  }
  Serial.print("I2S initialized at "); 
  Serial.print(SEVENTIES_FUNK_DRUM_LOOP_109_BPM_RATE); 
  Serial.println(" Hz");
}

void loop() {
  bool active1 = digitalRead(PIN1);
  bool active2 = digitalRead(PIN2);
  bool active3 = digitalRead(PIN3);
  bool active4 = digitalRead(PIN4);
  
  if (display_counter % DISPLAY_UPDATE_INTERVAL == 0) {
    updateDisplay(active1, active2, active3, active4);
  }
  display_counter++;
  
  // Debug output to Serial
  if (debug_counter % DEBUG_UPDATE_INTERVAL == 0) {
    Serial.print("Pins: P1=");
    Serial.print(active1 ? "HIGH" : "LOW");
    Serial.print(" P2=");
    Serial.print(active2 ? "HIGH" : "LOW");
    Serial.print(" P3=");
    Serial.print(active3 ? "HIGH" : "LOW");
    Serial.print(" P4=");
    Serial.print(active4 ? "HIGH" : "LOW");
    Serial.print(" | Active tracks: ");
    
    bool any_active = false;
    if (active1) { Serial.print("Drums "); any_active = true; }
    if (active2) { Serial.print("Synth "); any_active = true; }
    if (active3) { Serial.print("Funk "); any_active = true; }
    if (active4) { Serial.print("Groove "); any_active = true; }
    
    if (!any_active) {
      Serial.print("READY (standby)");
    }
    Serial.println();
  }
  debug_counter++;
  
  // mix and output one sample
  int32_t mixed = 0;
  int active_count = 0;
  
  if (active1) {
    // RP2040 can access PROGMEM directly, but pgm_read_word is safer
    int16_t sample = pgm_read_word(&seventies_funk_drum_loop_109_bpm_data[pos1]);
    mixed += (int32_t)sample;
    active_count++;
    pos1++;
    if (pos1 >= SEVENTIES_FUNK_DRUM_LOOP_109_BPM_LENGTH) {
      pos1 = 0;
    }
  }
  
  if (active2) {
    int16_t sample = pgm_read_word(&square_synth_keys_loop_112_bpm_data[pos2]);
    mixed += (int32_t)sample;
    active_count++;
    pos2++;
    if (pos2 >= SQUARE_SYNTH_KEYS_LOOP_112_BPM_LENGTH) {
      pos2 = 0;
    }
  }
  
  if (active3) {
    int16_t sample = pgm_read_word(&funk_groove_loop_data[pos3]);
    mixed += (int32_t)sample;
    active_count++;
    pos3++;
    if (pos3 >= FUNK_GROOVE_LOOP_LENGTH) {
      pos3 = 0;
    }
  }
  
  if (active4) {
    int16_t sample = pgm_read_word(&groove_loop_126_bpm_data[pos4]);
    mixed += (int32_t)sample;
    active_count++;
    pos4++;
    if (pos4 >= GROOVE_LOOP_126_BPM_LENGTH) {
      pos4 = 0;
    }
  }
  
  // divide by number of active channels to prevent clipping
  if (active_count > 0) {
    mixed /= active_count;
  }
  
  // clamp to int16 range
  if (mixed > 32767) mixed = 32767;
  if (mixed < -32768) mixed = -32768;
  
  int16_t output = (int16_t)mixed;
  i2s.write(output);  // left
  i2s.write(output);  // right (duplicate for stereo)
}
