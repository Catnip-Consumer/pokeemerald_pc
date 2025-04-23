#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <thread>

#include <ai/config.h>

#ifdef ENABLE_SDL2
	#include <SDL2/SDL.h>
#endif

#include <ai/sdl2.h>
#include <ai/thread.h>

uint8_t flash[sizeof(FLASH_BASE)];
volatile uint16_t keys = 0;
volatile bool agentStop;

#ifdef _WIN32
#include <windows.h>

static const std::filesystem::path getExecutableDir() {
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

#elif __linux__
#include <unistd.h>
#include <limits.h>

static const std::filesystem::path getExecutableDir() {
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
    return "";
}

#elif __APPLE__
#include <mach-o/dyld.h>
#include <limits.h>

static const std::filesystem::path getExecutableDir() {
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        return std::filesystem::path(buffer).parent_path();
    }
    return "";
}
#endif

static void ReadSaveFile() {
	// fill flash buffer with 0xFF and read contents
	memset(flash, 0xFF, sizeof(flash));
	std::ifstream savefile;

	try {
		const auto savePath = getExecutableDir() / "emerald-ai.sav";
		savefile = std::ifstream(savePath, std::ios::binary);

		// get file size
		savefile.seekg(0, std::ios::end);
		const auto size = savefile.tellg();
		savefile.seekg(0, std::ios::beg);

		// read from file
		const auto readSize = min((std::streampos) size, (std::streampos) sizeof(flash));
		savefile.read(reinterpret_cast<char*>(flash), size);

	} catch (std::exception*) {
		// assume the file was not found
	}

	// close the file
	if (savefile.is_open()) {
		savefile.close();
	}
}

int main(int argc, char **argv) {
	ReadSaveFile();

	agentStop = false;
	std::thread agent(runAgent, 0, 0);

#ifdef ENABLE_SDL2
	initSDL();
#endif

	while(true) {
	#ifdef ENABLE_SDL2
		if(handleEventsSDL()) {
			break;
		}
	#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	agentStop = true;
	#ifdef ENABLE_SDL2
		exitSDL();
	#endif
	agent.join();
	return 0;
}
