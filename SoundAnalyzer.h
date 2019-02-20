//+--------------------------------------------------------------------------
//
// SoundFrameIRQ - (c) 2018 Dave Plummer.  All Rights Reserved.
//
// File:        SoundAnalyzer.h 
//
// Description:
//
//   Interrupt driven code that samples at (for example) 36000Hz on a timer
//   IRQ.  
//
// History:     Sep-12-2018         Davepl      Commented
//
//---------------------------------------------------------------------------

#pragma once

const size_t    MAX_SAMPLES		   = 512;
const size_t    SAMPLING_FREQUENCY = 25000;

#define PRINT_PEAKS				0
#define SHOW_SAMPLE_TIMING		0
#define SHOW_FFT_TIMING			0

// Depending on how many bamds have been defined, one of these tables will contain the frequency
// cutoffs for that "size" of a spectrum display.  Really only the 32 band is "scientific" in any
// sense, the rest are tuned to look good.  And really only the 16 band has had a lot of work.

static int cutOffs32Band[32] = 
{
	10, 20, 25, 31, 40,	50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 
    1250, 1600, 2000, 2500, 3150, 4000, 5000, 6400, 8000, 10000, 12500, 16500, 20000
};

static int cutOffs24Band[24] =
{
    40, 80, 150, 220, 270, 320, 380, 440, 540, 630,  800, 1000, 1250, 1600, 2000, 2500, 3150, 
    3800, 4200, 4800, 5400, 6200, 7400, 12500 
};

static int cutOffs16Band[16] =
{
	100, 250, 450, 565, 715, 900, 1125, 1400, 1750, 2250, 2800, 3150, 4000, 5000, 6400, 12500
};

static int cutOffs8Band [8] =
{
	20, 150, 400, 750, 751, 752, 800, 1200
};

// SampleBuffer
//
// Contains the actual samples; the timer IRQ calls us every 1/Nth of second to take a new sample. When we get full
// someone will call FFT() and ProcessPeaks() and then GetPeaks() will return a set of peaks, one per band, from
// that sample data.
//
// To maintain a continuous flow of samples (ABC - Always Be Crunching) we acquire the samples under interrupt into
// one buffer while processing the FFT on the other.  The SamplerController manages who is doing what to what buffer.
// A Mutex protects each buffer - you must hold the mutex before modifying.

class SampleBuffer
{
  private:
	arduinoFFT		  _FFT;                 // Perhaps could be static, but might have state info, so one per buffer
	size_t            _MaxSamples;          // Number of samples we will take, must be a power of 2
	size_t            _SamplingFrequency;   // Sampling Frequency should be at least twice that of highest freq sampled
	size_t            _BandCount;
	float			* _vPeaks; 
	int				  _InputPin;
	static float      _oldVU;
	portMUX_TYPE	  _mutex;

	// BucketFrequency
	//
	// Return the frequency corresponding to the Nth sample bucket.  Skips the first two 
	// buckets which are overall amplitude and something else.

	int BucketFrequency(int iBucket) const
	{
		if (iBucket <= 1)
			return 0;

		int iOffset = iBucket - 2;
		return iOffset * (_SamplingFrequency / 2) / (_MaxSamples / 2);
	}

	// BandCutoffTable
	//
	// Depending on how many bands we have, returns the cutoffs of where those bands are in the spectrum

	static int * BandCutoffTable(int bandCount)				
	{
		if (bandCount == 8)
			return cutOffs8Band;
		if (bandCount == 16)
			return cutOffs16Band;
		if (bandCount == 24)
			return cutOffs24Band;
		if (bandCount == 32)
			return cutOffs32Band;
		return cutOffs32Band;
	}

  public:

	volatile int	  _cSamples;
	double			* _vReal;
	double			* _vImaginary;

	SampleBuffer(size_t MaxSamples, size_t BandCount, size_t SamplingFrequency, int InputPin)
	{
		_BandCount		   = BandCount;
		_SamplingFrequency = SamplingFrequency;
		_MaxSamples        = MaxSamples;
		_InputPin          = InputPin;

		_vReal			   = (double *) malloc(MaxSamples * sizeof(_vReal[0]));
		_vImaginary		   = (double *) malloc(MaxSamples * sizeof(_vImaginary[0]));
		_vPeaks			   = (float *)  malloc(BandCount  * sizeof(_vPeaks[0]));
		_oldVU			   = 0.0f;

		_mutex = portMUX_INITIALIZER_UNLOCKED;
		vPortCPUInitializeMutex(&_mutex);

		Reset();
	}
	~SampleBuffer()
	{
		free(_vReal);
		free(_vImaginary);
		free(_vPeaks);
	}

