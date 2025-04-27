#include <iostream>
#include <cmath>

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <ai/main.h>

extern "C" {
	#include <platform/framedraw.h>
}

#define WINDOW_SCALE_AI_GRID 10

SDLState sdlState;
SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_GLContext sdlGL;
//SDL_Texture *sdlTexture[CONCURRENT_AGENTS];
GLuint glAgentTex[CONCURRENT_AGENTS];
ImGuiIO* io;

double FPSAccumulator = 0.0;
double fps = 0;

double drawAccumulator = 0.0;
uint64_t lastGameTime = 0;
bool frameAdvance = false;

static inline ImVec2 getGBAScreenSizeAtScale(float xscale, float yscale) {
	return ImVec2(DISPLAY_WIDTH * xscale, DISPLAY_HEIGHT * yscale);
}

static inline void signalFrameReady() {
	sdlState.gos.currentFrame = sdlState.gos.currentFrame + 1;

	{
		// Signal all threads SDL is ready now
		std::lock_guard<std::mutex> lock(sdlState.signal.mutex);
		sdlState.signal.cv.notify_all();
	}
}

bool initSDL() {
	// Initialize SDL with no audio
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		return false;
	}

	// Setup OpenGL flags
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

	// Create a new SDL window
	const auto requestedSize = getGBAScreenSizeAtScale(GRID_COLS, GRID_ROWS);
	sdlWindow = SDL_CreateWindow(
		"emerald-ai - starting...",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		requestedSize.x, requestedSize.y,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
	);

	if (sdlWindow == NULL) {
		std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return false;
	}

	// create an OpenGL context for this window
	sdlGL = SDL_GL_CreateContext(sdlWindow);
	if (sdlGL == nullptr) {
		std::cout << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_GL_MakeCurrent(sdlWindow, sdlGL);
	SDL_GL_SetSwapInterval(0); // no vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	io = &ImGui::GetIO(); (void)*io;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(sdlWindow, sdlGL);
	ImGui_ImplOpenGL3_Init("#version 130");

	// Macro that sets an OpenGL texture to use nearest neighbor scaling...
	#define TEX_NEAREST_SETUP												\
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	\
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	\
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);\
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create OpenGL textures for agents
	glGenTextures(CONCURRENT_AGENTS, glAgentTex);

	for(size_t i = 0;i < CONCURRENT_AGENTS;i ++) {
		glBindTexture(GL_TEXTURE_2D, glAgentTex[i]);
		TEX_NEAREST_SETUP

		// create blank texture data. Will be updated later
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGB5,
			DISPLAY_WIDTH, DISPLAY_HEIGHT,
			0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
			sdlState.aos.screens[i]
		);
	}

	return true; // Init success
}

void exitSDL() {
	signalFrameReady();		// wake up threads so they can exit safely.

	// Clean up OpenGL context
	glDeleteTextures(CONCURRENT_AGENTS, glAgentTex);

	// Cleanup ImGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

	// Cleanup SDL and OpenGL contexts
    SDL_GL_DeleteContext(sdlGL);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
}

static bool handleEventsSDL(SDL_Event& event) {
	ImGui_ImplSDL2_ProcessEvent(&event);

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

		case SDL_WINDOWEVENT:
			return event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(sdlWindow);
	}
	return false;
}

static inline void updateTitle() {
	// Update the title of the window with the current FPS
	std::string title = "emerald-ai - fps: "+ std::to_string((size_t) io->Framerate) +" - ai: ";

	if(fps < 1000) {
		title += std::to_string((size_t) round(fps)) + " per second";

	} else {
		title += std::to_string((size_t) round(fps / 1000)) + " per ms";
	}

	SDL_SetWindowTitle(sdlWindow, title.c_str());
}

static void drawSDL();

static void updateDeltaFPS(double deltaTime) {
	FPSAccumulator += deltaTime;

	// check if a second has elaped
	if(FPSAccumulator < 1.0) {
		return;
	}

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
	updateTitle();

	// Reset accumulator and FPS counters
	FPSAccumulator = min(1.0, FPSAccumulator - 1);

	sdlState.gos.fpsIndex = (sdlState.gos.fpsIndex + 1) % FPS_COUNTS;

	for (int i = 0; i < CONCURRENT_AGENTS; i++) {
		// WARNING: This technically can make us lose a few FPS if the later agents push their FPS updates to new FPS index.
		// I honestly dont care enough to fix it though. FPS anyway is just an approximation.
		sdlState.sds.frameCounts[i][sdlState.gos.fpsIndex] = 0;
	}
}

/* aiframe should match this table when reading SDLPlaybackSpeed to check whether to update draw at all. */
static constexpr double deltaForNextFrame[] = {
	[(size_t) SDLPlaybackSpeed::PAUSED] =		INFINITY,
	[(size_t) SDLPlaybackSpeed::REALTIME] =		1 / 60.0,
	[(size_t) SDLPlaybackSpeed::FAST] =			1 / 360.0,
	[(size_t) SDLPlaybackSpeed::SLIDESHOW] =	1 / 20.0,
	[(size_t) SDLPlaybackSpeed::MAX] =			1.0,
};

static void updateDeltaTime(double deltaTime) {
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
	signalFrameReady();
}

static void updateDeltas() {
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

static inline float aspectRatio() {
	return DISPLAY_WIDTH / (float) DISPLAY_HEIGHT;
}

static inline ImVec2 calculateWindowInnerSize() {
	auto area = ImGui::GetWindowSize();
	auto start = ImGui::GetCursorPos();
	area.x -= start.x * 2;
	area.y -= start.y;
	return area;
}

static constexpr ImVec2 displaySize = ImVec2(DISPLAY_WIDTH, DISPLAY_HEIGHT);

static inline void drawAiGrid() {
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(getGBAScreenSizeAtScale(GRID_COLS, GRID_ROWS));
	ImGui::Begin("AI view", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);

	// Draw each ai agent in a grid
	for(int y = 0; y < GRID_ROWS; y++) {
		for(int x = 0; x < GRID_COLS; x++) {
			ImGui::SetCursorPos(ImVec2(x * DISPLAY_WIDTH, y * DISPLAY_HEIGHT));
			ImGui::Image(
				(ImTextureID)(intptr_t) glAgentTex[x + (y * GRID_COLS)],
				displaySize
			);
		}
	}

	ImGui::End();
}

static inline void drawTextureOf(int16_t index) {
	glBindTexture(GL_TEXTURE_2D, glAgentTex[index]);
	glTexSubImage2D(
		GL_TEXTURE_2D, 0,
		0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
		GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
		sdlState.aos.screens[index]
	);
}

static inline void drawTextures() {
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

static constexpr ImVec4 windowbg = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

static void drawSDL() {
	drawTextures();

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	drawAiGrid();

	/**
		if (videoScaleChanged) {
			SDL_SetWindowSize(sdlWindow, DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale);
			videoScaleChanged = false;
		}
	 */

	// Render the frame
	ImGui::Render();
	glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
	glClearColor(windowbg.x, windowbg.y, windowbg.z, windowbg.w);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(sdlWindow);

	// update title bar content
	updateTitle();
}
