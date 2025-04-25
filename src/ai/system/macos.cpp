#include <iostream>
#include <ai/config.h>
#include <ai/main.h>

#include <mach-o/dyld.h>
#include <limits.h>

const std::filesystem::path getExecutableDir() {
	char buffer[PATH_MAX];
	uint32_t size = sizeof(buffer);

	if (_NSGetExecutablePath(buffer, &size) == 0) {
		return std::filesystem::path(buffer).parent_path();
	}

	return "";
}

void* LoadEmeraldDLL() {
	// TODO: Implement
	return nullptr;
}

void UnloadEmeraldDLL(void* dll) {
	// TODO: Implement
}

void* GetProcAddress(void* dll, const char* procName) {
	return nullptr;	// TODO: Implement
}

bool setThreadAffinity(std::thread& t, bool core0) {
	return false;	// TODO: Implement
}

void* getCurrentThreadHandle() {
	return nullptr;	// TODO: Implement
}

bool closeThreadHandle(void* handle) {
	return false;	// TODO: Implement
}