	bool TryForImmediateLock()
	{
		return vPortCPUAcquireMutexTimeout(&_mutex, portMUX_TRY_LOCK);
	}

	void WaitForLock() 
	{
		vPortCPUAcquireMutex(&_mutex);
	}

	void ReleaseLock()
	{
		vPortCPUReleaseMutex(&_mutex);
	}
    
    // SampleBuffer::Reset
    //
    // Resets (clears) everything about the buffer except for the time stamp.

	void Reset()
	{
		_cSamples = 0;
		for (int i = 0; i < _MaxSamples; i++)
		{
			_vReal[i] = 0.0;
			_vImaginary[i] = 0.0f;
		}
		for (int i = 0; i < _BandCount; i++)
			_vPeaks[i] = 0;
	}

    // SampleBuffer::FFT
    //
    // Run the FFT on the sample buffer.  When done the first two buckets are VU data and only the first MAX_SAMPLES/2
    // are valid.  For each bucket afterwards you can call BucketFrequency to find out what freq corresponds to what bucket

	void FFT()
	{
		#if SHOW_FFT_TIMING
		unsigned long fftStart = millis();
		#endif

		_FFT.Windowing(_vReal, _MaxSamples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
		_FFT.Compute(_vReal, _vImaginary, _MaxSamples, FFT_FORWARD);
		_FFT.ComplexToMagnitude(_vReal, _vImaginary, _MaxSamples);              // This and MajorPeak may only actually need _MaxSamples/2
        _FFT.MajorPeak(_vReal, _MaxSamples, _SamplingFrequency);                //   but I can't tell, and it was no perf win when I tried it.91939193919391939193919391919389191239

		#if SHOW_FFT_TIMING
		Serial.printf("FFT took %ld ms at %d FPS\n", millis() - fftStart, FPS(fftStart, millis()));
		#endif
	}
	
	inline bool IsBufferFull() const __attribute__((always_inline))
	{
		return (_cSamples >= _MaxSamples);
	}

    // SampleBuffer::AcquireSample
    //
    // IRQ calls here through the IRQ stub

	void AcquireSample()
	{
		// If we can't lock the buffer, we just do nothing.  The buffer shouldn't be busy for long, but way too long
		// to wait on in an ISR, so if we can't lock it immediately, just bail until the next timer IRQ fires.  We keep
		// some statistics about how many interrupts in total were fired vs how many times we were able to get data vs
		// how often the buffer was locked.  Ideally you want 99-100% consumption and no misses!
		
        g_cInterrupts++;

		if (TryForImmediateLock())				
		{
			if (_cSamples < _MaxSamples)
			{ 
				_vReal[_cSamples] = analogRead(_InputPin);
				_vImaginary[_cSamples] = 0;
				_cSamples++;
				g_cSamples++;
			}
			ReleaseLock();
		}
		else
			g_cIRQMisses++;
	}

    // SampleBuffer::ProcessPeaks
    //
    // Runs through and figures out what the peak level is in each of the bands.  Also calculates
    // the overall VU level and adjusts the auto gain.

	void ProcessPeaks()
	{
		static const int NOISE_CUTOFF = 10;

		float averageSum = 0.0f;
		double samplesPeak = 0.0f;
		for (int i = 2; i < _MaxSamples / 2; i++)
		{
			averageSum += _vReal[i];
			if (_vReal[i] > samplesPeak)
				samplesPeak = _vReal[i];
		}

		float t = averageSum / (_MaxSamples / 2);
		gVU = max(t, (_oldVU * 3 + t) / 4);
		_oldVU = gVU;

		for (int i = 2; i < _MaxSamples / 2; i++)
		{
			if (_vReal[i] > powf(NOISE_CUTOFF, gLogScale))
			{
				int freq = BucketFrequency(i);

				int iBand = 0;
				while (iBand < _BandCount)
				{
					if (freq < BandCutoffTable(_BandCount)[iBand])
						break;
					iBand++;
				}
				if (iBand > _BandCount)
					iBand = _BandCount;

				float scaledValue = _vReal[i];
				if (scaledValue > _vPeaks[iBand])
					_vPeaks[iBand] = scaledValue;
			}
		}

		#if PRINT_PEAKS			
		Serial.print("Raws:  ");
		for (int i = 0; i < _BandCount; i++)
		{
			Serial.printf("%8.1f, ", _vPeaks[i]);
		}
		Serial.println("");
		#endif
   
        // HACKHACK (davepl) 
		//
		// Egregious hand-tuning of the spectrum, these simply make the response look more linear to pink noise

        if (_BandCount == 16 && gVU > (MAX_VU / 8))
        {
		    _vPeaks[0] *= 0.3f;											// Bring down the low bass a little bit
		    _vPeaks[1] *= 0.6f;
		    _vPeaks[2] *= 0.8f;
			// ...
            _vPeaks[9]  *= 1.10f;
            _vPeaks[10] *= 1.25f;
            _vPeaks[11] *= 1.40f;
            _vPeaks[12] *= 1.60f;
            _vPeaks[13] *= 1.80f;
            _vPeaks[14] *= 1.90f;
            _vPeaks[15] *= 2.00f;
        }

        // First we're going to scale our data up exponentially, then scale it down linearly, which should give us a logrithmic (or exponential?) display

		for (int i = 0; i < _BandCount; i++)
			_vPeaks[i] = powf(_vPeaks[i], gLogScale);

		// All Bands Peak is the peak across every band; it's the "TOP" value that we must scale the entire display to fit

		static float lastAllBandsPeak = 0.0f;
		float allBandsPeak = 0;
		for (int i = 0; i < _BandCount; i++)
			allBandsPeak= max(allBandsPeak, _vPeaks[i]);
		
		if (allBandsPeak < 1)
			allBandsPeak = 1;
		
		// The followinf picks allBandsPeak if it's gone up.  If it's gone down, it "averages" it by faking a running average of GAIN_DAMPEN past peaks

		allBandsPeak = max(allBandsPeak, ((lastAllBandsPeak * (GAIN_DAMPEN-1)) + allBandsPeak) / GAIN_DAMPEN);	// Dampen rate of change a little bit on way down
		lastAllBandsPeak = allBandsPeak;

		// Now scale everything so that the peak is at 1.0f and everything else is fractional relative to it.    We never go below a meximum
		// gain (for example, 2^26) so that we don't amplify hiss and noise to the point it shows up on the display.

		if (allBandsPeak < powf(2, 26))
			allBandsPeak = powf(2, 26);

		for (int i = 0; i < _BandCount; i++)
			_vPeaks[i] /= (allBandsPeak * 1.1f);
		gScaler = allBandsPeak;

        #if PRINT_PEAKS
		Serial.print("Aftr:  ");
		for (int i = 0; i < _BandCount; i++)
		{
			Serial.printf("%8.1f, ", _vPeaks[i]);
		}
		Serial.println("");
        #endif
	}
    
    // SampleBuffer::GetBandPeaks
    //
    // Once the FFT processing is complete you can call this function to get a copy of what each of the
    // peaks in the various bands is

	PeakData GetBandPeaks()
	{
		PeakData data;
		for (int i = 0; i < _BandCount; i++)
			data.Peaks[i] = _vPeaks[i];
		return data;
	}

};
float      SampleBuffer::_oldVU;

class SoundAnalyzer
{
  private:

