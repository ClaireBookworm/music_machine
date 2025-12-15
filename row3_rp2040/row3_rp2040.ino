#include "Electronic-house-music-loop-120-bpm.h"
#include "Funk-beat-120-bpm.h"
#include "Jazzistic-music-loop-120-bpm.h"
#include "Kraftwerk-style-synth-loop-120-bpm.h"
#include <I2S.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <SerialPIO.h>

#define ALARM_DT_NUM 1
#define ALARM_DT_IRQ TIMER_IRQ_1

#define SAMPLE_RATE 22050
#define SAMPLE_DT_US 45

#define DELAY_ABSENT_MS 300

#define N_TRACKS 4
#define TRACK_INVALID -1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
I2S i2s(OUTPUT);

uint32_t _delT_us = SAMPLE_DT_US;

const int16_t* track_pointers[N_TRACKS] = {electronic_house_music_loop_120_bpm_data, funk_beat_120_bpm_data, jazzistic_music_loop_120_bpm_data, kraftwerk_style_synth_loop_120_bpm_data};
int32_t track_lengths[N_TRACKS] = {ELECTRONIC_HOUSE_MUSIC_LOOP_120_BPM_LENGTH, FUNK_BEAT_120_BPM_LENGTH, JAZZISTIC_MUSIC_LOOP_120_BPM_LENGTH, KRAFTWERK_STYLE_SYNTH_LOOP_120_BPM_LENGTH};
const char* track_names[N_TRACKS] = {"Electronic House", "Funk Beat", "Jazzistic", "Kraftwerk Style Synth"};

struct TrackInfo {
  bool is_playing;
  int track_id;
  uint32_t counter;
};

struct TrackInfo track1 = {false, 0, 0};
struct TrackInfo track2 = {false, 0, 0};
struct TrackInfo track3 = {false, 0, 0};
struct TrackInfo track4 = {false, 0, 0};

SerialPIO ser2(8, 4);
SerialPIO ser3(9, 2);
SerialPIO ser4(10, 3);

void alarm_dt_handler(void){
  // setup next call right away
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_DT_NUM);
  timer_hw->alarm[ALARM_DT_NUM] = (uint32_t) (timer_hw->timerawl + _delT_us);
  int active = track1.is_playing + track2.is_playing + track3.is_playing + track4.is_playing;
  if (!active) { i2s.write(0, false); return; }


  int32_t mixed = 0;

  const int16_t* track_ptr;
  int32_t track_len;

  if (track1.is_playing) {
    track_ptr = track_pointers[track1.track_id];
    track_len = track_lengths[track1.track_id];

    mixed += track_ptr[track1.counter] / active;
    
    track1.counter++;
    if (track1.counter >= track_len) {
      track1.counter = 0;
    }
  }

  if (track2.is_playing) {
    track_ptr = track_pointers[track2.track_id];
    track_len = track_lengths[track2.track_id];

    mixed += track_ptr[track2.counter] / active;
    
    track2.counter++;
    if (track2.counter >= track_len) {
      track2.counter = 0;
    }
  }

  if (track3.is_playing) {
    track_ptr = track_pointers[track3.track_id];
    track_len = track_lengths[track3.track_id];

    mixed += track_ptr[track3.counter] / active;
    
    track3.counter++;
    if (track3.counter >= track_len) {
      track3.counter = 0;
    }
  }

  if (track4.is_playing) {
    track_ptr = track_pointers[track4.track_id];
    track_len = track_lengths[track4.track_id];

    mixed += track_ptr[track4.counter] / active;
    
    track4.counter++;
    if (track4.counter >= track_len) {
      track4.counter = 0;
    }
  }

  int16_t output = (int16_t)mixed;
  int32_t output32 = (output << 16) | (output & 0xffff);
  i2s.write(output32, false);
}

