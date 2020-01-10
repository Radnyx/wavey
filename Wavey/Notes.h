#ifndef NOTES_H
#define NOTES_H
#include <string>

namespace Notes {
	constexpr int MIDDLE_C = 40;
	extern float freqs[88];
	void computeFreqs();
	int middleOctave(int note);
	int chrom(int note);
	const std::string & name(int note);
}

#endif // NOTES_H