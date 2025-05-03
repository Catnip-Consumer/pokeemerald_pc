#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <ai/main.h>
#include <ai/gui/imgui-helper.hpp>

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
extern size_t setFocusOnAgentWindow ;
extern void windowAiInfo(size_t agentId);

/* Window names */
#define WDNAME(name) ((name "##") + std::to_string(agentId)).c_str()
#define AGENT_DISPLAY "GBA Screen"
#define AGENT_LOG "Log Viewer"
#define AGENT_PARTY "Party"

