
#include <ai/gui/gui.h>
#include <ai/main.h>

#include <imgui_internal.h>

static const ImVec2 agrentScreenSizePlz = displaySize * 4;
static const ImVec2 agentInfoLayoutScale = ImVec2(0.5f, 0.66f);
static const ImVec2 agentInfoWindowSize = agrentScreenSizePlz / ImVec2(0.5f, 0.66f);

static inline void agentScreen(size_t agentId) {
	// This bollocks just hides the window tab bar
	static ImGuiWindowClass agentScreenClass;
	agentScreenClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoCloseButton;
	ImGui::SetNextWindowClass(&agentScreenClass);

	// Set window preferred size and remove padding to look all nice
	ImGui::SetNextWindowSize(displaySize * 4, ImGuiCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	// Create a node for agent screen
	if(ImGui::Begin(WDNAME(AGENT_DISPLAY), nullptr)) {
		// TODO: Add aspect ratio calculation to scale all agent views accordingly
		auto topLeft = ImVec2(0, 0);
		auto displayAreaSize = ImGui::GetContentRegionAvail();
		auto agentSize = displayAreaSize;

		ImGui::Image((ImTextureID)(intptr_t) glAgentTex[agentId], agentSize + topLeft);
	}

	ImGui::PopStyleVar(1);
	ImGui::End();
}

void makeBottomNode(size_t agentId) {
	if(ImGui::Begin(WDNAME("todo"), nullptr, ImGuiWindowFlags_None)) {
		ImGui::Text("Bottom Node");
	}

	ImGui::End();
}

void windowAiInfo(size_t agentId) {
	const char* windowName = ("AI " + std::to_string(agentId)).c_str();
	ImGui::SetNextWindowSize(agentInfoWindowSize, ImGuiCond_FirstUseEver);
	ImGui::Begin(windowName, &viewingAgents[agentId]);

	// Divide up the dockspace into 3 sections: 2 horizontally + additional below
	const auto dockspaceId = ImGui::GetID(windowName);

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None);
	ImGui::DockBuilderSetNodeSize(dockspaceId, agentInfoWindowSize);

	// Split the left region into two (vertical split)
	ImGuiID topNode, bottomNode;
	ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Up, agentInfoLayoutScale.y, &topNode, &bottomNode);

	// Split the dockspace into two regions (horizontal split)
	ImGuiID topLeftNode, topRightNode;
	ImGui::DockBuilderSplitNode(topNode, ImGuiDir_Right, agentInfoLayoutScale.x, &topRightNode, &topLeftNode);

	// Assign windows to the dock nodes
	ImGui::DockBuilderDockWindow(WDNAME(AGENT_DISPLAY), topLeftNode);
	ImGui::DockBuilderDockWindow(WDNAME("todo"), bottomNode);
	ImGui::DockBuilderDockWindow(WDNAME(AGENT_LOG), topRightNode);

	// Create the dockspace for agent info
	ImGui::DockBuilderFinish(dockspaceId);
	ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_NoUndocking);

	agentScreen(agentId);
	agentLogWindow(agentId);
	makeBottomNode(agentId);
	ImGui::End();
}
