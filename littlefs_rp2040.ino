#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2S.h>
#include <LittleFS.h>

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

#define BUFFER_SIZE 512
int16_t buffer1[BUFFER_SIZE];
int16_t buffer2[BUFFER_SIZE];
int16_t buffer3[BUFFER_SIZE];
int16_t buffer4[BUFFER_SIZE];

File file1, file2, file3, file4;
uint32_t file1_samples, file2_samples, file3_samples, file4_samples;
uint32_t pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0;
uint32_t buf_pos1 = 0, buf_pos2 = 0, buf_pos3 = 0, buf_pos4 = 0;
bool buf1_valid = false, buf2_valid = false, buf3_valid = false, buf4_valid = false;

const char *pin1_name = "Drums";
const char *pin2_name = "Synth Keys";
const char *pin3_name = "Funk Groove";
const char *pin4_name = "Groove";

uint32_t display_counter = 0;
#define DISPLAY_UPDATE_INTERVAL 4410 // update ~10x per second (was 22050 = ~1x/sec)
uint32_t debug_counter = 0;
#define DEBUG_UPDATE_INTERVAL 4410 // print debug info ~10x per second

void fillBuffer1()
{
	uint32_t to_read = min(BUFFER_SIZE, file1_samples - pos1);
	file1.read((uint8_t *)buffer1, to_read * 2);
	buf_pos1 = 0;
	buf1_valid = true;
}

void fillBuffer2()
{
	uint32_t to_read = min(BUFFER_SIZE, file2_samples - pos2);
	file2.read((uint8_t *)buffer2, to_read * 2);
	buf_pos2 = 0;
	buf2_valid = true;
}

void fillBuffer3()
{
	uint32_t to_read = min(BUFFER_SIZE, file3_samples - pos3);
	file3.read((uint8_t *)buffer3, to_read * 2);
	buf_pos3 = 0;
	buf3_valid = true;
}

void fillBuffer4()
{
	uint32_t to_read = min(BUFFER_SIZE, file4_samples - pos4);
	file4.read((uint8_t *)buffer4, to_read * 2);
	buf_pos4 = 0;
	buf4_valid = true;
}

void updateDisplay(bool active1, bool active2, bool active3, bool active4)
{
	display.clearDisplay();
	display.setTextColor(SSD1306_WHITE);

	// Check if any pin is active
	bool any_active = active1 || active2 || active3 || active4;

	if (!any_active)
	{
		// Show "ready" when no pins are active (standby)
		display.setTextSize(2);
		display.setCursor(20, 24);
		display.println("ready");
	}
	else
	{
		// Show active pins
		display.setTextSize(1);
		int y = 0;
		if (active1)
		{
			display.setCursor(0, y);
			display.print("P");
			display.print(PIN1);
			display.print(": ");
			display.println(pin1_name);
			y += 10;
		}
		if (active2)
		{
			display.setCursor(0, y);
			display.print("P");
			display.print(PIN2);
			display.print(": ");
			display.println(pin2_name);
			y += 10;
		}
		if (active3)
		{
			display.setCursor(0, y);
			display.print("P");
			display.print(PIN3);
			display.print(": ");
			display.println(pin3_name);
			y += 10;
		}
		if (active4)
		{
			display.setCursor(0, y);
			display.print("P");
			display.print(PIN4);
			display.print(": ");
			display.println(pin4_name);
			y += 10;
		}
	}

	display.display();
}

