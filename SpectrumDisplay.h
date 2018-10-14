//+--------------------------------------------------------------------------
//
// SpectrumDisplayIRQ - (c) 2018 Dave Plummer.  All Rights Reserved.
//
// File:        SpectrumDisplay.h
//
// Description:
//
//   Spectrum analyzer display.  Draws the bands and the peak highlights
//   and the VU meter.
//
// History:     Sep-12-2018         Davepl      Commented
//
//---------------------------------------------------------------------------

#define PEAK2_DECAY_PER_SECOND  2.2f          
#define SHADE_BAND_EDGE           0

// PeakData class
//
// Simple data class that holds the music peaks for up to 32 bands.  When the sound analyzer finishes a pass, its
// results are simplified down to this small class of band peaks.

class PeakData
{
  public:

  float Peaks[BAND_COUNT];

  PeakData()
  {
    for (int i = 0; i < ARRAYSIZE(Peaks); i++)
      Peaks[i] = 0.0f;
  }
};

// SpectrumDisplay
//
// Responsible for drawing the spectrum analyzer on the RGB LED matrix given a never ending
// series of PeakData objects coming out of the FFT thread

class SpectrumDisplay
{
  private:

    LEDMatrixGFX    * _pMatrix;
    byte              _numberOfBands;

    PeakData          _peaks;

    float             _peak1Decay[BAND_COUNT] = { 0 };
    float             _peak2Decay[BAND_COUNT] = { 0 };
  
    unsigned long     _lastPeak1Time[BAND_COUNT] = { 0 } ;

    // SpectrumDisplay::DecayPeaks
    //
    // Every so many ms we decay the peaks by a given amount

    void DecayPeaks()
    {
        static unsigned long lastDecay = 0;
        float seconds = (millis() - lastDecay) / (float)MS_PER_SECOND;
        lastDecay = millis();

        float decayAmount1 = max (0, seconds * gPeakDecay);
        float decayAmount2 = seconds * PEAK2_DECAY_PER_SECOND;

        for (int iBand = 0; iBand < BAND_COUNT; iBand++)
        {
            _peak1Decay[iBand] -= min(decayAmount1, _peak1Decay[iBand]);    
            _peak2Decay[iBand] -= min(decayAmount2, _peak2Decay[iBand]);    
        }
    }

    // Display::DrawBand
    //
    // Draws the bar graph rectangle for a bar and then the white line on top of it

    void DrawBand(byte iBand, uint16_t baseColor)
    {
        int value  = _peak1Decay[iBand]  * (_pMatrix->height() - 1);
        int value2 = _peak2Decay[iBand] * _pMatrix->height();

        if (value > _pMatrix->height())
            value = _pMatrix->height();
    
        if (value2 > _pMatrix->height())
            value2 = _pMatrix->height();

        int bandWidth = _pMatrix->width() / _numberOfBands;
        int xOffset   = iBand * bandWidth;
        int yOffset   = _pMatrix->height() - value;
        int yOffset2  = _pMatrix->height() - value2;
    
       int iRow = 0;
       for (int y = yOffset2; y < _pMatrix->height(); y++)
        {
            CRGB color = _pMatrix->from16Bit(baseColor);
            iRow++;
            _pMatrix->drawLine(xOffset, y, xOffset+bandWidth-1, y, _pMatrix->to16bit(color));
        }
    
        #if SHADE_BAND_EDGE
            CRGB color = _pMatrix->from16Bit(baseColor);
            color.fadeToBlackBy(32);
            baseColor = _pMatrix->to16bit(color);
            _pMatrix->drawLine(xOffset+bandWidth-1, yOffset2, xOffset+bandWidth-1, _pMatrix->height(), baseColor);
        #endif

        const int PeakFadeTime_ms = 1000;

        CRGB colorHighlight = CRGB(CRGB::White);
	    unsigned long msPeakAge = millis() - _lastPeak1Time[iBand];
	    if (msPeakAge > PeakFadeTime_ms)
		    msPeakAge = PeakFadeTime_ms;
	    
        float agePercent = (float) msPeakAge / (float) MS_PER_SECOND;
	    byte fadeAmount = min(255, agePercent * 256);

        colorHighlight = CRGB(CRGB::White).fadeToBlackBy(fadeAmount);

        if (value == 0)
		    colorHighlight = _pMatrix->from16Bit(baseColor);

        // if gPeakDecay is less than zero we interpret that here to mean "don't draw it at all".  

        if (gPeakDecay >= 0.0f)
            _pMatrix->drawLine(xOffset, max(0, yOffset-1), xOffset + bandWidth - 1, max(0, yOffset-1),_pMatrix->to16bit(colorHighlight));
    }

