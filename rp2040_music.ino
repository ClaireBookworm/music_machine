#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2S.h>
#include "music_files/Seventies-funk-drum-loop-109-BPM.h"  // your .h file name here

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

I2S i2s(OUTPUT);

#define BUFFER_SIZE 512
int16_t buffer[BUFFER_SIZE];

// audio_data is already extracted as int16_t samples from the .h file
// AUDIO_DATA_LENGTH is defined in the header file
const uint32_t num_samples = AUDIO_DATA_LENGTH;

void setup() {
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) while(1);
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 24);
  display.println("playing");
  display.display();

  i2s.setBCLK(28);
  i2s.setDATA(27);
  i2s.setBitsPerSample(16);
  
  if(!i2s.begin(AUDIO_SAMPLE_RATE)) while(1);  // use sample rate from header file
}

void loop() {

  if(AUDIO_N_CHANNELS == 2) {
    // stereo: samples are already L,R,L,R... just write them
    for(uint32_t i = 0; i < AUDIO_DATA_LENGTH; i++) {
      int16_t sample = pgm_read_word(&audio_data[i]);
      i2s.write(sample);
    }
  } else {
    // mono: duplicate each sample for L and R
    for(uint32_t i = 0; i < AUDIO_DATA_LENGTH; i++) {
      int16_t sample = pgm_read_word(&audio_data[i]);
      i2s.write(sample);
      i2s.write(sample);
    }
  }
  // delay(1000);
}