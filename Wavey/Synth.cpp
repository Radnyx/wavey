#include "Synth.h"
#include <iostream>
#include <algorithm>
#include <SDL.h>

constexpr int SIN_RESOLUTION = 1024;
float SIN_TABLE[SIN_RESOLUTION];
float COS_TABLE[256];
void computeSinCos() {
	for (int i = 0; i < SIN_RESOLUTION; i++) {
		SIN_TABLE[i] = std::sinf(i / ((float)SIN_RESOLUTION) * 2.0f * (float)M_PI);
	}
	for (int i = 0; i < 256; i++) {
		COS_TABLE[i] = std::cosf(i / 256.0f * 2.0f * (float)M_PI);
	}
}
float sinLookup(float x) {
	double dummy;
	x = (float)std::modf(x / (2.0f * (float)M_PI), &dummy);
	return SIN_TABLE[(int)(x * SIN_RESOLUTION)];
}
float cosLookup(float x) {
	double dummy;
	x = (float)std::modf(x / (2.0f * (float)M_PI), &dummy);
	return COS_TABLE[(int) (x * 256)];
}

namespace Waveform {
	float sin(float x) {
		return sinLookup(x);
	}

	float square(float x, float duty) {
		double dummy;
		x = (float)std::modf(x / (2.0f * (float)M_PI), &dummy);
		return x < duty ? 1.0f : -1.0f;
	}

	float sawtooth(float x) {
		double dummy;
		x = (float)std::modf(x / (2.0f * (float)M_PI), &dummy);
		return 2 * x - 1;
	}

	float triangle(float x) {
		double dummy;
		x = (float)std::modf(x / (2.0f * (float)M_PI), &dummy);
		if (x < 0.5f) {
			return 4.0f * x - 1.0f;
		}
		else {
			return -4.0f * x + 3.0f;
		}
	}
}

namespace Synth {
	constexpr int SAMPLES = 1024; 
	constexpr float REVERB_DIST = 0.2f;
	constexpr int REVERB_SAMPLES = (int) (SAMPLE_RATE * REVERB_DIST);
	float reverb[REVERB_SAMPLES];
	int reverbFrame = 0;
	Channel channels[NUM_CHANNELS];
	Config config;
	SDL_AudioDeviceID device;

	float wave(float x, unsigned char params) {
		float mix = 0.0f;
		if (params & 0b0001) {
			mix += Waveform::sin(x);
		} 
		if (params & 0b0010) {
			mix += Waveform::square(x, config.duty) * 0.25f;
		}
		if (params & 0b0100) {
			mix += Waveform::sawtooth(x) * 0.3f;
		}
		if (params & 0b1000) {
			mix += Waveform::triangle(x) * 0.6f;
		}
		return mix;
	}

	float waveHarmonics(float x, unsigned char params, int depth, float harms[8]) {
		float mix = 0.0f;
		float vol = 1.0f;
		for (int i = 0; i < depth; i++) {
			mix += harms[i] * wave((i + 1) * x, params) * vol;
			vol *= 0.75f;
		}
		return mix;
	}

	void genSamples(float * stream, int length) {
		const float mul = (float)(2.0f * M_PI) / SAMPLE_RATE;

		// Calculate where to begin in the waveform
		int offset[NUM_CHANNELS];
		for (int i = 0; i < NUM_CHANNELS; i++) {
			if (channels[i].on) {
				offset[i] = (int) (channels[i].progress * SAMPLE_RATE / 
					(channels[i].freq / 2.0f + config.shift));
			}
		}

		float harms[8];
		for (int i = 0; i < config.harmonics; i++) {
			harms[i] = cosLookup(i * config.harmonicOffset + i);
		}

		for (int i = 0; i < length; i ++) {
			float mix = 0.0f;
			for (int j = 0; j < NUM_CHANNELS; j++) {
				if (channels[j].on) {
					const float x = (channels[j].freq / 2.0f + config.shift)
						* (offset[j] + i) * mul;
					mix += waveHarmonics(x, config.waveforms, config.harmonics, harms);
				}
				// On the last sample, record from 0.0-1.0 the location 
				// across the current waveform.
				if (i == length - 1) {
					const float x = (channels[j].freq / 2.0f + config.shift)
						* (offset[j] + i + 1) * mul;
					double dummy;
					channels[j].progress = (float)std::modf(x / (2.0f * (float)M_PI), &dummy);
				}
			}
			mix *= config.volume;
			mix += reverb[reverbFrame] * 0.5f;
			stream[i] = mix;
			reverb[reverbFrame] = mix;
			reverbFrame = (reverbFrame + 1) % REVERB_SAMPLES;
		}
	}

	void callback(void *, Uint8 * stream, int length) {
		genSamples((float *)stream, length / 4);
	}

	void init() {
		if (SDL_Init(SDL_INIT_AUDIO) < 0) {
			std::cout << "SDL_Init failed: " << SDL_GetError() << std::endl;
		}

		// Initialize SDL audio specifications
		SDL_AudioSpec desired;
		desired.freq = SAMPLE_RATE;
		desired.format = AUDIO_F32;
		desired.channels = 1;
		desired.samples = SAMPLES;
		desired.callback = callback;

		SDL_AudioSpec obtained;
		device = SDL_OpenAudioDevice(
			NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE
		);
		if (device == 0) {
			std::cout << "SDL_OpenAudioDevice failed: " << SDL_GetError() << std::endl;
		}

		SDL_PauseAudioDevice(device, 0);

		for (int i = 0; i < REVERB_SAMPLES; i++) reverb[i] = 0.0f;

		computeSinCos();
	}

	void destroy() {
		SDL_CloseAudioDevice(device);
		SDL_Quit();
	}

	void resetNote() {
		config.startTime = SDL_GetTicks();
	}

	void attackRelease() {
		const float time = (SDL_GetTicks() - config.startTime) / 1000.0f;
		if (time >= config.attack) {
			float releaseTime = time - config.attack;
			config.volume = std::max(0.0f, (config.release - releaseTime) / config.release);
		}
		else {
			config.volume = std::min(1.0f, 1.0f - (config.attack - time) / config.attack);
		}
	}

	void vibrato() {
		const float seconds = SDL_GetTicks() / 1000.0f;
		config.shift = config.vibratoDepth * std::sinf(
			config.vibratoRate * seconds * 2 * (float)M_PI);
	}

	void harmonics() {
		config.harmonicOffset += config.harmonicVelocity;
	}

	void duty() {
		if (config.dutyRate == 0.0f) {
			config.duty = config.dutyOffset;
		}
		else {
			const float seconds = SDL_GetTicks() / 1000.0f;
			config.duty = 0.5f + 0.4f * std::sinf(
				config.dutyRate * seconds * 2 * (float)M_PI);
		}
	}
}
