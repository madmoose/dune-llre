#pragma once

const int MAX_TEXT = 1024;
const int MAX_ERROR = 256;
const int MAX_TITLE = 256;
const int MAX_EVENTS = 256;

struct Size {
	int w;
	int h;
};

inline
bool operator==(const Size &a, const Size &b) {
	return a.w == b.w && a.h == b.h;
}

inline
bool operator!=(const Size &a, const Size &b) {
	return !(a == b);
}

struct Pos {
	int x;
	int y;
};

inline
bool operator==(const Pos &a, const Pos &b) {
	return a.x == b.x && a.y == b.y;
}

inline
bool operator!=(const Pos &a, const Pos &b) {
	return !(a == b);
}

struct Display {
	Size size;
	int rate;
	float dpi;
};

const Size default_window_size = {2 * 320, 2 * 200};

constexpr int32_t DEFAULT_WINDOW_POS = 1 << 31;

struct Window {
	bool resizable;
	bool hidden;
	bool fullscreen;

	char const *title;
	Pos pos;
	Size size;
	bool moved;
	bool resized;

	char synced_title[MAX_TITLE];
	Pos synced_pos;
	Size synced_size;
	bool synced_resizable;
	bool synced_hidden;
	bool synced_fullscreen;
};

struct App {
	virtual void wait_for_vsync() = 0;
	virtual void update_palette(uint8_t palette[768]) = 0;
	virtual void update_screen(uint8_t screen[64000]) = 0;
};

int dune_main(App &app, int argc, char **argv);