	hw_timer_t	  * _SamplerTimer = NULL;													// The timer which will first SAMPLING_FREQUENCY times per second (like 32000)
	SampleBuffer    _bufferA;																// A front buffer and a back buffer
	SampleBuffer	_bufferB;
	unsigned int	_sampling_period_us = PERIOD_FROM_FREQ(SAMPLING_FREQUENCY);
	uint8_t			_inputPin;																// Which hardware pin do we actually sample audio from?

	static volatile SampleBuffer * _pIRQBuffer;												// Static because there is only one, and it lives at global scoope	
																							//  Volatile because the IRQ code could touch it when you're not paying attention
  public:

	SoundAnalyzer(uint8_t inputPin)
		: _bufferA(MAX_SAMPLES, BAND_COUNT, SAMPLING_FREQUENCY, INPUT_PIN),
		  _bufferB(MAX_SAMPLES, BAND_COUNT, SAMPLING_FREQUENCY, INPUT_PIN),
 		  _sampling_period_us(PERIOD_FROM_FREQ(SAMPLING_FREQUENCY)),
		  _inputPin(inputPin)
	{
		_pIRQBuffer = &_bufferA;
	}

	// SoundAnalyzer::StartInterrupts
    //
    // Sets a time interrupt to fire every _Samping_period_us (in microseconds).  The timers run on an 80MHz clock
    // so the scale that down to 1M per second (microseconds).  

