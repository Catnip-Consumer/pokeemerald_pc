#include <iostream>
#include <fstream>
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
volatile bool agentWaitSync;

std::mutex agentMutex;
volatile size_t agentWaitingSync;

#ifdef _WIN32
#include <windows.h>

const std::filesystem::path getExecutableDir() {
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

#elif __linux__
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

#elif __APPLE__
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

		std::cout << "Read " << size << " bytes from " << savePath << std::endl;

	} catch (std::exception*) {
		// assume the file was not found
	}

	// close the file
	if (savefile.is_open()) {
		savefile.close();
	}
}

int main(int argc, char **argv) {
	auto threadHandle = getCurrentThreadHandle();
	setThreadAffinity(threadHandle, true);
	closeThreadHandle(threadHandle);

	ReadSaveFile();

	#ifdef _WIN32
		std::filesystem::create_directory(getExecutableDir() / "_dll");
	#endif

	agentStop = false;
	agentWaitSync = true;
	agentWaitingSync = 0;
	std::thread agent[CONCURRENT_AGENTS];

	for(int i = 0; i < CONCURRENT_AGENTS; i++) {
		#ifdef _WIN32
			// because Windows tries to load the same DLL multiple times, create copies of the DLL for each thread! yay!
			const auto target = getExecutableDir() / "_dll" / ("libemerald_" + std::to_string(i) + ".dll");

			if(std::filesystem::exists(target)) {
				std::filesystem::remove(target);
			}

			std::filesystem::copy_file(getExecutableDir() / "libemerald.dll", target);
		#endif
		agent[i] = std::thread(runAgent, 0, i);
	}

#ifdef ENABLE_SDL2
	initSDL();
#endif

	std::cout << "Waiting for agents to sync..." << std::endl;

	while(true) {
		#ifdef ENABLE_SDL2
			if(handleEventsSDL()) {
				break;
			}
		#endif

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		{
			std::lock_guard<std::mutex> lock(agentMutex);
			if(agentWaitingSync >= CONCURRENT_AGENTS) {
				break;
			}
		}
	}

	agentWaitSync = false;
	std::cout << "Running " << CONCURRENT_AGENTS << " agents..." << std::endl;

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

	for(int i = 0; i < CONCURRENT_AGENTS; i++) {
		agent[i].join();
	}
	return 0;
}
