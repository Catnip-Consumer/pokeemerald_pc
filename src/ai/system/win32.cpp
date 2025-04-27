#include <iostream>
#include <windows.h>
#include <ai/main.h>

const std::filesystem::path getExecutableDir() {
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	return std::filesystem::path(buffer).parent_path();
}

void* LoadEmeraldDLL(struct EmeraldAddresses* eme, int index) {
	const auto _exedir = getExecutableDir();
	const auto _dllPath = (_exedir / ("_dll/libemerald_"+ std::to_string(index) + ".dll")).string();
	HMODULE dll = LoadLibrary(_dllPath.c_str());

	if (!dll) {
		std::cerr << "Failed to load " << _dllPath << std::endl;
		return nullptr;
	}

	if(!GetEmeraldDLLAddresses(eme, dll)) {
		FreeLibrary(dll);
		return nullptr;
	}

	return dll;
}

void UnloadEmeraldDLL(void* dll) {
	FreeLibrary((HMODULE) dll);
}

void* GetProcAddress(void* dll, const char* procName) {
	return (void*) GetProcAddress((HMODULE) dll, procName);
}

bool setThreadAffinity(void* handle, bool core0) {
	if (SetThreadAffinityMask((HANDLE) handle, core0 ? 1 : ~3) == 0) {
		std::cerr << "Failed to set thread affinity: " << GetLastError() << std::endl;
		return false;
	}

	return true;
}

void* getCurrentThreadHandle() {
	// Get the pseudo-handle for the current thread
	HANDLE pseudoHandle = GetCurrentThread();

    // Convert the pseudo-handle to a real handle
    HANDLE realHandle;
	if (!DuplicateHandle(
			GetCurrentProcess(), pseudoHandle, GetCurrentProcess(), &realHandle,
			0, FALSE, DUPLICATE_SAME_ACCESS
	)) {
		std::cerr << "Failed to duplicate thread handle. Error: " << GetLastError() << std::endl;
		return nullptr;
    }

	return (void*) realHandle;
}

bool closeThreadHandle(void* handle) {
	if (!CloseHandle((HANDLE) handle)) {
		std::cerr << "Failed to close thread handle. Error: " << GetLastError() << std::endl;
		return false;
	}

	return true;
}
