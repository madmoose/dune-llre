#include "SDL.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "platform.h"

struct SDLWindow : public Window {
	SDL_Window *sdl;
};

static bool quit;
static int num_updates;
static const char *platform;
static Display display;
static SDLWindow window = {

};

static SDL_Renderer *sdl_renderer;
static uint8_t *sdl_palette;
static uint8_t *sdl_screen_buffer;
static SDL_Texture *sdl_screen_texture;
static std::atomic_uint32_t vsync_counter;

static
void sdl_error(const char *name) {
	const char *error = SDL_GetError();
	printf("%s: %s\n", name, error);
	exit(1);
}

static
bool init_display()
{
	float dpi;
	if (SDL_GetDisplayDPI(0, &dpi, NULL, NULL) != 0) {
		sdl_error("SDL_GetDisplayDPI");
		return false;
	}
	display.dpi = dpi;

	SDL_DisplayMode mode;
	if (SDL_GetCurrentDisplayMode(0, &mode) != 0) {
		sdl_error("SDL_GetCurrentDisplayMode");
		return false;
	}
	display.size.w = mode.w;
	display.size.h = mode.h;
	display.rate = mode.refresh_rate;
	return true;
}

static void update_window();

static
bool init_window()
{
	if (!window.title) {
		window.title = "Dune - The Sleeper REawakens";
	}
	auto x = window.pos.x == DEFAULT_WINDOW_POS ? SDL_WINDOWPOS_CENTERED : window.pos.x;
	auto y = window.pos.y == DEFAULT_WINDOW_POS ? SDL_WINDOWPOS_CENTERED : window.pos.y;

	auto width = window.size.w == 0 ? default_window_size.w : window.size.w;
	auto height = window.size.h == 0 ? default_window_size.h : window.size.h;
	uint32_t flags = 0;
	if (window.resizable) {
		flags |= SDL_WINDOW_RESIZABLE;
	}
	if (window.hidden) {
		flags |= SDL_WINDOW_HIDDEN;
	}
	auto sdl_window = SDL_CreateWindow(window.title, x, y, width, height, flags);
	if (!sdl_window) {
		return false;
	}
	window.sdl = sdl_window;
	window.synced_pos = window.pos;
	strncpy(window.synced_title, window.title, sizeof(window.synced_title) - 1);
	update_window();

	sdl_renderer = SDL_CreateRenderer(window.sdl, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	sdl_palette = new uint8_t[768];
	memset(sdl_palette, 0, 768);

	sdl_screen_buffer = new uint8_t[64000];
	memset(sdl_screen_buffer, 0, 64000);

	sdl_screen_texture =
		SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 320, 200);
	printf("sdl_screen_texture: %p\n", sdl_screen_texture);

	uint32_t format;
	int access;
	int w, h;
	SDL_QueryTexture(sdl_screen_texture, &format, &access, &w, &h);
	printf("format: %x, access %x, %dx%d\n", format, access, w, h);

	return true;
}

static
void update_window()
{
	if (window.fullscreen != window.synced_fullscreen) {
		if (SDL_SetWindowFullscreen(window.sdl, window.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) < 0) {
			sdl_error("SDL_SetWindowFullscreen");
		}
		window.synced_fullscreen = window.fullscreen;
	}

	if (window.title != window.synced_title && strcmp(window.title, window.synced_title) != 0) {
		SDL_SetWindowTitle(window.sdl, window.title);
		strncpy(window.synced_title, window.title, sizeof(window.synced_title));
		window.title = window.synced_title;
	}

	if (window.pos != window.synced_pos) {
		SDL_SetWindowPosition(window.sdl, window.pos.x, window.pos.y);
	}
	SDL_GetWindowPosition(window.sdl, &window.pos.x, &window.pos.y);
	window.moved = num_updates == 0 || window.pos != window.synced_pos;
	window.synced_pos = window.pos;

	if (window.size != window.synced_size) {
		SDL_SetWindowSize(window.sdl, window.size.w, window.size.h);
	}
	SDL_GetWindowSize(window.sdl, &window.size.w, &window.size.h);
	window.resized = num_updates == 0 || window.size != window.synced_size;
	window.synced_size = window.size;

	if (window.resizable != window.synced_resizable) {
		SDL_SetWindowResizable(window.sdl, (SDL_bool)window.resizable);
	}
	window.synced_resizable = window.resizable;

	if (window.hidden != window.synced_hidden) {
		if (window.hidden) {
			SDL_HideWindow(window.sdl);
		} else {
			SDL_ShowWindow(window.sdl);
		}
	}
	window.synced_hidden = window.hidden;
}

