#include "View.h"
#include <iostream>
#include <sstream>
#include <string>
#include <assert.h>
#include "Synth.h"
#include "Notes.h"

/* To string with precision.
   Courtesy of https://stackoverflow.com/questions/16605967/. */
template <typename T>
std::string to_string_prec(const T a_value, const int n = 6)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return out.str();
}

namespace View {
	constexpr int WIDTH = 85, HEIGHT = 23;
	char grid[WIDTH][HEIGHT];

	/* Clear inside. */
	void clear() {
		for (int y = 1; y < HEIGHT - 1; y++) {
			for (int x = 1; x < WIDTH - 1; x++) {
				grid[x][y] = ' ';
			}
		}
	}

	void init() {
		// Draw border
		for (int y = 0; y < HEIGHT; y++) {
			grid[0][y] = '#';
			grid[WIDTH - 1][y] = '#';
		}
		for (int x = 0; x < WIDTH; x++) {
			grid[x][0] = '#';
			grid[x][HEIGHT - 1] = '#';
		}
		clear();
	}

	void ch(char c, int x, int y) {
		assert(x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT);
		grid[x][y] = c;
	}

	void button(bool set, int x, int y) {
		ch(set ? 'X' : ' ', x, y);
		ch('[', x - 1, y);
		ch(']', x + 1, y);
	}

	void write(const char * text, int x, int y) {
		for (int i = 0; *text != '\0'; i++, text++)
			ch(*text, x + i, y);
	}

	void vert(int x, int y, int length) {
		for (int i = 0; i < length; i++) {
			ch('|', x, y + i);
		}
	}

	void horiz(int x, int y, int length) {
		for (int i = 0; i < length; i++) {
			ch('-', x + i, y);
		}
	}
	void drawWaveforms(int x, int y) {
		button(Synth::config.waveforms & Synth::SINE, x, y);
		write("SIN", x + 3, y);
		button(Synth::config.waveforms & Synth::SQUARE, x, y + 2);
		write("SQR", x + 3, y + 2);
		button(Synth::config.waveforms & Synth::SAWTOOTH, x, y + 4);
		write("SAW", x + 3, y + 4);
		button(Synth::config.waveforms & Synth::TRIANGLE, x, y + 6);
		write("TRI", x + 3, y + 6);
	}

	void drawDegrees(int x, int y) {
		static const char * const names[] = { "root", "3rd", "5th", "7th", "9th" };
		for (int i = 0; i < 5; i++) {
			int yy = y + i * 2;
			button(Synth::channels[i].on, x, yy);
			write(names[i], x + 3, yy);
		}
	}

	void drawTimbres(int x, int y) {
		write("harmonics     = ", x, y);
		write(std::to_string(Synth::config.harmonics).c_str(), x + 16, y);
		write("harm mod (Hz) = ", x, y + 2);
		write(to_string_prec(Synth::config.harmonicVelocity.load(), 2).c_str(), 
			x + 16, y + 2);
		write("duty width    = ", x, y + 4);
		write(to_string_prec(Synth::config.dutyOffset.load(), 2).c_str(), x + 16, y + 4);
		write("duty mod (Hz) = ", x, y + 6);
		write(to_string_prec(Synth::config.dutyRate.load(), 2).c_str(), x + 16, y + 6);
	}

	// Map inversion number to order of notes played
	constexpr int INVERSIONS[5][5] = {
		{ 0, 1, 2, 3, 4 },
		{ 0, 4, 1, 2, 3 },
		{ 3, 0, 4, 1, 2 },
		{ 2, 3, 0, 4, 1 },
		{ 1, 2, 3, 0, 4 },
	};

	void drawChords(const std::vector<Chord> & prog, int x, int y) {
		for (unsigned i = 0; i < prog.size(); i++) {
			const auto & c = prog.at(i);
			int xx = x + i * 13;
			write(std::to_string(i + 1).c_str(), xx, y);
			write(c.name().c_str(), xx, y + 1);

			const std::string names[5] = {
				Notes::name(c.root),
				Notes::name(c.third()),
				Notes::name(c.fifth()),
				Notes::name(c.seventh()),
				Notes::name(c.ninth())
			};
			for (int i = 0; i < Chord::NUM_INVERSIONS; i++) {
				int channel = INVERSIONS[c.inversion][i];
				if (Synth::channels[channel].on)
					write(names[channel].c_str(), 
						xx, y + 7 - i);
			}
		}
	}

	void intro() {
		std::cout << "Chord Progression Synthesizer -- type 'help' for help" << std::endl;
	}

	void help() {
		std::cout
			<< "new/n        -- Generate a new progression (for fun, do this first)" << std::endl
			<< "beats    <n> -- Set beats per measure (1-5)" << std::endl
			<< "bpm      <n> -- Set beats per minute" << std::endl
			<< "harm     <n> -- Number of harmonics of frequency to mix (1-8)" << std::endl
			<< "harm mod <f> -- Modulate amplitude of harmonics at given frequency" << std::endl
			<< "duty     <w> -- Square wave width (0.0-1.0)" << std::endl
			<< "duty mod <f> -- Modulate square wave width at given frequency" << std::endl
			<< "attack   <t> -- Length of volume attack in seconds" << std::endl
			<< "release  <t> -- Length of volume release in seconds" << std::endl
			<< "vibe <d> <f> -- Vibrato at depth (in Hz) at given frequency" << std::endl
			<< "sin          -- Toggle sine wave" << std::endl
			<< "sqr          -- Toggle square wave" << std::endl
			<< "saw          -- Toggle sawtooth wave" << std::endl
			<< "tri          -- Toggle triangle wave" << std::endl
			<< "1            -- Toggle root of chord" << std::endl
			<< "3            -- Toggle third of chord" << std::endl
			<< "5            -- Toggle fifth of chord" << std::endl
			<< "7            -- Toggle seventh of chord" << std::endl
			<< "9            -- Toggle ninth of chord" << std::endl
			<< "exit/quit    -- Quit this program" << std::endl;
	}

	void render(const std::vector<Chord> & prog, int bpm) {
		clear();

		drawWaveforms(3, 2);
		horiz(1, 10, 10);
		drawDegrees(3, 12);
		vert(11, 1, HEIGHT - 2);
		write("beats/min   = ", 13, 2);
		write(std::to_string(bpm).c_str(), 27, 2);
		write("attack  (s) = ", 13, 4);
		write(to_string_prec(Synth::config.attack.load(), 2).c_str(), 27, 4);
		write("release (s) = ", 13, 6);
		write(to_string_prec(Synth::config.release.load(), 2).c_str(), 27, 6);
		vert(32, 1, 9);
		horiz(12, 10, 72);
		drawTimbres(35, 2);
		drawChords(prog, 14, 12);
		vert(57, 1, 9);
		write("vibe depth (Hz) = ", 59, 2);
		write(to_string_prec(Synth::config.vibratoDepth.load(), 2).c_str(), 78, 2);
		write("vibe mod (Hz)   = ", 59, 4);
		write(to_string_prec(Synth::config.vibratoRate.load(), 2).c_str(), 78, 4);

		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; x++) {
				std::cout << grid[x][y];
			}
			std::cout << std::endl;
		}
	}
}