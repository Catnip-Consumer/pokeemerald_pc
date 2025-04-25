#include <iostream>
#include <cmath>
#include <ai/config.h>

#ifdef ENABLE_SDL2
#include <SDL2/SDL.h>
#include <ai/main.h>

extern "C" {
	#include <platform/framedraw.h>
}

#define WINDOW_SCALE_AI_GRID 10

SDLState sdlState;
SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture[CONCURRENT_AGENTS];

double FPSAccumulator = 0.0;
double fps = 0;
bool fpsUpdated = false;

double drawAccumulator = 0.0;
uint64_t lastGameTime = 0;
bool frameAdvance = false;

void initSDL() {
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	sdlWindow = SDL_CreateWindow(
		"emerald-ai",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		DISPLAY_WIDTH * WINDOW_SCALE_AI_GRID, DISPLAY_HEIGHT * WINDOW_SCALE_AI_GRID,
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
	SDL_RenderSetLogicalSize(sdlRenderer, (DISPLAY_WIDTH * GRID_COLS), (DISPLAY_HEIGHT * GRID_ROWS));

	for(int i = 0; i < CONCURRENT_AGENTS; i++) {
		sdlTexture[i] = SDL_CreateTexture(
			sdlRenderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING,
			DISPLAY_WIDTH, DISPLAY_HEIGHT
		);

		if (sdlTexture[i] == NULL){
			std::cout << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
			return;
		}
	}
}

void exitSDL() {
	SDL_DestroyWindow(sdlWindow);
	SDL_Quit();
}

bool handleEventsSDL(SDL_Event& event) {
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
		case KEY_##key:  sdlState.gos.userInput &= ~key; return false;

	#define HANDLE_KEYDOWN(key) \
		case KEY_##key:  sdlState.gos.userInput |= key; return false;

	#define HANDLE_SPEED_SET(key, speed) \
		case key: sdlState.gos.playbackSpeed = speed; return false;

	switch (event.type){
		case SDL_KEYUP:
			switch (event.key.keysym.sym) {
				HANDLE_SPEED_SET(SDLK_1, SDLPlaybackSpeed::REALTIME)
				HANDLE_SPEED_SET(SDLK_2, SDLPlaybackSpeed::FAST)
				HANDLE_SPEED_SET(SDLK_3, SDLPlaybackSpeed::SLIDESHOW)
				HANDLE_SPEED_SET(SDLK_4, SDLPlaybackSpeed::MAX)

				case SDLK_SPACE:
					frameAdvance = true;
					return false;

				case SDLK_PAUSE: // must also force-draw the next frame to look correct
					sdlState.gos.playbackSpeed = SDLPlaybackSpeed::PAUSED;
					frameAdvance = true;
					return false;

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
			return false;

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
			return false;

		case SDL_QUIT:
			return true;
		}
	return false;
}

void updateDeltaFPS(double deltaTime) {
	FPSAccumulator += deltaTime;

	// check if a second has elaped
	if(FPSAccumulator < 1.0) {
		return;
	}

	fpsUpdated = true;
	fps = 0;

	// Load the total number of FPS from all agents
	for(int j = 0; j < FPS_COUNTS; j++) {
		double indexFPS = 0;

		for (int i = 0; i < CONCURRENT_AGENTS; i++) {
			indexFPS += sdlState.sds.frameCounts[i][j];
		}

		fps += indexFPS / CONCURRENT_AGENTS;
	}

	fps /= FPS_COUNTS;

	// Reset accumulator and FPS counters
	FPSAccumulator = min(1.0, FPSAccumulator - 1);

	{
		std::lock_guard<std::mutex> lock(sdlState.mutexSDS);

		sdlState.gos.fpsIndex = (sdlState.gos.fpsIndex + 1) % FPS_COUNTS;

		for (int i = 0; i < CONCURRENT_AGENTS; i++) {
			sdlState.sds.frameCounts[i][sdlState.gos.fpsIndex] = 0;
		}
	}
}

/* aiframe should match this table when reading SDLPlaybackSpeed to check whether to update draw at all. */
constexpr double deltaForNextFrame[] = {
	[(size_t) SDLPlaybackSpeed::PAUSED] =		INFINITY,
	[(size_t) SDLPlaybackSpeed::REALTIME] =		1 / 60.0,
	[(size_t) SDLPlaybackSpeed::FAST] =			1 / 360.0,
	[(size_t) SDLPlaybackSpeed::SLIDESHOW] =	1 / 20.0,
	[(size_t) SDLPlaybackSpeed::MAX] =			1 * 4.0,
};

void updateDeltaTime(double deltaTime) {
	drawAccumulator += deltaTime;

	// check if deltatime has elapsed
	auto deltaNeeded = deltaForNextFrame[(size_t) sdlState.gos.playbackSpeed];

	if(frameAdvance) {
		// little hack to ensure when we stop being paused, accumulator is at 0
		deltaNeeded = drawAccumulator;
		frameAdvance = false;
	}

	if(drawAccumulator < deltaNeeded) {
		return;
	}

	drawAccumulator = min(deltaNeeded, drawAccumulator - deltaNeeded);

	// Draw the next frame
	drawSDL();
	sdlState.gos.currentFrame = sdlState.gos.currentFrame + 1;
}

void updateDeltas() {
	// getg the new delta time
	uint64_t curGameTime = SDL_GetPerformanceCounter();
	double deltaTime = ((curGameTime - lastGameTime) / (double)SDL_GetPerformanceFrequency());
	lastGameTime = curGameTime;

	// update components that use delta time
	updateDeltaFPS(deltaTime);
	updateDeltaTime(deltaTime);
}

bool updateSDL() {
	bool exit = false;
	SDL_Event event;

	while (!exit && SDL_PollEvent(&event)) {
		exit |= handleEventsSDL(event);
	}

	updateDeltas();
	return exit;
}

static void drawTextureOf(int16_t index) {
	SDL_UpdateTexture(sdlTexture[index], NULL, sdlState.aos.screens[index], DISPLAY_WIDTH * sizeof (Uint16));
}

static void drawTextures() {
	// If viewing an AI agent, only draw that agent's texture
	if(sdlState.gos.viewIndex >= 0) {
		drawTextureOf(sdlState.gos.viewIndex);
		return;
	}

	// Draw all textures
	for (int i = 0; i < CONCURRENT_AGENTS; i++) {
		drawTextureOf(i);
	}
}

void drawAiGrid() {
	// Draw each ai agent in a grid
	for(int i = 0; i < GRID_ROWS * GRID_COLS; i++) {
		const SDL_Rect rect = {
			(i % GRID_COLS) * DISPLAY_WIDTH,
			(i / GRID_COLS) * DISPLAY_HEIGHT,
			DISPLAY_WIDTH, DISPLAY_HEIGHT
		};
		SDL_RenderCopy(sdlRenderer, sdlTexture[i], NULL, &rect);
	}
}

void updateTitle() {
	// Title is only updated if the FPS has changed
	if(!fpsUpdated) {
		return;
	}

	fpsUpdated = false;

	// Update the title of the window with the current FPS
	std::string title = "emerald-ai ";

	if(fps < 1000) {
		title += std::to_string((size_t) round(fps)) + " fps";

	} else {
		title += std::to_string((size_t) round(fps / 1000)) + " FPM";
	}

	SDL_SetWindowTitle(sdlWindow, title.c_str());
}

void drawSDL() {
	drawTextures();
	SDL_RenderClear(sdlRenderer);
	drawAiGrid();
	updateTitle();

	/**
		if (videoScaleChanged) {
			SDL_SetWindowSize(sdlWindow, DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale);
			videoScaleChanged = false;
		}
	 */

	SDL_RenderPresent(sdlRenderer);
}

#endif
