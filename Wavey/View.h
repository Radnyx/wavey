#ifndef VIEW_H
#define VIEW_H
#include <vector>
#include "Chord.h"

namespace View {
	void init();
	void intro();
	void help();
	void render(const std::vector<Chord> & prog, int bpm);
}

#endif // VIEW_H