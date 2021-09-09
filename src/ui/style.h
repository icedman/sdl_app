#ifndef STYLE_H
#define STYLE_H

#include "renderer.h"
#include "theme.h"

#include <string>

struct view_style_t {
	std::string font;
	bool italic;
	bool bold;
	color_info_t fg;
	color_info_t bg;
	bool filled;
	int border;
	int border_radius;
};

void view_style_register(view_style_t style, std::string name);
view_style_t view_style_get(std::string name);

#endif // STYLE_H
