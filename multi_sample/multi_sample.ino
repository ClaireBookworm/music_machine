#include "Seventies-funk-drum-loop-109-BPM.h"
#include "Square-synth-keys-loop-112-bpm.h"
#include "Funk-groove-loop.h"
#include "Groove-loop-126-bpm.h"
#include <I2S.h>

#include <SerialPIO.h>

#define ALARM_DT_NUM 1
#define ALARM_DT_IRQ TIMER_IRQ_1

#define SAMPLE_RATE 22050
#define SAMPLE_DT_US 45

#define DELAY_ABSENT_MS 300

#define N_TRACKS 4

#define TRACK_INVALID -1


I2S i2s(OUTPUT);

uint32_t _delT_us = SAMPLE_DT_US;

const int16_t* track_pointers[N_TRACKS] = {seventies_funk_drum_loop_109_bpm_data, square_synth_keys_loop_112_bpm_data, funk_groove_loop_data, groove_loop_126_bpm_data};
int32_t track_lengths[N_TRACKS] = {SEVENTIES_FUNK_DRUM_LOOP_109_BPM_LENGTH, SQUARE_SYNTH_KEYS_LOOP_112_BPM_LENGTH, FUNK_GROOVE_LOOP_LENGTH, GROOVE_LOOP_126_BPM_LENGTH};
const char* track_names[N_TRACKS] = {"Drums", "Synth Keys", "Funk Groove", "Groove"};

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

  int32_t mixed = 0;

  const int16_t* track_ptr;
  int32_t track_len;

  if (track1.is_playing) {
    track_ptr = track_pointers[track1.track_id];
    track_len = track_lengths[track1.track_id];

    mixed += track_ptr[track1.counter] / N_TRACKS;
    
    track1.counter++;
    if (track1.counter >= track_len) {
      track1.counter = 0;
    }
  }

  if (track2.is_playing) {
    track_ptr = track_pointers[track2.track_id];
    track_len = track_lengths[track2.track_id];

    mixed += track_ptr[track2.counter] / N_TRACKS;
    
    track2.counter++;
    if (track2.counter >= track_len) {
      track2.counter = 0;
    }
  }

  if (track3.is_playing) {
    track_ptr = track_pointers[track3.track_id];
    track_len = track_lengths[track3.track_id];

    mixed += track_ptr[track3.counter] / N_TRACKS;
    
    track3.counter++;
    if (track3.counter >= track_len) {
      track3.counter = 0;
    }
  }

  if (track4.is_playing) {
    track_ptr = track_pointers[track4.track_id];
    track_len = track_lengths[track4.track_id];

    mixed += track_ptr[track4.counter] / N_TRACKS;
    
    track4.counter++;
    if (track4.counter >= track_len) {
      track4.counter = 0;
    }
  }

  int16_t output = (int16_t)mixed;
  int32_t output32 = (output << 16) | (output & 0xffff);
  i2s.write(output32, false);
}

void setup() {
  Serial.begin(0);

  Serial1.begin(115200, SERIAL_8E2);
  ser2.begin(115200, SERIAL_8E2);  
  ser3.begin(115200, SERIAL_8E2);  
  ser4.begin(115200, SERIAL_8E2);  

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

  if (track1.is_playing) {
    if (millis() - t1 > DELAY_ABSENT_MS) {
      track1.is_playing = false;
      track1.counter = 0;
    }
  }

  if (track2.is_playing) {
    if (millis() - t2 > DELAY_ABSENT_MS) {
      track2.is_playing = false;
      track2.counter = 0;
    }
  }

  if (track3.is_playing) {
    if (millis() - t3 > DELAY_ABSENT_MS) {
      track3.is_playing = false;
      track3.counter = 0;
    }
  }

  if (track4.is_playing) {
    if (millis() - t4 > DELAY_ABSENT_MS) {
      track4.is_playing = false;
      track4.counter = 0;
    }
  }
}