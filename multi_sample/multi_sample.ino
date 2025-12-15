#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2S.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "music_files/Seventies-funk-drum-loop-109-BPM.h"
#include "music_files/Square-synth-keys-loop-112-bpm.h"
#include "music_files/Funk-groove-loop.h"
#include "music_files/Groove-loop-126-bpm.h"

#define UART_RX_PIN 3
#define UART_BAUD 57600

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
I2S i2s(OUTPUT);

uint32_t pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0;
volatile uint8_t active_mask = 0;

PIO pio = pio0;
uint sm;

// correct pio uart rx program
static const uint16_t uart_rx_program[] = {
  0x2020,  // 0: wait 0 pin 0
  0xe027,  // 1: set x, 7 [10]  
  0x4001,  // 2: in pins, 1
  0x0642,  // 3: jmp x-- 2 [6]
};

void pio_uart_rx_init(uint pin, uint baud) {
  pio_program_t prog = {
    .instructions = uart_rx_program,
    .length = 4,
    .origin = -1
  };
  
  uint offset = pio_add_program(pio, &prog);
  sm = pio_claim_unused_sm(pio, true);
  
  pio_sm_config c = pio_get_default_sm_config();
  sm_config_set_in_pins(&c, pin);
  sm_config_set_jmp_pin(&c, pin);
  sm_config_set_in_shift(&c, true, true, 8);
  
  float div = (float)clock_get_hz(clk_sys) / (8 * baud);
  sm_config_set_clkdiv(&c, div);
  
  pio_gpio_init(pio, pin);
  gpio_pull_up(pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
  
  pio_sm_init(pio, sm, offset, &c);
  pio_sm_set_enabled(pio, sm, true);
}

uint8_t deviceToTrack(uint8_t id) {
  switch(id) {
    case 0x42: return 0;
    case 0x02: return 1;
    case 0x03: return 2;
    case 0x04: return 3;
    default: return 255;
  }
}

void setup() {
  Serial.begin(115200);
  
  Wire.setSDA(6);
  Wire.setSCL(7);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  i2s.setBCLK(28);
  i2s.setDATA(27);
  i2s.setBitsPerSample(16);
  i2s.begin(SEVENTIES_FUNK_DRUM_LOOP_109_BPM_RATE);
  
  pio_uart_rx_init(UART_RX_PIN, UART_BAUD);
}

void loop() {
  static uint32_t sample_count = 0;
  static uint32_t last_seen[4] = {0, 0, 0, 0};
  
  // drain pio fifo
  while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
    uint8_t b = pio_sm_get(pio, sm) >> 24;  // byte is in top 8 bits
    uint8_t t = deviceToTrack(b);
    if (t < 4) last_seen[t] = sample_count;
  }
  
  // update mask every ~100ms
  if (sample_count % 4410 == 0) {
    active_mask = 0;
    for (int i = 0; i < 4; i++) {
      if (sample_count - last_seen[i] < 6615) {  // 150ms
        active_mask |= (1 << i);
      }
    }
  }
  
  sample_count++;
  
  // audio
  int32_t mixed = 0;
  int count = 0;
  
  if (active_mask & 0x01) {
    mixed += pgm_read_word(&seventies_funk_drum_loop_109_bpm_data[pos1++]);
    if (pos1 >= SEVENTIES_FUNK_DRUM_LOOP_109_BPM_LENGTH) pos1 = 0;
    count++;
  }
  if (active_mask & 0x02) {
    mixed += pgm_read_word(&square_synth_keys_loop_112_bpm_data[pos2++]);
    if (pos2 >= SQUARE_SYNTH_KEYS_LOOP_112_BPM_LENGTH) pos2 = 0;
    count++;
  }
  if (active_mask & 0x04) {
    mixed += pgm_read_word(&funk_groove_loop_data[pos3++]);
    if (pos3 >= FUNK_GROOVE_LOOP_LENGTH) pos3 = 0;
    count++;
  }
  if (active_mask & 0x08) {
    mixed += pgm_read_word(&groove_loop_126_bpm_data[pos4++]);
    if (pos4 >= GROOVE_LOOP_126_BPM_LENGTH) pos4 = 0;
    count++;
  }
  
  if (count) mixed /= count;
  if (mixed > 32767) mixed = 32767;
  if (mixed < -32768) mixed = -32768;
  
  i2s.write16((int16_t)mixed, (int16_t)mixed);
}