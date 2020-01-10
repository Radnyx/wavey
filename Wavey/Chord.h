#ifndef CHORD_H
#define CHORD_H
#include <string>

class Chord {
public:
	constexpr static int NUM_INVERSIONS = 5;
	int root;
	enum Type { SEV, MIN_SEV, MAJ_SEV, SUS4_SEV, NUM_TYPES } type;
	// 0 => 1 3 5 7 9
	// 1 => 1 9 3 5 7
	// 2 => 7 1 9 3 5
	// 3 => 5 7 1 9 3
	// 4 => 3 5 7 1 9
	int inversion = 0;

	int third() const;
	int fifth() const { return root + 7; }
	int seventh() const;
	int ninth() const { return root + 14; }
	std::string name() const;
	float centerOfGravity() const;
};

struct Transition {
	int dist;
	Chord::Type newType;
};

#endif // CHORD_H