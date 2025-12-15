#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2S.h>

// RP2040 multicore
#include <pico/multicore.h>

#include "music_files/Seventies-funk-drum-loop-109-BPM.h"
#include "music_files/Square-synth-keys-loop-112-bpm.h"
#include "music_files/Funk-groove-loop.h"
#include "music_files/Groove-loop-126-bpm.h"

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------- I2S ----------------
I2S i2s(OUTPUT);

// ---------------- Track positions (core0 audio only) ----------------
uint32_t pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0;

const char *track1_name = "Drums";
const char *track2_name = "Synth Keys";
const char *track3_name = "Funk Groove";
const char *track4_name = "Groove";

// ---------------- Shared state (core1 -> core0) ----------------
// bit0..bit3 correspond to tracks 1..4
volatile uint8_t g_active_mask = 0;

// How long a track stays “active” after we last saw its ID byte
static const uint32_t ACTIVE_HOLD_MS = 150;

// ---------------- Device -> track mapping ----------------
// return 0..3 for tracks; 255 for unknown
uint8_t deviceToTrack(uint8_t id) {
  switch (id) {
    case 0x42: return 0; // drums
    case 0x02: return 1; // synth
    case 0x03: return 2; // funk
    case 0x04: return 3; // groove
    default:   return 255;
  }
}

// ---------------- OLED helper ----------------
void updateDisplayFromMask(uint8_t mask) {
  bool a1 = mask & 0x01;
  bool a2 = mask & 0x02;
  bool a3 = mask & 0x04;
  bool a4 = mask & 0x08;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  bool any = a1 || a2 || a3 || a4;

  if (!any) {
    display.setTextSize(2);
    display.setCursor(20, 24);
    display.println("ready");
  } else {
    display.setTextSize(1);
    int y = 0;
    if (a1) { display.setCursor(0, y); display.print("T1: "); display.println(track1_name); y += 10; }
    if (a2) { display.setCursor(0, y); display.print("T2: "); display.println(track2_name); y += 10; }
    if (a3) { display.setCursor(0, y); display.print("T3: "); display.println(track3_name); y += 10; }
    if (a4) { display.setCursor(0, y); display.print("T4: "); display.println(track4_name); y += 10; }
  }

  display.display();
}

// ---------------- Core1 task: UART parsing ----------------
void core1_task() {
  uint32_t last_seen[4] = {0, 0, 0, 0};

  while (true) {
    // Read all bytes available, update last_seen per track
    while (Serial1.available()) {
      uint8_t b = (uint8_t)Serial1.read();
      uint8_t t = deviceToTrack(b);
      if (t < 4) last_seen[t] = millis();
    }

    // Compute active mask with hold time
    uint32_t now = millis();
    uint8_t mask = 0;
    for (int i = 0; i < 4; i++) {
      if (now - last_seen[i] <= ACTIVE_HOLD_MS) mask |= (1u << i);
    }

    g_active_mask = mask;

    // Small yield so core1 doesn’t spin at 100% for no reason
    delay(1);
  }
}

// ---------------- Audio helpers ----------------
static inline int16_t clamp16(int32_t x) {
  if (x > 32767) return 32767;
  if (x < -32768) return -32768;
  return (int16_t)x;
}

// ---------------- setup ----------------
void setup() {
  Serial.begin(9600);
  delay(200);

  // Hardware UART (default pins for Serial1)
  Serial1.begin(9600);

  // OLED init (core0 does setup; later we update at low rate)
  Wire.setSDA(6);
  Wire.setSCL(7);
  Wire.begin();
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1) {}
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 24);
  display.println("ready");
  display.display();

  // I2S init (core0)
  i2s.setBCLK(28);
  i2s.setDATA(27);
  i2s.setBitsPerSample(16);

  // More buffering helps; feel free to push higher
  i2s.setBuffers(16, 256, 0);

  if (!i2s.begin(SEVENTIES_FUNK_DRUM_LOOP_109_BPM_RATE)) {
    while (1) {}
  }
  

  // Launch core1 AFTER Serial1 is configured
  multicore_launch_core1(core1_task);
}

// ---------------- loop (core0): audio only ----------------
void loop() {
  // OPTIONAL: OLED update at a safe cadence
  static uint32_t last_oled_ms = 0;
  uint32_t now_ms = millis();
  if (now_ms - last_oled_ms >= 200) {   // 5 Hz
    last_oled_ms = now_ms;
    updateDisplayFromMask(g_active_mask);
  }

  // Audio block render + write
  const int BLOCK = 128;
  int16_t buf[BLOCK];

  // Wait until there is room for the whole block (stereo -> *2)
  while (i2s.availableForWrite() < (BLOCK * 2)) {
    // tight spin; core0 should prioritize audio
  }

  uint8_t mask = g_active_mask;
  bool a1 = mask & 0x01;
  bool a2 = mask & 0x02;
  bool a3 = mask & 0x04;
  bool a4 = mask & 0x08;

  for (int n = 0; n < BLOCK; n++) {
    int32_t mixed = 0;
    int c = 0;

    if (a1) {
      mixed += (int32_t)pgm_read_word(&seventies_funk_drum_loop_109_bpm_data[pos1++]);
      if (pos1 >= SEVENTIES_FUNK_DRUM_LOOP_109_BPM_LENGTH) pos1 = 0;
      c++;
    }
    if (a2) {
      mixed += (int32_t)pgm_read_word(&square_synth_keys_loop_112_bpm_data[pos2++]);
      if (pos2 >= SQUARE_SYNTH_KEYS_LOOP_112_BPM_LENGTH) pos2 = 0;
      c++;
    }
    if (a3) {
      mixed += (int32_t)pgm_read_word(&funk_groove_loop_data[pos3++]);
      if (pos3 >= FUNK_GROOVE_LOOP_LENGTH) pos3 = 0;
      c++;
    }
    if (a4) {
      mixed += (int32_t)pgm_read_word(&groove_loop_126_bpm_data[pos4++]);
      if (pos4 >= GROOVE_LOOP_126_BPM_LENGTH) pos4 = 0;
      c++;
    }

    if (c) mixed /= c;
    buf[n] = clamp16(mixed);
  }

  for (int n = 0; n < BLOCK; n++) {
    i2s.write16(buf[n], buf[n]);
  }
}
