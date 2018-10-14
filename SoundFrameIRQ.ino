//+-----------------------------------------------------------------------------
//
// DAV - Dave's Audio Visualizer - (c) 2018 Dave Plummer, All Rights Reserved.
//
// File:        SpectrumAnalyzer.ino   (Main Source File) 
//
// Description:
//
//   Handles chip setup and launches the processing tasks.  Note that I 
//   am using the Heltec WiFi32 dev board for my ESP32, and Pins 
//   15, 4, and 16 are used by the built in TFT.  You can change the
//   UserInterface code to omit that if you're using a different board.
//
// Usage:
//
//   You can freely use this code to build the intended project for 
//   personal use.  You can build as many as you want for home use and
//   limited gift giving with no financial gain.  Any commerical use
//   is prohibited without prior permission.  You assume all liability
//   for the code if you use it.   This is experimental code that could
//   burn down your house.  Use it as your own risk.
//
// Dependencies:
//
//	 Library					    Version		Purpose
//   --------------------			-------		----------------
//	 ESP32 Board packages						ESP 32 Support
//	 AdaFruit GFX Library			1.2.9		Drawing primitives
//	 ardunioFFT						1.4.0		FFT math for sound
//	 FastLED						3.2.0		Control of RGB LEDs
//	 U8G2							2.23.18		Control of TFT screen
//
// History:     Sep-12-2018         Davepl@Davepl.com      Commented
//
//------------------------------------------------------------------------------

#define FASTLED_INTERNAL                                    // Make build quieter
#include <U8g2lib.h>					                    // So we can talk to the CUU text 
#include <FastLED.h>					                    // FastLED for the LED panels
#include <pixeltypes.h>                                     // Handy color and hue stuff
#include <gfxfont.h>					                    // Adafruit GFX for the panels 
#include <Fonts/FreeSans9pt7b.h>                            // A nice font for the VFD
#include <Adafruit_GFX.h>                                   // GFX wrapper so we can draw on matrix
#include <arduinoFFT.h>										// FFT code for SoundAnalzyer

#define BAND_COUNT			16                              // Choices are 8, 16, 24, or 32.  Only 16 is "pretty" and hand-tuned, but you could fix others
#define MATRIX_WIDTH		48                              // Number of pixels wide
#define MATRIX_HEIGHT		16                              // Number of pixels tall
#define GAIN_DAMPEN          2                              // Higher values cause auto gain to react more slowly
#define LED_PIN				 5                              // Data pin for matrix leds
#define INPUT_PIN			 2                              // Audio line input 
#define COLOR_SPEED_PIN     33                              // How fast palette rotates in spectrum bars 
#define MAX_COLOR_SPEED     64                              //    ...and the max allowed
#define BRIGHTNESS_PIN		25                              // Pin for brightness pot read
#define PEAK_DECAY_PIN      26                              // Pin for peak decay pot read
#define COLOR_SCHEME_PIN    27                              // Pin for controlling color scheme
#define SUPERSAMPLES         2                              // How many supersamples to take 
#define SAMPLE_BITS         12								// Sample resolution (0-4095)
#define MAX_ANALOG_IN    ((1<<SAMPLE_BITS)*SUPERSAMPLES)    // What our max analog input value is on all analog pins (4096 is default 12 bit resolution)
#define MAX_VU           12000                              // How high our VU could max out at.  Arbitarily tuned.
#define ONSCREEN_FPS         0                              // Debugging display of FPS count on LED screen
#define MS_PER_SECOND     1000                              // 1000 milliseconds per second
#define STACK_SIZE        4096							    // Stack size for each new thread

#define BLACK			0x0000                              // Color definitions in 16-bit 5-6-5 space
#define BLUE			0x001F
#define RED				0xF800
#define GREEN			0x07E0
#define CYAN			0x07FF
#define MAGENTA			0xF81F
#define YELLOW			0xFFE0 
#define WHITE			0xFFFF

