#include "Notes.h"
#include <math.h>

namespace Notes {

	/* Equal temperment tuning of notes on a piano where
		0  = low Ab
		40 = middle C
		87 = high B */
	float freqs[88];
	void computeFreqs() {
		for (int i = 0; i < 88; i++) {
			freqs[i] = (float)(pow(1.05946309436, i - 49.0) * 440.0);
		}
	}

	/* Shift the given note to within the octave from middle C. */
	int middleOctave(int note) {
		return MIDDLE_C + (note + 8) % 12;
	}

	/* Chromatic scale degrees from C. */
	int chrom(int note) {
		return middleOctave(note) - MIDDLE_C;
	}

	/* Return the name of the given note. */
	const std::string names[]
		= { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
	const std::string & name(int note) {
		return names[chrom(note)];
	}
}