void setup()
{
	Serial.begin(115200);
	delay(1000); // Wait for Serial to initialize
	Serial.println("\n\n=== RP2040 Music Machine Starting ===");
	Serial.println("Serial monitor connected!");

	// Use INPUT if TX pins actively drive the line, INPUT_PULLDOWN if they're open-drain
	pinMode(PIN1, INPUT); // TX pins typically drive HIGH/LOW, no pull needed
	pinMode(PIN2, INPUT);
	pinMode(PIN3, INPUT);
	pinMode(PIN4, INPUT);
	Serial.println("Pins configured as INPUT");

	Wire.setSDA(6); // for SDA on pin 6
	Wire.setSCL(7); // for SCL on pin 7
	Wire.begin();
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
		while (1)
			;

	display.clearDisplay();
	display.setTextSize(1);
	display.setCursor(0, 0);
	display.println("mounting...");
	display.display();

	if (!LittleFS.begin())
	{
		display.println("FS FAIL!");
		display.display();
		while (1)
			;
	}

	// open files with exact names from screenshot
	file1 = LittleFS.open("/Seventies-funk-drum-loop-109-BPM.wav", "r");
	file2 = LittleFS.open("/Square-synth-keys-loop-112-bpm_trimmed.wav", "r");
	file3 = LittleFS.open("/Funk-groove-loop.wav", "r");
	file4 = LittleFS.open("/Groove-loop-126-bpm.wav", "r");

	if (!file1 || !file2 || !file3 || !file4)
	{
		display.clearDisplay();
		display.setCursor(0, 0);
		display.setTextSize(1);
		if (!file1)
			display.println("drum missing");
		if (!file2)
			display.println("synth missing");
		if (!file3)
			display.println("funk missing");
		if (!file4)
			display.println("groove missing");
		display.display();
		while (1)
			;
	}

	// skip 44-byte wav headers
	file1.seek(44);
	file1_samples = (file1.size() - 44) / 2;
	file2.seek(44);
	file2_samples = (file2.size() - 44) / 2;
	file3.seek(44);
	file3_samples = (file3.size() - 44) / 2;
	file4.seek(44);
	file4_samples = (file4.size() - 44) / 2;

	// prefill buffers
	fillBuffer1();
	fillBuffer2();
	fillBuffer3();
	fillBuffer4();

	display.clearDisplay();
	display.setTextSize(2);
	display.setCursor(20, 24);
	display.println("ready");
	display.display();

	Serial.println("Files loaded successfully!");
	Serial.println("System ready - waiting for pin input...");
	Serial.println("---");

	i2s.setBCLK(28);
	i2s.setDATA(27);
	i2s.setBitsPerSample(16);
	if (!i2s.begin(44100))
	{
		Serial.println("I2S FAILED!");
		while (1)
			;
	}
	Serial.println("I2S initialized at 44100 Hz");
}

void loop()
{
	bool active1 = digitalRead(PIN1);
	bool active2 = digitalRead(PIN2);
	bool active3 = digitalRead(PIN3);
	bool active4 = digitalRead(PIN4);

	if (display_counter % DISPLAY_UPDATE_INTERVAL == 0)
	{
		updateDisplay(active1, active2, active3, active4);
	}
	display_counter++;

	// Debug output to Serial
	if (debug_counter % DEBUG_UPDATE_INTERVAL == 0)
	{
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
		if (active1)
		{
			Serial.print("Drums ");
			any_active = true;
		}
		if (active2)
		{
			Serial.print("Synth ");
			any_active = true;
		}
		if (active3)
		{
			Serial.print("Funk ");
			any_active = true;
		}
		if (active4)
		{
			Serial.print("Groove ");
			any_active = true;
		}

		if (!any_active)
		{
			Serial.print("READY (standby)");
		}
		Serial.println();
	}
	debug_counter++;

	int32_t mixed = 0;
	int active_count = 0;

	if (active1)
	{
		if (buf_pos1 >= BUFFER_SIZE || !buf1_valid)
		{
			fillBuffer1();
		}
		mixed += (int32_t)buffer1[buf_pos1++];
		active_count++;
		pos1++;
		if (pos1 >= file1_samples)
		{
			pos1 = 0;
			file1.seek(44);
			fillBuffer1();
		}
	}

	if (active2)
	{
		if (buf_pos2 >= BUFFER_SIZE || !buf2_valid)
		{
			fillBuffer2();
		}
		mixed += (int32_t)buffer2[buf_pos2++];
		active_count++;
		pos2++;
		if (pos2 >= file2_samples)
		{
			pos2 = 0;
			file2.seek(44);
			fillBuffer2();
		}
	}

	if (active3)
	{
		if (buf_pos3 >= BUFFER_SIZE || !buf3_valid)
		{
			fillBuffer3();
		}
		mixed += (int32_t)buffer3[buf_pos3++];
		active_count++;
		pos3++;
		if (pos3 >= file3_samples)
		{
			pos3 = 0;
			file3.seek(44);
			fillBuffer3();
		}
	}

	if (active4)
	{
		if (buf_pos4 >= BUFFER_SIZE || !buf4_valid)
		{
			fillBuffer4();
		}
		mixed += (int32_t)buffer4[buf_pos4++];
		active_count++;
		pos4++;
		if (pos4 >= file4_samples)
		{
			pos4 = 0;
			file4.seek(44);
			fillBuffer4();
		}
	}

	if (active_count > 0)
	{
		mixed /= active_count;
	}

	if (mixed > 32767)
		mixed = 32767;
	if (mixed < -32768)
		mixed = -32768;

	int16_t output = (int16_t)mixed;
	i2s.write(output);
	i2s.write(output);
}