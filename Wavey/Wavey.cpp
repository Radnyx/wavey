#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <thread>
#include <vector>
#include <map>
#include <atomic>
#include <cassert>
#include <ctime>
#include <SDL.h>
#include "Notes.h"
#include "Chord.h"
#include "Synth.h"
#include "View.h"

std::vector<Chord> progression;

std::map<Chord::Type, std::vector<Transition>> transitions;
void computeTransitions() {
	transitions[Chord::MIN_SEV] = {
		{  0, Chord::MAJ_SEV },	// m7 =======> M7
		{  5, Chord::MIN_SEV },	// m7 = + 4 => m7
		{  5, Chord::SEV },		// m7 = + 4 =>  7
		{ -1, Chord::SEV }, 	// m7 = -m2 =>  7
	};

	transitions[Chord::MAJ_SEV] = {
		{  2, Chord::MAJ_SEV },	// M7 = + 2 => M7
		{ -2, Chord::MAJ_SEV }, // M7 = - 2 => M7
		{  9, Chord::MIN_SEV },	// M7 = + 6 => m7
		{  9, Chord::SEV },		// M7 = + 6 =>  7
	};

	transitions[Chord::SEV] = {
		{  1, Chord::SEV },		//  7 = + 1 =>  7
		{ -1, Chord::SEV },		//  7 = - 1 =>  7
		{  5, Chord::SEV },		//  7 = + 4 =>  7
		{  5, Chord::MAJ_SEV },	//  7 = + 4 => M7
		{ -1, Chord::MAJ_SEV },	//  7 = - 1 => M7
		{  0, Chord::MIN_SEV },	//  7 =======> m7
		{  0, Chord::SUS4_SEV },//  7 =======> sus4(7)
	};

	transitions[Chord::SUS4_SEV] = {
		{  0, Chord::SEV },		// sus4(7) =======> 7
		{  2, Chord::SUS4_SEV },// sus4(7) = + 2 => sus4(7) 
		{ -2, Chord::SUS4_SEV },// sus4(7) = - 2 => sus4(7) 
	};
}

/* Pick a random chord root and type. */
Chord randomChord() {
	return { 
		Notes::MIDDLE_C + rand() % 12, 
		(Chord::Type)(rand() % Chord::NUM_TYPES),
		rand() % Chord::NUM_INVERSIONS
	};
}

/* Return the closest inversion of the given root and
   chord type compared to the given chord c. */
Chord closest(const Chord & c, int root, Chord::Type type) {
	const float cog = c.centerOfGravity();
	float closest = std::numeric_limits<float>::infinity();
	Chord best;
	// pick inversion with closest "center of gravity"
	for (int i = 0; i < Chord::NUM_INVERSIONS; i++) {
		Chord c2 = { root, type, i };
		float dist = std::fabsf(cog - c2.centerOfGravity());
		if (dist < closest) {
			closest = dist;
			best = c2;
		}
	}
	return best;
}

/* Randomly choose the next chord in the progression given 
   the defined transition graph. */
Chord nextChord(const Chord & c) {
	Chord::Type type = c.type;
	const auto & options = transitions.at(type);
	const auto & trans = options.at(rand() % options.size());
	const int root = Notes::middleOctave(c.root + trans.dist);
	return closest(c, root, trans.newType);
}

void assignChord(const Chord & c) {
	Synth::channels[0].freq = Notes::freqs[c.root] / 4.0f;
	Synth::channels[1].freq = Notes::freqs[c.third()   - 12 * (c.inversion >= 4)];
	Synth::channels[2].freq = Notes::freqs[c.fifth()   - 12 * (c.inversion >= 3)];
	Synth::channels[3].freq = Notes::freqs[c.seventh() - 12 * (c.inversion >= 2)];
	Synth::channels[4].freq = Notes::freqs[c.ninth()   - 12 * (c.inversion >= 1)];
}

