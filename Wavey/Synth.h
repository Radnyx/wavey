#ifndef SYNTH_H
#define SYNTH_H
#include <atomic>

namespace Waveform {
	float sin(float x);
	float square(float x, float duty);
	float sawtooth(float x);
	float triangle(float x);
}

namespace Synth {
	constexpr int SAMPLE_RATE = 44100;
	constexpr int NUM_CHANNELS = 5; 
	constexpr int SINE = 0b0001,
		SQUARE = 0b0010,
		SAWTOOTH = 0b0100,
		TRIANGLE = 0b1000;

	struct Channel {
		float freq = 0.0f;
		bool on = true;
		float progress = 0.0f;
	};
	/*
		0 - root
		1 - 3rd
		2 - 5th
		3 - 7th
		4 - 9th
		( TODO: distinct bass channel ? )
	*/
	extern Channel channels[NUM_CHANNELS];

	struct Config {
		// SDL millis of attack
		long startTime = 0;
		// master volume
		float volume = 1.0f;
		// modulate harmonic amplitudes
		float harmonicOffset = 0.0f;
		// shift frequency by Hz
		float shift = 0.0f;
		// square wave width
		float duty = 0.0f;
		// square wave width/cycle speed
		std::atomic<float> dutyOffset = 0.5f,
			dutyRate = 0.0f;
		// number of harmonics to mix
		std::atomic<int> harmonics = 1;
		// update harmonic offset 
		std::atomic<float> harmonicVelocity = 0.2f;
		// Depth and rate in Hz
		std::atomic<float> vibratoDepth, vibratoRate;
		// ADSR envelope, time in seconds
		std::atomic<float> attack = 0.2f, 
			release = 1.25f;
		unsigned char waveforms = SINE;
	};
	extern Config config;

	void init();
	void destroy();

	void resetNote();
	void duty();
	void attackRelease();
	void vibrato();
	void harmonics();
}

#endif // SYNTH_H