    void StartInterrupts()
	{
		Serial.printf("Continual sampling every %d us for a sample rate of %d Hz.\n", _sampling_period_us, SAMPLING_FREQUENCY);

		// Timer interrupt
		_SamplerTimer = timerBegin(0, 80, true);					// Scalar for 80Mhz 
		timerAttachInterrupt(_SamplerTimer, &OnTimer, true);		// Set callback
		timerAlarmWrite(_SamplerTimer, _sampling_period_us, true);	// Set number of 1MHz events to fire upon and set reload == true
		timerAlarmEnable(_SamplerTimer);
	}

	static void IRAM_ATTR OnTimer();

    // ScanInputs
    //
    // With interrupts DISABED, quickly checks the input pots and records the value.  Not the greatest design to have the sound ananlyzer
    // doing the user input, but only it knows when it's safe and that reading from the pot won't collide with an interrupt read

    void ScanInputs()
    {
        analogSetSamples(1);                                // Otherwise voltage sweeps through range twice
		float raw = analogRead(BRIGHTNESS_PIN);				// Brightness is logrithmic, so we do the opposite
		raw = mapFloat(raw, 0, 4096, 1.5, 10);
		raw = roundf(raw);
		gBrightness = std::min(255.0f,powf(raw, 2.52f));

        raw = analogRead(COLOR_SPEED_PIN);
        raw = mapFloat(raw, 0, 4096, 0, MAX_COLOR_SPEED);
        gColorSpeed = raw;

        
        // Peak delay for white lines.  When set all the way up to PEAK2_DECAY_PER_SECOND, they'll seem to be stuck
        // to the top of the bars.  At zero the float, in between they fall.  Below zero is just how we convey
        // "don't draw them at all".  So you can dial in -0.5 to 2.2 for example, and below 0 is OFF.

        raw = analogRead(PEAK_DECAY_PIN);
        raw = mapFloat(raw, 0, 4096, -0.5f, PEAK2_DECAY_PER_SECOND);
        gPeakDecay = raw;

        raw = analogRead(COLOR_SCHEME_PIN);
        raw = mapFloat(raw, 0, 4096, 0, ARRAYSIZE(allPalettes));
        giColorScheme = raw;

        analogSetSamples(SUPERSAMPLES);
    }

    // RunSamplerPass
    //
    // Swap the front and rear buffers then run the FFT

    PeakData RunSamplerPass(int bandCount)
	{
		SampleBuffer * pBackBuffer = nullptr;

		for (;;)
		{
			if (_bufferA._cSamples == MAX_SAMPLES)
			{
				portDISABLE_INTERRUPTS();
                ScanInputs();
				_bufferB.Reset();
				_pIRQBuffer = &_bufferB;
				pBackBuffer = &_bufferA;
				portENABLE_INTERRUPTS();
				break;
			}
			if (_bufferB._cSamples == MAX_SAMPLES)
			{
				portDISABLE_INTERRUPTS();
                ScanInputs();
				_bufferA.Reset();
				_pIRQBuffer = &_bufferA;
				pBackBuffer = &_bufferB;
				portENABLE_INTERRUPTS();
				break;
			}
			delay(0);
		}

		pBackBuffer->WaitForLock();
    	    pBackBuffer->FFT();
		    pBackBuffer->ProcessPeaks();
		    PeakData peaks = pBackBuffer->GetBandPeaks();
		    pBackBuffer->Reset();
		pBackBuffer->ReleaseLock();

		return peaks;
	}
};

volatile SampleBuffer * SoundAnalyzer::_pIRQBuffer;

void IRAM_ATTR SoundAnalyzer::OnTimer()
{
    ((SampleBuffer *)_pIRQBuffer)->AcquireSample();         // Can't call through a volatile pointer directly
}

// The globlal instance of the SoundAnalyzer

SoundAnalyzer g_SoundAnalyzer(INPUT_PIN);