/* Toggle the given channel from being mixed. */
void toggleChannel(int channel) {
	SDL_LockAudio();
	Synth::channels[channel].on ^= true;
	SDL_UnlockAudio();
}

/* Toggle the given waveform(s) from the synthesizer. */
void toggleWaveform(int wave) {
	SDL_LockAudio();
	if (Synth::config.waveforms & wave)
		Synth::config.waveforms ^= wave;
	else
		Synth::config.waveforms |= wave;
	SDL_UnlockAudio();
}

/* Starts playing the measure from the beginning. */
std::atomic<int> lastBeat = 0;
std::atomic<float> measureStart = 0;
void resetMeasure() {
	measureStart = SDL_GetTicks() / 1000.0f;
	lastBeat = -1;
}

/* Reset the progression when commanded. */
std::atomic<int> beatsPerMeasure = 4;
std::atomic<bool> newProgression = false;
void updateProgression() {
	if (newProgression) {
		progression.clear();
		resetMeasure();
		Chord c = randomChord();
		progression.push_back(c);
		for (int i = 0; i < beatsPerMeasure - 1; i++) {
			c = nextChord(c);
			progression.push_back(c);
		}
		newProgression = false;
	}
}


/* Update to the next chord depending on the bpm. */
float bps = 45.0f / 60.0f;
void updateChord() {
	if (progression.size() == 0) return;

	const int currentBeat = (int) ((SDL_GetTicks() / 1000.0f - measureStart) * bps);
	if (currentBeat > lastBeat) {
		const auto & c = progression.at(currentBeat % progression.size());
		assignChord(c);
		Synth::resetNote();
		lastBeat = currentBeat;
	}
}

/* Update synth properties periodically. */
void updateSynth() {
	SDL_LockAudio();
	Synth::duty();
	Synth::harmonics();
	Synth::attackRelease();
	Synth::vibrato();
	SDL_UnlockAudio();
}

