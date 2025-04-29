#pragma once

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <ai/main.h>

/* ImVec2 has no native operator+. Add for convenience */
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

/* ImVec2 has no native operator+. Add for convenience */
static inline ImVec2 operator+(const ImVec2& lhs, float rhs) {
	return ImVec2(lhs.x + rhs, lhs.y + rhs);
}

/* ImVec2 has no native operator-. Add for convenience */
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

/* ImVec2 has no native operator-. Add for convenience */
static inline ImVec2 operator-(const ImVec2& lhs, float rhs) {
	return ImVec2(lhs.x - rhs, lhs.y - rhs);
}

/* ImVec2 has no native operator*. Add for convenience */
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y);
}

/* ImVec2 has no native operator-. Add for convenience */
static inline ImVec2 operator*(const ImVec2& lhs, float rhs) {
	return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

/* ImVec2 has no native operator/. Add for convenience */
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y);
}

/* ImVec2 has no native operator/. Add for convenience */
static inline ImVec2 operator/(const ImVec2& lhs, float rhs) {
	return ImVec2(lhs.x / rhs, lhs.y / rhs);
}

/* GBA display size as ImVec2 */
static constexpr ImVec2 displaySize = ImVec2(DISPLAY_WIDTH, DISPLAY_HEIGHT);

/* SDL and ImGui variables exposed to all windows */
extern GLuint glAgentTex[CONCURRENT_AGENTS];
extern ImGuiIO* io;

/* GUI rendering functions and vars */
extern void windowAllAi();
extern void agentLogWindow(size_t agentId);
extern void agentPartyWindow(size_t agentId);

extern bool viewingAgents[CONCURRENT_AGENTS];
extern void windowAiInfo(size_t agentId);

/* Window names */
#define WDNAME(name) ((name "##") + std::to_string(agentId)).c_str()
#define AGENT_DISPLAY "GBA Screen"
#define AGENT_LOG "Log Viewer"
#define AGENT_PARTY "Party"

