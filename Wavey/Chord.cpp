#include "Chord.h"
#include <assert.h>
#include "Notes.h" 

/* Return the correct third (major or minor) given the chord. */
int Chord::third() const {
	switch (type) {
	case Chord::SEV: case Chord::MAJ_SEV: return root + 4;
	case Chord::MIN_SEV: return root + 3;
	case Chord::SUS4_SEV: return root + 5;
	}
	assert(false);
	return -1;
}

/* Return the correct seventh (major or flatted) given the chord. */
int Chord::seventh() const {
	switch (type) {
	case Chord::SEV: case Chord::MIN_SEV: case Chord::SUS4_SEV:
		return root + 10;
	case Chord::MAJ_SEV: return root + 11;
	}
	assert(false);
	return -1;
}

/* Return the proper name of the given chord. */
std::string Chord::name() const {
	std::string over;
	switch (inversion) {
	case 2: over = "/" + Notes::name(seventh()); break;
	case 3: over = "/" + Notes::name(fifth()); break;
	case 4: over = "/" + Notes::name(third()); break;
	}
	std::string type;
	switch (this->type) {
	case Chord::SEV: type = "7"; break;
	case Chord::SUS4_SEV: type = "sus4(7)"; break;
	case Chord::MIN_SEV: type = "m7"; break;
	case Chord::MAJ_SEV: type = "M7"; break;
	}
	return Notes::name(root) + type + over;
}

/* Compute the "average" note of all the notes in c. */
float Chord::centerOfGravity() const {
	const int sum = root
		+ (third() - 12 * (inversion >= 4))
		+ (fifth() - 12 * (inversion >= 3))
		+ (seventh() - 12 * (inversion >= 2))
		+ (ninth() - 12 * (inversion >= 1));
	return sum / 5.0f;
}