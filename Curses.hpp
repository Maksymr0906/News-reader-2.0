#pragma once

#include <curses.h>
#include <string>

class Curses {
public:
	Curses() {
		initscr();
		noecho();
	}

	~Curses() {
		endwin();
	}
};

WINDOW* create_new_window(int height, int width, int starty, int startx);
int waddstr(WINDOW* window, const std::string& str);