static
void update_events()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			quit = true;
		}
	}
}

static
bool update()
{
	SDL_PumpEvents();
	update_events();
	update_window();
	num_updates++;
	return !quit;
}

static
void update_sdl_screen_texture()
{
	void *pixels;
	int pitch;

	SDL_LockTexture(sdl_screen_texture, nullptr, &pixels, &pitch);
	for (int y = 0; y != 200; ++y) {
		for (int x = 0; x != 320; ++x) {
			int c = sdl_screen_buffer[320 * y + x];
			((uint8_t *)pixels)[y * pitch + 4 * x + 0] = (sdl_palette[3 * c + 0] * 255 + 31) / 63;
			((uint8_t *)pixels)[y * pitch + 4 * x + 1] = (sdl_palette[3 * c + 1] * 255 + 31) / 63;
			((uint8_t *)pixels)[y * pitch + 4 * x + 2] = (sdl_palette[3 * c + 2] * 255 + 31) / 63;
			((uint8_t *)pixels)[y * pitch + 4 * x + 3] = 0;
		}
	}
	SDL_UnlockTexture(sdl_screen_texture);

	SDL_Rect srcrect = {0, 0, 320, 200};
	SDL_Rect dstrect = {0, 0, window.synced_size.w, window.synced_size.h};

	SDL_SetRenderDrawColor(sdl_renderer, 255, 0, 0, 255);
	SDL_RenderClear(sdl_renderer);
	SDL_RenderCopy(sdl_renderer, sdl_screen_texture, &srcrect, &dstrect);
	SDL_RenderPresent(sdl_renderer);
}

static
bool init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		sdl_error("SDL_Init");
		return false;
	}
	if (!init_display()) {
		return false;
	}
	if (!init_window()) {
		return false;
	}
	platform = SDL_GetPlatform();
	return true;
}

struct SDLApp : App {
	void wait_for_vsync();
	void update_palette(uint8_t palette[768]);
	void update_screen(uint8_t screen[64000]);
};

void SDLApp::wait_for_vsync()
{
	uint32_t old = vsync_counter.load();
	vsync_counter.wait(old);
}

void SDLApp::update_palette(uint8_t palette[768])
{
	memcpy(sdl_palette, palette, 768);
}

void SDLApp::update_screen(uint8_t buffer[64000])
{
	memcpy(sdl_screen_buffer, buffer, 64000);
}

int main(int argc, char **argv)
{
	window.pos.x = DEFAULT_WINDOW_POS;
	window.pos.y = DEFAULT_WINDOW_POS;

	init();

	App *app = new SDLApp;

	auto game_thread = new std::thread([=] {
		dune_main(*app, argc, argv);
	});

	auto frame_time = std::chrono::nanoseconds(int(1000000000 / 70));
	auto frame_start = std::chrono::steady_clock::now();
	while (update()) {
		// TODO: This limits us to 60fps for now
		update_sdl_screen_texture();

		++vsync_counter;
		vsync_counter.notify_one();

		const auto frame_end = frame_start + frame_time;
		frame_start = frame_end;
		std::this_thread::sleep_until(frame_end);
	}
}