volatile float         gScaler       = 0.0f;                // Instanteous read of LED display vertical scaling
volatile size_t        gFPS          = 0;				    // FFT frames per second
volatile size_t        mFPS          = 0;				    // Matrix frames per second
volatile float         gLogScale     = 2.0f;                // How exponential the peaks are made to be
volatile float         gBrightness   = 64;                  // LED matrix brightness, 0-255
volatile float         gPeakDecay    = 0.0;                 // Peak decay for white line on top of spectrum bars
volatile float         gColorSpeed   = 128.0f;              // How fast the color palette rotates (smaller is faster, it's a time divisor)
volatile float         gVU			 = 0;                   // Instantaneous read of VU value
volatile int           giColorScheme = 0;                   // Global color scheme (index into table of palettes)

volatile unsigned long g_cSamples    = 0;                   // Total number of samples successfully collected
volatile unsigned long g_cInterrupts = 0;                   // Total number of interrupts that have occured
volatile unsigned long g_cIRQMisses  = 0;                   // Number of times buffer wasn't lockable by IRQ

#include "Utilities.h"										// Functions and helpers like ARRAYSIZE for global use
#include "LEDMatrixGFX.h"									// Expose our LED panels as drawable surfaces with primitives
#include "Palettes.h"										// Color schemes for the spectrum analyzer bars
#include "SpectrumDisplay.h"								// Draws the bars on the LEDs
#include "SoundAnalyzer.h"									// Measures and processes the incoming audio

// Global Objects

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, 15, 4, 16);
LEDMatrixGFX					    gMatrix(MATRIX_WIDTH, MATRIX_HEIGHT, 255);
SpectrumDisplay						gDisplay(&gMatrix, BAND_COUNT);
SoundAnalyzer						gAnalyzer(INPUT_PIN);


// setup()
//
// Init the various pins, set up the Analog to Digital Converter (ADC), and assign work to the CPU cores

void setup()
{
	Serial.begin(115200);
	Serial.println("DAV - Dave's Audio Visualizer - (c) 2018 Plummer's Software LLC. All rights reserved.");
    Serial.println("-------------------------------------------------------------------------------------");
    Serial.println("Intializizing TFT display...");
	u8g2.begin();
	u8g2.clear();

    Serial.println("Configuring Input Pins...");
	pinMode(BRIGHTNESS_PIN, INPUT);
    pinMode(COLOR_SPEED_PIN, INPUT);
    pinMode(PEAK_DECAY_PIN, INPUT);

    Serial.println("Setting up ADC...");			// BUGBUG Hardcoded output
    Serial.println("  Resolution  : 12 bit");
    Serial.println("  Cycles      : 32");
    Serial.println("  Supersamples:  2");
    Serial.println("  Attenuation :  2.5dB");

	// Set up the ESP32 DAC

	analogReadResolution(12);						// Sets the sample bits and read resolution, default is 12-bit (0 - 4095), range is 9 - 12 bits
	analogSetWidth(12);								// Sets the sample bits and read resolution, default is 12-bit (0 - 4095), range is 9 - 12 bits
	analogSetCycles(32);							// Set number of cycles per sample, default is 8 and provides an optimal result, range is 1 - 255
	analogSetSamples(SUPERSAMPLES);					// Set number of samples in the range, default is 1, it has an effect on sensitivity has been multiplied
	analogSetClockDiv(1);							// Set the divider for the ADC clock, default is 1, range is 1 - 255
    analogSetAttenuation(ADC_11db);					// For all pins
    analogSetPinAttenuation(INPUT_PIN,ADC_2_5db);	// Sets the input attenuation for the audio pin ONLY, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
   
	//TaskHandle_t uiTask;
	
	TaskHandle_t samplerTask;
	TaskHandle_t matrixTask;
	TaskHandle_t uiTask;

    Serial.println("Scheduling CPU Cores...");

	xTaskCreatePinnedToCore(SamplerLoop,       "Sampler Loop", STACK_SIZE, nullptr, 1, &samplerTask, 0); // Sampler stuff on CPU Core 1
	xTaskCreatePinnedToCore(MatrixLoop,        "Matrix Loop",  STACK_SIZE, nullptr, 1, &matrixTask,  1); // Matrix  stuff on CPU Core 0

    Serial.println("Launching Background Task for TFT...");

	xTaskCreate(TFTUpdateLoop, "TFT Loop", STACK_SIZE, nullptr, 0, &uiTask);							// UI stuff not bound to any core and at lower priority

    Serial.println("Audio Sampler Launching...");
    Serial.printf("  FFT Size: %d bytes\n", MAX_SAMPLES);
	g_SoundAnalyzer.StartInterrupts();  
    
    Serial.println("Sampler Started!  System is OPERATIONAL.");
}