/* Control audio in separate thread. */
std::atomic<bool> audioRunning = true;
void control() {
	// seed random in thread 'cause Microsoft
	srand((unsigned int)time(NULL));
	while (audioRunning) {
		updateProgression();
		updateChord();
		updateSynth();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

/* Set some synth defaults. */
void setDefaults() {
	// turn off 7th and 9th to begin with
	toggleChannel(3);
	toggleChannel(4);
}

int main(int argc, char * argv[])
{
	srand((unsigned int)time(NULL)); 
	Notes::computeFreqs();
	computeTransitions();
	Synth::init();
	View::init();

	std::thread controller(control);

	setDefaults();

	View::intro();

	while (true) {

		std::string input;
		std::getline(std::cin, input);
		if (input.length() == 0) continue;

		std::istringstream iss(input);
		const std::vector<std::string> tokens{
			std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>{} };
		const std::string & cmd = tokens.at(0);
		if (cmd == "help") {
			View::help();
			continue;
		}
		else if (cmd == "new" || cmd == "n") {
			newProgression = true;
			resetMeasure();
		}
		/* beats n */
		else if (cmd == "beats") {
			if (tokens.size() != 2) {
				std::cerr << "Invalid number of parameters." << std::endl;
				continue;
			}
			try {
				int beats = std::stoi(tokens.at(1));
				if (beats < 1) {
					std::cerr << "Must have a positive number of beats." << std::endl;
					continue;
				}
				if (beats > 5) {
					std::cerr << "Note: beats limited to 5." << std::endl;
					beats = 5;
				}
				beatsPerMeasure = beats;
				newProgression = true;
				resetMeasure();
			}
			catch (std::exception &) {
				std::cerr << "Could not understand '" << tokens.at(1) << "'." << std::endl;
				continue;
			}
		}
		/* bpm n */
		else if (cmd == "bpm") {
			if (tokens.size() != 2) {
				std::cerr << "Invalid number of parameters." << std::endl;
				continue;
			}
			try {
				int bpm = std::stoi(tokens.at(1));
				bps = bpm / 60.0f;
				resetMeasure();
			}
			catch (std::exception &) {
				std::cerr << "Could not understand '" << tokens.at(1) << "'." << std::endl;
				continue;
			}
		}
		/* harm n 
		   harm mod rate (Hz)
		*/
		else if (cmd == "harm") {
			if (tokens.at(1) == "mod") {
				float harmv = std::stof(tokens.at(2));
				Synth::config.harmonicVelocity = harmv;
			}
			else {
				int harms = std::stoi(tokens.at(1));
				if (harms > 8) {
					std::cerr << "Note: harmonics limited to 8." << std::endl;
					harms = 8;
				}
				SDL_LockAudio();
				Synth::config.harmonics = harms;
				SDL_UnlockAudio();
			}
		}
		/* vibe depth (Hz) rate (Hz) */
		else if (cmd == "vibe") { // TODO: default
			if (tokens.size() != 3) {
				std::cerr << "Invalid number of parameters." << std::endl;
				continue;
			}
			const float depth = std::stof(tokens.at(1));
			const float rate  = std::stof(tokens.at(2));
			Synth::config.vibratoDepth = depth;
			Synth::config.vibratoRate = rate;
		}
		/* duty width 
		   duty mod rate (Hz)
		*/
		else if (cmd == "duty") {
			if (tokens.size() == 1) {
				std::cerr << "Invalid number of parameters." << std::endl;
				continue;
			}
			if (tokens.at(1) == "mod") {
				if (tokens.size() != 3) {
					std::cerr << "Invalid number of parameters." << std::endl;
					continue;
				}
				float rate = std::stof(tokens.at(2));
				Synth::config.dutyRate = rate;
			}
			else {
				if (tokens.size() != 2) {
					std::cerr << "Invalid number of parameters." << std::endl;
					continue;
				}
				try {
					float duty = std::stof(tokens.at(1));
					SDL_LockAudio();
					Synth::config.dutyOffset = duty;
					SDL_UnlockAudio();
				}
				catch (std::exception &) {
					std::cerr << "Could not understand '" << tokens.at(1) << "'." << std::endl;
					continue;
				}
			}
		}
		/* attack t (s) */
		else if (cmd == "attack") {
			if (tokens.size() != 2) {
				std::cerr << "Invalid number of parameters." << std::endl;
				continue;
			}
			const float a = std::stof(tokens.at(1));
			Synth::config.attack = a;
		}
		/* release t (s) */
		else if (cmd == "release") {
			if (tokens.size() != 2) {
				std::cerr << "Invalid number of parameters." << std::endl;
				continue;
			}
			const float r = std::stof(tokens.at(1));
			Synth::config.release = r;
		}
		/* Toggle waveforms */
		else if (cmd == "sin") {
			toggleWaveform(Synth::SINE);
		}
		else if (cmd == "sqr") {
			toggleWaveform(Synth::SQUARE);
		}
		else if (cmd == "saw") {
			toggleWaveform(Synth::SAWTOOTH);
		}
		else if (cmd == "tri") {
			toggleWaveform(Synth::TRIANGLE);
		}
		/* Toggle channels (chord degrees) */
		else if (cmd == "1") {
			toggleChannel(0);
		}
		else if (cmd == "3") {
			toggleChannel(1);
		}
		else if (cmd == "5") {
			toggleChannel(2);
		}
		else if (cmd == "7") {
			toggleChannel(3);
		}
		else if (cmd == "9") {
			toggleChannel(4);
		}
		/* Quit the application */
		else if (cmd == "exit" || cmd == "quit") {
			audioRunning = false;
			controller.join();
			break;
		}

		while (newProgression) SDL_Delay(16);
		View::render(progression, (int)(bps * 60.0f));
	}

	Synth::destroy();

	return 0;
}