  public:

    SpectrumDisplay(LEDMatrixGFX * pgfx, byte numberOfBands)
    {
        _pMatrix = pgfx;
        _numberOfBands = numberOfBands;
    }

    // Display::SetPeaks
    //
    // Allows the analyzer to call and set the peak data for all of the bands at once.

    void SetPeaks(byte bands, PeakData peakData)
    {
        //Serial.print("SetPeaks: ");
        for (int i = 0; i < bands; i++)
        {
            //Serial.printf("%f, ", peakData.Peaks[i]);

            if (peakData.Peaks[i] > _peak1Decay[i])
	        {
                _peak1Decay[i] = peakData.Peaks[i];
		        _lastPeak1Time[i] = millis();				// For the white line top peak we track when it was set so we can age it out visually
	        }
            if (peakData.Peaks[i] > _peak2Decay[i])
	        {
                _peak2Decay[i] = peakData.Peaks[i];
	        }
        }
        //Serial.println("");
    }

    void Draw(int baseHue)
    {
        _pMatrix->fillScreen(BLACK);
        for (int i = 0; i < _numberOfBands; i++)
        {
            CRGB color = ColorFromPalette(allPalettes[giColorScheme], i*16 + baseHue);           
            DrawBand(i, _pMatrix->to16bit(color));
        }
        DrawVUMeter(0);
        DecayPeaks();
    }

    // DrawVUMeter
    // 
    // Draws the symmetrical VU meter along with its fading peaks up at the top of the display.

    void DrawVUMeter(int yVU)
    {
        static int iPeakVUy = 0;        // size (in LED pixels) of the VU peak
        static unsigned long msPeakVU = 0;       // timestamp in ms when that peak happened so we know how old it is

        const int MAX_FADE = 256;

        _pMatrix->fillRect(0, yVU, _pMatrix->width(), 1, BLACK16);

        if (iPeakVUy > 1)
        {
            int fade = MAX_FADE * (millis() - msPeakVU) / (float) MS_PER_SECOND;
            DrawVUPixels(iPeakVUy,   yVU, fade);
            DrawVUPixels(iPeakVUy-1, yVU, fade);
        }

        int xHalf = _pMatrix->width()/2-1;
        int bars  = map(gVU, 0, MAX_VU, 1, xHalf);
        bars = min(bars, xHalf);

        if (bars > iPeakVUy)
        {
            msPeakVU = millis();
            iPeakVUy = bars;
        }
        else if (millis() - msPeakVU > MS_PER_SECOND)
        {
            iPeakVUy = 0;
        }

        for (int i = 0; i < bars; i++)
            DrawVUPixels(i, yVU);
    }

    // SpectrumDisplay::DrawVUPixels

	void DrawVUPixels(int i, int yVU, int fadeBy = 0)
	{
		int xHalf = _pMatrix->width()/2;
		_pMatrix->drawPixel(xHalf-i-1, yVU, ColorFromPalette(vuPalette256, i*(256/xHalf)).fadeToBlackBy(fadeBy));
		_pMatrix->drawPixel(xHalf+i,   yVU, ColorFromPalette(vuPalette256, i*(256/xHalf)).fadeToBlackBy(fadeBy));
	}
};
