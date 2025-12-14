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

uint32_t pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0;

const char *track1_name = "Drums";
const char *track2_name = "Synth Keys";
const char *track3_name = "Funk Groove";
const char *track4_name = "Groove";

// uint8_t stable_devices[10];
// uint8_t stable_num_devices = 0;
uint32_t last_uart_read = 0;
#define UART_READ_INTERVAL 200 // read uart every 20ms

uint32_t display_counter = 0;
#define DISPLAY_UPDATE_INTERVAL 4410
uint32_t debug_counter = 0;
#define DEBUG_UPDATE_INTERVAL 4410

uint8_t active_devices[10]; // store up to 10 device ids
uint8_t num_devices = 0;

// map device id to track number (1-4)
uint8_t deviceToTrack(uint8_t id)
{
	switch (id)
	{
	case 0x42:
		return 1; // drums
	case 0x02:
		return 2; // synth
	case 0x03:
		return 3; // funk
	case 0x04:
		return 4; // groove
	default:
		return 0; // unknown id, no track
	}
}

void updateDisplay(bool active1, bool active2, bool active3, bool active4)
{
	display.clearDisplay();
	display.setTextColor(SSD1306_WHITE);

	bool any_active = active1 || active2 || active3 || active4;

	if (!any_active)
	{
		display.setTextSize(2);
		display.setCursor(20, 24);
		display.println("ready");
	}
	else
	{
		display.setTextSize(1);
		int y = 0;
		if (active1)
		{
			display.setCursor(0, y);
			display.print("T1: ");
			display.println(track1_name);
			y += 10;
		}
		if (active2)
		{
			display.setCursor(0, y);
			display.print("T2: ");
			display.println(track2_name);
			y += 10;
		}
		if (active3)
		{
			display.setCursor(0, y);
			display.print("T3: ");
			display.println(track3_name);
			y += 10;
		}
		if (active4)
		{
			display.setCursor(0, y);
			display.print("T4: ");
			display.println(track4_name);
			y += 10;
		}
	}

	// show connected device ids at bottom
	if (num_devices > 0)
	{
		display.setCursor(0, 56);
		display.print("IDs: ");
		for (int i = 0; i < num_devices && i < 5; i++)
		{
			display.print("0x");
			if (active_devices[i] < 16)
				display.print("0");
			display.print(active_devices[i], HEX);
			if (i < num_devices - 1)
				display.print(" ");
		}
	}

	display.display();
}

void setup()
{
	Serial.begin(115200);
	delay(1000);
	Serial.println("\n\n=== RP2040 Music Machine Starting ===");

	Serial1.begin(115200);

	Wire.setSDA(6);
	Wire.setSCL(7);
	Wire.begin();
  Wire.setClock(400000); // NEW
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
	{
		Serial.println("Display FAIL!");
		while (1)
			;
	}

	display.clearDisplay();
	display.setTextSize(2);
	display.setCursor(20, 24);
	display.println("ready");
	display.display();

	Serial.println("Audio data loaded!");

	i2s.setBCLK(28);
	i2s.setDATA(27);
	i2s.setBitsPerSample(16);
  i2s.setBuffers(8, 256, 0);

	if (!i2s.begin(SEVENTIES_FUNK_DRUM_LOOP_109_BPM_RATE))
	{
		Serial.println("I2S FAILED!");
		while (1)
			;
	}
	Serial.print("I2S at ");
	Serial.print(SEVENTIES_FUNK_DRUM_LOOP_109_BPM_RATE);
	Serial.println(" Hz");
}

void loop()
{
	// read all available device ids from uart
	// num_devices = 0;
	// while (Serial1.available() && num_devices < 10) {
	//   active_devices[num_devices++] = Serial1.read();
	// }
	//   if (millis() - last_uart_read >= UART_READ_INTERVAL) {
	//     stable_num_devices = 0;
	//     while (Serial1.available() && stable_num_devices < 10) {
	//       stable_devices[stable_num_devices++] = Serial1.read();
	//     }
	//     last_uart_read = millis();
	//   }

  static uint32_t sample_count = 0;
  static uint8_t stable_devices[10];
  static uint8_t stable_num_devices = 0;

  if (sample_count % 4410 == 0) {
    stable_num_devices = 0;  // reset for new packet
  }

  // read one byte per sample, accumulate over time
  if (Serial1.available() && stable_num_devices < 10) {
    stable_devices[stable_num_devices++] = Serial1.read();
  }

  sample_count++;

	// figure out which tracks are active based on device ids
	bool active1 = false, active2 = false, active3 = false, active4 = false;
	// for (int i = 0; i < num_devices; i++) {
	//   uint8_t track = deviceToTrack(active_devices[i]);
  for (int i = 0; i < stable_num_devices; i++) {
    uint8_t track = deviceToTrack(stable_devices[i]);
    if (track == 1)
      active1 = true;
    if (track == 2)
      active2 = true;
    if (track == 3)
      active3 = true;
    if (track == 4)
      active4 = true;
  }

	if (display_counter % DISPLAY_UPDATE_INTERVAL == 0)
	{
		// updateDisplay(active1, active2, active3, active4);
	}
	display_counter++;

	// if (debug_counter % DEBUG_UPDATE_INTERVAL == 0)
	// {
	// 	Serial.print("Devices: ");
	// 	for (int i = 0; i < num_devices; i++)
	// 	{
	// 		Serial.print("0x");
	// 		Serial.print(stable_devices[i], HEX);
	// 		Serial.print(" ");
	// 	}
	// 	Serial.print("| Tracks: ");
	// 	if (active1)
	// 		Serial.print("Drums ");
	// 	if (active2)
	// 		Serial.print("Synth ");
	// 	if (active3)
	// 		Serial.print("Funk ");
	// 	if (active4)
	// 		Serial.print("Groove ");
	// 	if (!active1 && !active2 && !active3 && !active4)
	// 		Serial.print("READY");
	// 	Serial.println();
	// }
	debug_counter++;

	// mix audio
	int32_t mixed = 0;
	int active_count = 0;

	if (active1) {
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

	// if (active4) {
		mixed += (int32_t)pgm_read_word(&groove_loop_126_bpm_data[pos4]);
		active_count++;
		pos4++;
		if (pos4 >= GROOVE_LOOP_126_BPM_LENGTH)
			pos4 = 0;
	// }

	if (active_count > 0)
		mixed /= active_count;
	if (mixed > 32767)
		mixed = 32767;
	if (mixed < -32768)
		mixed = -32768;

	int16_t output = (int16_t)mixed;
  i2s.write16(output, output);
}