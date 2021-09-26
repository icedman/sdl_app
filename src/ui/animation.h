#ifndef ANIMATION_H
#define ANIMATION_H

#include "view.h"
#include <vector>

struct animation {
	int delay;
	int length;
	int index;

	virtual void update(int tick);
	// void start();
};

typedef std::shared_ptr<animation> animation_ptr;

struct animator {
	
};

#endif // ANIMATION_H