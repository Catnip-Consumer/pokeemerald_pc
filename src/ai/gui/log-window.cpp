#include <iostream>

#include <ai/gui/gui.h>
#include <ai/main.h>
#include <ai/log.h>

const char logLevelText[][8] = {
	"[DEBUG]",
	"[INFO ]",
	"[WARN ]",
	"[ERROR]",
};

static std::string getTimecode(const LogEntry& entry) {
	char timecode[12];
	snprintf(
		timecode, sizeof(timecode), "%02zu:%02zu:%02zu:%02zu",
		entry.frame / 60 / 60 / 60 / 24,	// days
		(entry.frame / 60 / 60 / 60) % 24,	// hours
		(entry.frame / 60 / 60) % 60,		// minutes
		(entry.frame / 60) % 60				// seconds
	);
	return std::string(timecode);
}

bool Log::_internalLog(LogLevel level, size_t frame, std::string&& string) {
	this->logAdded = true;
	this->mutex.lock();

	// If there are too many log entries, remove the oldest ones
	while(this->history.size() >= INMEMORY_LOG_ENTRY_LIMIT) {
		this->history.pop_front();
	}

	// load and convert time to local time
	const time_t _time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	const auto tm = localtime(&_time);

	// add the full entry to the log
	this->history.push_back(LogEntry(string, level, frame, *tm));
	const auto& entry = this->history.back();
	this->mutex.unlock();

	// check if we should write to stderr
	if(level == LogLevel::Error) {
		std::cerr << string << std::endl;
	}

	// check if output stream was defined
	if(!this->logStream) {
		return true;
	}

	// create enough space to store the prefix in a string buffer
	const auto timecode = getTimecode(entry);
	static const size_t prefixSize = sizeof(logLevelText[0]) + 12 + timecode.length();
	char prefix[prefixSize];

	// write prefix text and timestamp to the string buffer
	snprintf(
		prefix, prefixSize, "[%02d:%02d:%02d]%s[%s] ",
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		logLevelText[(size_t)level], timecode.c_str()
	);

	// write to the log stream
	*this->logStream << prefix << string << "\n";
	return true;
}

void agentLogWindow(size_t agentId) {
	if(!ImGui::Begin(WDNAME(AGENT_LOG), nullptr)) {
		ImGui::End();
		return;
	}

	if(!ImGui::BeginTable("LogText", 4, ImGuiTableFlags_ScrollY|ImGuiTableFlags_BordersInnerV)) {
		ImGui::End();
		return;
	}

	// Measure maximum width of columns
	float timeWidth =  ImGui::CalcTextSize("00:00:00").x;
	float levelWidth = ImGui::CalcTextSize(logLevelText[0]).x;
	float frameWidth = ImGui::CalcTextSize("00:00:00:00").x;

	// Set up log table columns
	ImGui::TableSetupColumn("c0", ImGuiTableColumnFlags_WidthFixed|ImGuiTableColumnFlags_NoResize, timeWidth);
	ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthFixed|ImGuiTableColumnFlags_NoResize, levelWidth);
	ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthFixed|ImGuiTableColumnFlags_NoResize, frameWidth);
	ImGui::TableSetupColumn("c3", ImGuiTableColumnFlags_WidthStretch|ImGuiTableColumnFlags_NoResize);
	ImGui::TableSetupScrollFreeze(0, 1);

	// Set up log table header text
	ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("time");
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("type");
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("game time");
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("message");

	// Write all log messages
	agentData.data[agentId].log.mutex.lock();

	for(auto& entry : agentData.data[agentId].log.history) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%02d:%02d:%02d", entry.time.tm_hour, entry.time.tm_min, entry.time.tm_sec);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(logLevelText[(size_t)entry.level]);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(getTimecode(entry).c_str());
		ImGui::TableNextColumn();
		ImGui::TextWrapped("%s", entry.content.c_str());
	}

	agentData.data[agentId].log.mutex.unlock();

	// Scroll to the bottom of the log table if new log entries were added
	if(agentData.data[agentId].log.logAdded) {
		ImGui::SetScrollY(ImGui::GetScrollMaxY());
		agentData.data[agentId].log.logAdded = false;
	}

	ImGui::EndTable();
	ImGui::End();
}
