#include "Curses.hpp"

WINDOW* create_new_window(int height, int width, int starty, int startx) {
	WINDOW* new_window = newwin(height, width, starty, startx);
	keypad(new_window, true);
	return new_window;
}

int waddstr(WINDOW* window, const std::string& str) {
	return waddstr(window, str.c_str());
}