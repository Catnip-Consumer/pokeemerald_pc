#define ENABLE_SDL2

#ifdef ENABLE_SDL2
	#include <SDL2/SDL.h>
#endif

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <algorithm>

extern "C" {
	#include <rtc.h>
	#include <main.h>
	#include <platform/dma.h>
	#include <platform/dll.h>
	#include <gba/flash_internal.h>
}

#include <ai/sdl2.h>

struct SiiRtcInfo internalClock;
uint16_t keys = 0;

void Platform_GetStatus(struct SiiRtcInfo *rtc){
	rtc->status = internalClock.status;
}

void Platform_SetStatus(struct SiiRtcInfo *rtc){
	internalClock.status = rtc->status;
}

u8 BinToBcd(u8 bin) {
	int placeCounter = 1;
	u8 out = 0;
	do {
		out |= (bin % 10) * placeCounter;
		placeCounter *= 16;

	} while ((bin /= 10) > 0);

	return out;
}

static void UpdateInternalClock(void){
	time_t rawTime = time(NULL);
	struct tm *time = localtime(&rawTime);

	internalClock.year = BinToBcd(time->tm_year - 100);
	internalClock.month = BinToBcd(time->tm_mon + 1);
	internalClock.day = BinToBcd(time->tm_mday);
	internalClock.dayOfWeek = BinToBcd(time->tm_wday);
	internalClock.hour = BinToBcd(time->tm_hour);
	internalClock.minute = BinToBcd(time->tm_min);
	internalClock.second = BinToBcd(time->tm_sec);
}

void Platform_GetDateTime(struct SiiRtcInfo *rtc){
	UpdateInternalClock();

	rtc->year = internalClock.year;
	rtc->month = internalClock.month;
	rtc->day = internalClock.day;
	rtc->dayOfWeek = internalClock.dayOfWeek;
	rtc->hour = internalClock.hour;
	rtc->minute = internalClock.minute;
	rtc->second = internalClock.second;
}

void Platform_SetDateTime(struct SiiRtcInfo *rtc) {
	internalClock.month = rtc->month;
	internalClock.day = rtc->day;
	internalClock.dayOfWeek = rtc->dayOfWeek;
	internalClock.hour = rtc->hour;
	internalClock.minute = rtc->minute;
	internalClock.second = rtc->second;
}

void Platform_GetTime(struct SiiRtcInfo *rtc) {
    UpdateInternalClock();

	rtc->hour = internalClock.hour;
	rtc->minute = internalClock.minute;
	rtc->second = internalClock.second;
}

void Platform_SetTime(struct SiiRtcInfo *rtc) {
	internalClock.hour = rtc->hour;
	internalClock.minute = rtc->minute;
	internalClock.second = rtc->second;
}

void Platform_SetAlarm(u8 *alarmData) {

}

u16 Platform_GetKeyInput(void){
	return keys;
}

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

static const std::filesystem::path savefilePath = getExecutableDir() / "emerald-ai.sav";

void Platform_ReadFlash(u16 sectorNum, u32 offset, u8 *dest, u32 size) {
	std::ifstream savefile;

	try {
		savefile = std::ifstream(savefilePath, std::ios::binary);

		// read from file
		savefile.seekg((sectorNum << gFlash->sector.shift) + offset, std::ios::beg);
		savefile.read(reinterpret_cast<char*>(dest), size);

	} catch (std::exception*) {
		// assume the file was not found
	}

	// close the file
	if (savefile.is_open()) {
		savefile.close();
	}
}

static void ReadSaveFile() {
	// fill flash buffer with 0xFF and read contents
	memset(FLASH_BASE, 0xFF, sizeof(FLASH_BASE));
}

void Platform_StoreSaveFile(void) {

}

void Platform_QueueAudio(float *audioBuffer, s32 samplesPerFrame) {
	std::cerr << "Platform_QueueAudio()" << std::endl;
}

void SoftReset(u32 flags) {
	std::cerr << "SoftReset()" << std::endl;
}

void VBlankIntrWait() {
	REG_VCOUNT = 161;
	REG_DISPSTAT |= INTR_FLAG_VBLANK;
	RunDMAs(DMA_HBLANK);

	if (REG_DISPSTAT & DISPSTAT_VBLANK_INTR) {
		gIntrTable[4]();
	}
	REG_DISPSTAT &= ~INTR_FLAG_VBLANK;
}

static struct DLL_Platform dll_platform = {
	.HasAudio = false,
	.VBlankIntrWait = VBlankIntrWait,
	.SoftReset = SoftReset,
	.GetKeyInput = Platform_GetKeyInput,
	.StoreSaveFile = Platform_StoreSaveFile,
	.ReadFlash = Platform_ReadFlash,
	.QueueAudio = Platform_QueueAudio,
	.GetStatus = Platform_GetStatus,
	.SetStatus = Platform_SetStatus,
	.GetDateTime = Platform_GetDateTime,
	.SetDateTime = Platform_SetDateTime,
	.GetTime = Platform_GetTime,
	.SetTime = Platform_SetTime,
	.SetAlarm = Platform_SetAlarm
};

int main(int argc, char **argv) {
	std::memset(&internalClock, 0, sizeof(internalClock));
	internalClock.status = SIIRTCINFO_24HOUR;
	UpdateInternalClock();
	Platform_Set(&dll_platform);

#ifdef ENABLE_SDL2
	initSDL();
#endif

	ReadSaveFile();
	REG_VCOUNT = 161;
	AgbInit();

	int frame = 0;

	while(true) {
		AgbRunFrame();

		if((frame++) % 2048 == 0) {
			drawSDL();
		}
		VBlankIntrWait();

		if(handleEventsSDL()) {
			break;
		}
	}

	exitSDL();
	return 0;
}
