/*    
	AC101 Codec driver library example.
	Uses the ESP32-A1S module with integrated AC101 codec, mounted on the ESP32 Audio Kit board:
	https://wiki.ai-thinker.com/esp32-audio-kit

	Required library: ESP8266Audio https://github.com/earlephilhower/ESP8266Audio

	Copyright (C) 2019, Ivo Pullens, Emmission
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorRTTTL.h"
#include "AudioOutputI2S.h"
#include "AC101.h"

const char song[] PROGMEM = 
"Batman:d=8,o=5,b=180:d,d,c#,c#,c,c,c#,c#,d,d,c#,c#,c,c,c#,c#,d,d#,c,c#,c,c,c#,c#,f,p,4f";
// Plenty more at: http://mines.lumpylumpy.com/Electronics/Computers/Software/Cpp/MFC/RingTones.RTTTL

AudioGeneratorRTTTL *rtttl;
AudioFileSourcePROGMEM *file;
AudioOutputI2S *out;

#define IIS_SCLK                    27
#define IIS_LCLK                    26
#define IIS_DSIN                    25

#define IIC_CLK                     32
#define IIC_DATA                    33

#define GPIO_PA_EN                  GPIO_NUM_21
#define GPIO_SEL_PA_EN              GPIO_SEL_21

#define PIN_PLAY                    (23)      // KEY 4
#define PIN_VOL_UP                  (18)      // KEY 5
#define PIN_VOL_DOWN                (5)       // KEY 6

static AC101 ac;

static uint8_t volume = 5;
const uint8_t volume_step = 2;

void setup()
{
	Serial.begin(115200);

	Serial.printf("Connect to AC101 codec... ");
	while (not ac.begin(IIC_DATA, IIC_CLK))
	{
		Serial.printf("Failed!\n");
		delay(1000);
	}
	Serial.printf("OK\n");

	ac.SetVolumeSpeaker(volume);
	ac.SetVolumeHeadphone(volume);
//  ac.DumpRegisters();

	// Enable amplifier
	pinMode(GPIO_PA_EN, OUTPUT);
	digitalWrite(GPIO_PA_EN, HIGH);

	// Configure keys on ESP32 Audio Kit board
	pinMode(PIN_PLAY, INPUT_PULLUP);
	pinMode(PIN_VOL_UP, INPUT_PULLUP);
	pinMode(PIN_VOL_DOWN, INPUT_PULLUP);

	// Create audio source from progmem, enable I2S output,
	// configure I2S pins to matchn the board and create ringtone generator.
	file = new AudioFileSourcePROGMEM( song, strlen_P(song) );
	out = new AudioOutputI2S();
	out->SetPinout(IIS_SCLK /*bclkPin*/, IIS_LCLK /*wclkPin*/, IIS_DSIN /*doutPin*/);
	rtttl = new AudioGeneratorRTTTL();

	Serial.printf("Use KEY4 to play, KEY5/KEY6 for volume Up/Down\n");
}

bool pressed( const int pin )
{
	if (digitalRead(pin) == LOW)
	{
		delay(500);
		return true;
	}
	return false;
}

void loop()
{
	bool updateVolume = false;

	if (pressed(PIN_PLAY) and not rtttl->isRunning())
	{
		// Start playing
		file->seek(0, SEEK_SET);
		rtttl->begin(file, out);
		updateVolume = true;
	}

	if (rtttl->isRunning())
	{
		if (!rtttl->loop())
		{
			rtttl->stop();
			// Last note seems to loop after stop.
			// To silence also set volume to 0.
			ac.SetVolumeSpeaker(0);
			ac.SetVolumeHeadphone(0);
		}
	}

	if (pressed(PIN_VOL_UP))
	{
		if (volume <= (63-volume_step))
		{
			// Increase volume
			volume += volume_step;
			updateVolume = true;
		} 
	}
	if (pressed(PIN_VOL_DOWN))
	{
		if (volume >= volume_step)
		{
			// Decrease volume
			volume -= volume_step;
			updateVolume = true;
		} 
	}
	if (updateVolume)
	{
		// Volume change requested
		Serial.printf("Volume %d\n", volume);
		ac.SetVolumeSpeaker(volume);
		ac.SetVolumeHeadphone(volume);
	}
}

