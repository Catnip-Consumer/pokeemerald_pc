#include <cassert>
#include <cmath>
#include <iostream>

#include <ai/gui/gui.h>
#include <ai/main.h>

void windowAllAi() {
	const auto mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(mainViewport->ID);
	ImGui::SetNextWindowPos(mainViewport->Pos);
	ImGui::SetNextWindowSize(mainViewport->Size);
	ImGui::Begin("AI Grid", nullptr, ImGuiWindowFlags_NoDecoration);

	// TODO: Add aspect ratio calculation to scale all agent views accordingly
	auto topLeft = ImVec2(0, 0);
	auto displayAreaSize = io->DisplaySize;
	auto agentSize = displayAreaSize / ImVec2(GRID_COLS, GRID_ROWS);
	auto botRight = displayAreaSize;
	size_t agentHovered = -1;

	// Check if mouse is hovering over the grid
	const auto windowPos = ImGui::GetWindowPos();

	if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(topLeft + windowPos, botRight + windowPos)) {
		// calculate mouse hover position
		auto hover = (ImGui::GetMousePos() - topLeft - windowPos) / agentSize;

		assert(hover.x >= 0 && hover.x < GRID_COLS);
		assert(hover.y >= 0 && hover.y < GRID_ROWS);

		// calculate the hovered agent index
		agentHovered = (size_t) (std::floor(hover.x) + (std::floor(hover.y) * GRID_COLS));
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	// Draw each ai agent in a grid
	for(size_t y = 0; y < GRID_ROWS; y++) {
		for(size_t x = 0; x < GRID_COLS; x++) {
			const size_t agentId = x + (y * GRID_COLS);
			const auto agentPos = (ImVec2(x, y) * agentSize) + topLeft;

			// draw agent texture
			ImGui::SetCursorPos(agentPos);
			const auto screenPos = ImGui::GetCursorScreenPos();
			ImGui::Image((ImTextureID)(intptr_t) glAgentTex[agentId], agentSize);

			if(agentHovered != agentId) {
				continue;;
			}

			// when an agent is hovered, highlight it
			static const auto thickness = 2.f;
			ImGui::GetWindowDrawList()->AddRect(
				screenPos + thickness, screenPos + agentSize - thickness,
				ImColor(0xde, 0x3c, 0x6a, 0xE0),
				0.0f, 0, thickness * 2
			);

			// check if user clicked on the agent
			if(ImGui::IsMouseClicked(0)) {
				// toggle agent info window
				viewingAgents[agentId] = !viewingAgents[agentId];
			}
		}
	}

	ImGui::End();
}
