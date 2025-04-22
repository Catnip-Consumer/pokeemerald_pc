#include <iostream>
#include <SDL2/SDL.h>

extern "C" {
	#include <platform/framedraw.h>
}

#include <ai/sdl2.h>

// Dimensions of the GBA screen in pixels
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 160

#define GRID_ROWS 8
#define GRID_COLS 8
#define GRID_WIDTH  (DISPLAY_WIDTH * GRID_COLS)
#define GRID_HEIGHT (DISPLAY_HEIGHT * GRID_ROWS)

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;

uint32_t videoScale = 10;
bool videoScaleChanged = false;

void VDraw(SDL_Texture *texture){
    static uint16_t image[DISPLAY_WIDTH * DISPLAY_HEIGHT];

    memset(image, 0, sizeof(image));
    DrawFrame(image);
    SDL_UpdateTexture(texture, NULL, image, DISPLAY_WIDTH * sizeof (Uint16));
}

void initSDL() {
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	sdlWindow = SDL_CreateWindow(
		"emerald-ai",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	if (sdlWindow == NULL) {
		std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, 0);
	if (sdlRenderer == NULL){
		std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_RenderSetLogicalSize(sdlRenderer, GRID_WIDTH, GRID_HEIGHT);

	sdlTexture = SDL_CreateTexture(
		sdlRenderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING,
		DISPLAY_WIDTH, DISPLAY_HEIGHT
	);
	if (sdlTexture == NULL){
		std::cout << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

    VDraw(sdlTexture);
}

void exitSDL() {
	SDL_DestroyWindow(sdlWindow);
	SDL_Quit();
}

extern uint16_t keys;

// Key mappings
#define KEY_A_BUTTON      SDLK_z
#define KEY_B_BUTTON      SDLK_x
#define KEY_START_BUTTON  SDLK_RETURN
#define KEY_SELECT_BUTTON SDLK_BACKSLASH
#define KEY_L_BUTTON      SDLK_a
#define KEY_R_BUTTON      SDLK_s
#define KEY_DPAD_UP       SDLK_UP
#define KEY_DPAD_DOWN     SDLK_DOWN
#define KEY_DPAD_LEFT     SDLK_LEFT
#define KEY_DPAD_RIGHT    SDLK_RIGHT

#define HANDLE_KEYUP(key) \
	case KEY_##key:  keys &= ~key; break;

#define HANDLE_KEYDOWN(key) \
	case KEY_##key:  keys |= key; break;

bool handleEventsSDL() {
	bool exit = false;
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type){
			case SDL_QUIT:
				exit = true;
				break;

			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
					HANDLE_KEYUP(A_BUTTON)
					HANDLE_KEYUP(B_BUTTON)
					HANDLE_KEYUP(START_BUTTON)
					HANDLE_KEYUP(SELECT_BUTTON)
					HANDLE_KEYUP(L_BUTTON)
					HANDLE_KEYUP(R_BUTTON)
					HANDLE_KEYUP(DPAD_UP)
					HANDLE_KEYUP(DPAD_DOWN)
					HANDLE_KEYUP(DPAD_LEFT)
					HANDLE_KEYUP(DPAD_RIGHT)
				}
				break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					HANDLE_KEYDOWN(A_BUTTON)
					HANDLE_KEYDOWN(B_BUTTON)
					HANDLE_KEYDOWN(START_BUTTON)
					HANDLE_KEYDOWN(SELECT_BUTTON)
					HANDLE_KEYDOWN(L_BUTTON)
					HANDLE_KEYDOWN(R_BUTTON)
					HANDLE_KEYDOWN(DPAD_UP)
					HANDLE_KEYDOWN(DPAD_DOWN)
					HANDLE_KEYDOWN(DPAD_LEFT)
					HANDLE_KEYDOWN(DPAD_RIGHT)
				}
				break;

			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					unsigned int w = event.window.data1;
					unsigned int h = event.window.data2;
					videoScale = 0;

					if (w / DISPLAY_WIDTH > videoScale) {
						videoScale = w / DISPLAY_WIDTH;
					}

					if (h / DISPLAY_HEIGHT > videoScale) {
						videoScale = h / DISPLAY_HEIGHT;
					}

					if (videoScale < 1) {
						videoScale = 1;
					}

					videoScaleChanged = true;
				}
				break;
			}
	}

	return exit;
}

void drawSDL() {
	VDraw(sdlTexture);
	SDL_RenderClear(sdlRenderer);

	for(int i = 0; i < GRID_ROWS * GRID_COLS; i++) {
		const SDL_Rect rect = {
			(i % GRID_COLS) * DISPLAY_WIDTH,
			(i / GRID_COLS) * DISPLAY_HEIGHT,
			DISPLAY_WIDTH, DISPLAY_HEIGHT
		};
		SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &rect);
	}

	if (videoScaleChanged) {
		SDL_SetWindowSize(sdlWindow, DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale);
		videoScaleChanged = false;
	}

	SDL_RenderPresent(sdlRenderer);
}