void updateDisplay() {
	display.clearDisplay();
	display.setTextColor(SSD1306_WHITE);
	
	bool any = track1.is_playing || track2.is_playing || track3.is_playing || track4.is_playing;
	
	if (!any) {
	  display.setTextSize(2);
	  display.setCursor(20, 24);
	  display.println("ready");
	} else {
	  display.setTextSize(1);
	  int y = 0;
	  if (track1.is_playing) { display.setCursor(0, y); display.println(track_names[track1.track_id]); y += 10; }
	  if (track2.is_playing) { display.setCursor(0, y); display.println(track_names[track2.track_id]); y += 10; }
	  if (track3.is_playing) { display.setCursor(0, y); display.println(track_names[track3.track_id]); y += 10; }
	  if (track4.is_playing) { display.setCursor(0, y); display.println(track_names[track4.track_id]); y += 10; }
	}
	
	display.display();
  }

void setup() {
  Serial.begin(0);

  Serial1.begin(115200, SERIAL_8E2);
  ser2.begin(115200, SERIAL_8E2);  
  ser3.begin(115200, SERIAL_8E2);  
  ser4.begin(115200, SERIAL_8E2);  

  Wire.setSDA(6);
  Wire.setSCL(7);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  updateDisplay();

  // setup I2S
  i2s.setBCLK(28);
  i2s.setDATA(27);
  i2s.setBitsPerSample(16);
  i2s.begin(SAMPLE_RATE);

  // setup interrupt
  hw_set_bits(&timer_hw->inte, 1u << ALARM_DT_NUM);
  irq_set_exclusive_handler(ALARM_DT_IRQ, alarm_dt_handler);
  irq_set_enabled(ALARM_DT_IRQ, true);
  timer_hw->alarm[ALARM_DT_NUM] = (uint32_t) (timer_hw->timerawl + _delT_us);
}

int id_to_track(char id) {
	switch (id) {
	  case 0x01:
     return 0;
	  case 0x02:
     return 1;
	  case 0x03:
     return 2;
	  case 0x04:
     return 3;
	  default:
     return TRACK_INVALID;
	}
}

void loop() {
  static unsigned long t1 = 0;
  static unsigned long t2 = 0;
  static unsigned long t3 = 0;
  static unsigned long t4 = 0;

  static unsigned long last_display = 0;

  bool connected1 = false;

  char b = 0;
  int track_id = TRACK_INVALID;

  if (Serial1.available()) {
    b = Serial1.read();
    t1 = millis();
    Serial.print("on 1: ");
    Serial.println((int)b, HEX);

    track_id = id_to_track(b);
    
    if (!track1.is_playing) {
      if (track_id != TRACK_INVALID) {
        track1.track_id = track_id;
        track1.is_playing = true;
      }
    }
  }

  if (ser2.available()) {
    b = ser2.read();
    t2 = millis();
    Serial.print("on 2: ");
    Serial.println((int)b, HEX);

    track_id = id_to_track(b);
    
    if (!track2.is_playing) {
      if (track_id != TRACK_INVALID) {
        track2.track_id = track_id;
        track2.is_playing = true;
      }
    }
  }

  if (ser3.available()) {
    b = ser3.read();
    t3 = millis();
    Serial.print("on 3: ");
    Serial.println((int)b, HEX);

    track_id = id_to_track(b);
    
    if (!track3.is_playing) {
      if (track_id != TRACK_INVALID) {
        track3.track_id = track_id;
        track3.is_playing = true;
      }
    }
  }

  if (ser4.available()) {
    b = ser4.read();
    t4 = millis();
    Serial.print("on 4: ");
    Serial.println((int)b, HEX);

    track_id = id_to_track(b);
    
    if (!track4.is_playing) {
      if (track_id != TRACK_INVALID) {
        track4.track_id = track_id;
        track4.is_playing = true;
      }
    }
  }

  delay(20);
  
  unsigned long now = millis();

  if (track1.is_playing) {
    if (now - t1 > DELAY_ABSENT_MS) {
      track1.is_playing = false;
      track1.counter = 0;
    }
  }

  if (track2.is_playing) {
    if (now - t2 > DELAY_ABSENT_MS) {
      track2.is_playing = false;
      track2.counter = 0;
    }
  }

  if (track3.is_playing) {
    if (now - t3 > DELAY_ABSENT_MS) {
      track3.is_playing = false;
      track3.counter = 0;
    }
  }

  if (track4.is_playing) {
    if (now - t4 > DELAY_ABSENT_MS) {
      track4.is_playing = false;
      track4.counter = 0;
    }
  }

  if (now - last_display > 100) {
    updateDisplay();
    last_display = now;
  }
}