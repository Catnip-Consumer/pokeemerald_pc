#include <iostream>
#include <ai/config.h>
#include <ai/main.h>

#include <unistd.h>
#include <limits.h>

const std::filesystem::path getExecutableDir() {
	char buffer[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

	if (len != -1) {
		buffer[len] = '\0';
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
}§

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