// VFDUpdateLoop
//
// Displays statistics on the Heltec's built in TFT board.  If you are using a different board, you would simply get rid of
// this or modify it to fit a screen you do have.  You could also try serial output, as it's on a low-pri thread it shouldn't
// disturb the primary cores, but I haven't tried it myself.

void TFTUpdateLoop(void *)
{
	for (;;)
	{ 
		char szBuffer[32];
		u8g2.clearBuffer();						// clear the internal memory
		u8g2.setFont(u8g2_font_profont15_tf);	// choose a suitable font
		sprintf(szBuffer, "Mat/FFT FPS: %d/%d", mFPS, gFPS);
        u8g2.drawStr(0,10,szBuffer);			// write something to the internal memory

		sprintf(szBuffer, "Auto Gain  : 2^%-3.0f", (float)log2(gScaler));
		u8g2.drawStr(0,22,szBuffer);			// write something to the internal memory

		sprintf(szBuffer, "Brightness : %3.1f", gBrightness);
		u8g2.drawStr(0,34,szBuffer);			// write something to the internal memory

        sprintf(szBuffer, "IRQ Hits/Lk: %-2.0f/%-2.0f", 100.0f * g_cSamples / g_cInterrupts, 100.0f * g_cIRQMisses / g_cInterrupts);
		u8g2.drawStr(0,46,szBuffer);			// write something to the internal memory

        sprintf(szBuffer, "Color Speed: %d", (int) gColorSpeed);
        u8g2.drawStr(0,58,szBuffer);			// write something to the internal memory

		u8g2.sendBuffer();	
	}
}

// SamplerLoop
//
// One CPU core spins in this loop, pulling completed buffers and running the FFT, etc.

void SamplerLoop(void *)
{
	unsigned long lastFrame = 0;
	for (;;)
	{ 
		gFPS = FPS(lastFrame, millis());
		lastFrame = millis();

		PeakData peaks = gAnalyzer.RunSamplerPass(BAND_COUNT);
		gDisplay.SetPeaks(BAND_COUNT, peaks);
        
        delay(5);
    }
}

// MatrixLoop
//
// The other CPU core spins in this loop continually redrawing the LED display.  With a 48x16 display it can manage
// about 40 frames per second.  We also read the brightness pot here so that it's prompt and responsive even
// if the -display- of the brightness number on the VFD is slow.

void MatrixLoop(void *)
{
    static float colorShift = 0;
	unsigned long lastTime = 0;
	for (;;)
	{
		mFPS = FPS(lastTime, millis());
        float secondsElapsed = (millis() - lastTime) / (float) MS_PER_SECOND;
		lastTime = millis();

		gMatrix.fillScreen(BLACK16);

        // When the speed is set to zero (or close... below 2) we don't just stop scrolling the color, we also reset to the left so that
        // the flag colors line up and so on

        if (gColorSpeed < 2)
        { 
            gDisplay.Draw(0);
        }
        else
        {
            colorShift += gColorSpeed * secondsElapsed;
            while (colorShift >= 256)
                colorShift -= 256;

            gDisplay.Draw((byte)colorShift);	
        }

        #if ONSCREEN_FPS
		gMatrix.setTextColor(RED16);
		gMatrix.setCursor(20, 0);
		gMatrix.print(gFPS);
	
		gMatrix.setTextColor(BLUE16);
		gMatrix.setCursor(0, 0);
		gMatrix.print(mFPS);
        #endif

		gMatrix.setBrightness(gBrightness);                          // gBrightness value from pot
		gMatrix.ShowMatrix();

		yield();
	}
}

// loop()
//
// This is where the Arduino framework would normally do all of your work, but we scheduled our background task and
// assigned tasks to CPU cores in setup(), so this function does nothing.

void loop()
{
	delay(portMAX_DELAY);
}
