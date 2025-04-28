
#include <ai/gui/gui.h>
#include <ai/main.h>

void agentLogWindow(size_t agentId) {
	if(ImGui::Begin(WDNAME(AGENT_LOG), nullptr, ImGuiWindowFlags_None)) {
		ImGui::Text("WIP");
	}

	ImGui::End();
}
