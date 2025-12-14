#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2S.h>
#include "music_files/Seventies-funk-drum-loop-109-BPM.h"
#include "music_files/Square-synth-keys-loop-112-bpm.h"
#include "music_files/Funk-groove-loop.h"
#include "music_files/Groove-loop-126-bpm.h"

// include your 4 audio files
// #include "music_files/sample1.h"
// #include "music_files/sample2.h"
// #include "music_files/sample3.h"
// #include "music_files/sample4.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

I2S i2s(OUTPUT);

#define BUFFER_SIZE 512
int16_t buffer[BUFFER_SIZE];

// input pins
#define PIN1 3 // P3 physical pin
#define PIN2 4 // P4 physical pin
#define PIN3 2 // P2 physical pin
#define PIN4 1 // P1 physical pin

// track position in each sample
uint32_t pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0;

// instrument names for each pin
const char* pin1_name = "Drums";
const char* pin2_name = "Synth Keys";
const char* pin3_name = "Funk Groove";
const char* pin4_name = "Groove";

// display update counter
uint32_t display_counter = 0;
#define DISPLAY_UPDATE_INTERVAL 22050  // update display ~once per second at 44.1kHz

void setup()
{
	// setup input pins with pulldown
	pinMode(PIN1, INPUT_PULLDOWN);
	pinMode(PIN2, INPUT_PULLDOWN);
	pinMode(PIN3, INPUT_PULLDOWN);
	pinMode(PIN4, INPUT_PULLDOWN);

	Wire.begin();
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
		while (1)
			;

	display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(20, 24);
	display.println("ready");
	display.display();

	i2s.setBCLK(28);
	i2s.setDATA(27);
	i2s.setBitsPerSample(16);

	if (!i2s.begin(44100))
		while (1)
			; // match all samples to same rate
}

void updateDisplay(bool active1, bool active2, bool active3, bool active4)
{
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);
	
	int y = 0;
	if (active1) {
		display.setCursor(0, y);
		display.print("P");
		display.print(PIN1);
		display.print(": ");
		display.println(pin1_name);
		y += 10;
	}
	if (active2) {
		display.setCursor(0, y);
		display.print("P");
		display.print(PIN2);
		display.print(": ");
		display.println(pin2_name);
		y += 10;
	}
	if (active3) {
		display.setCursor(0, y);
		display.print("P");
		display.print(PIN3);
		display.print(": ");
		display.println(pin3_name);
		y += 10;
	}
	if (active4) {
		display.setCursor(0, y);
		display.print("P");
		display.print(PIN4);
		display.print(": ");
		display.println(pin4_name);
		y += 10;
	}
	
	if (y == 0) {
		display.setCursor(0, 24);
		display.setTextSize(2);
		display.println("ready");
	}
	
	display.display();
}

void loop()
{
	// read pin states
	bool active1 = digitalRead(PIN1);
	bool active2 = digitalRead(PIN2);
	bool active3 = digitalRead(PIN3);
	bool active4 = digitalRead(PIN4);
	
	// update display periodically
	if (display_counter % DISPLAY_UPDATE_INTERVAL == 0) {
		updateDisplay(active1, active2, active3, active4);
	}
	display_counter++;

	// mix and output one sample
	int32_t mixed = 0;
	int active_count = 0;

	if (active1)
	{
		mixed += (int32_t)pgm_read_word(&seventies_funk_drum_loop_109_bpm_data[pos1]);
		active_count++;
		pos1++;
		if (pos1 >= SEVENTIES_FUNK_DRUM_LOOP_109_BPM_LENGTH)
			pos1 = 0;
	}

	if (active2)
	{
		mixed += (int32_t)pgm_read_word(&square_synth_keys_loop_112_bpm_data[pos2]);
		active_count++;
		pos2++;
		if (pos2 >= SQUARE_SYNTH_KEYS_LOOP_112_BPM_LENGTH)
			pos2 = 0;
	}

	if (active3)
	{
		mixed += (int32_t)pgm_read_word(&funk_groove_loop_data[pos3]);
		active_count++;
		pos3++;
		if (pos3 >= FUNK_GROOVE_LOOP_LENGTH)
			pos3 = 0;
	}

	if (active4)
	{
		mixed += (int32_t)pgm_read_word(&groove_loop_126_bpm_data[pos4]);
		active_count++;
		pos4++;
		if (pos4 >= GROOVE_LOOP_126_BPM_LENGTH)
			pos4 = 0;
	}

	// divide by number of active channels to prevent clipping
	if (active_count > 0)
	{
		mixed /= active_count;
	}

	// clamp to int16 range
	if (mixed > 32767)
		mixed = 32767;
	if (mixed < -32768)
		mixed = -32768;

	int16_t output = (int16_t)mixed;
	i2s.write(output); // left
	i2s.write(output); // right
}