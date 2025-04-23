#include <thread>
#include <iostream>
#include <cstring>
#include <chrono>

#include <ai/config.h>
#include <ai/thread.h>

extern "C" {
	#include <rtc.h>
	#include <main.h>
	#include <platform/dll.h>
	#include <platform/dma.h>
	#include <gba/flash_internal.h>
}

using namespace std::chrono_literals;

extern void DrawFrame(uint16_t *pixels, struct EmeraldAddresses* eme);
static bool GetEmeraldDLLAddresses(void* dll);

#ifdef _WIN32
#include <windows.h>

static void* LoadEmeraldDLL(int index) {
	const auto _exedir = getExecutableDir();
	const auto _dllPath = (_exedir / ("_dll/libemerald_"+ std::to_string(index) + ".dll")).string();
    HMODULE dll = LoadLibrary(_dllPath.c_str());

    if (!dll) {
        std::cerr << "Failed to load " << _dllPath << std::endl;
        return nullptr;
    }

	if(!GetEmeraldDLLAddresses(dll)) {
		FreeLibrary(dll);
		return nullptr;
	}

	return dll;
}

static void UnloadEmeraldDLL(void* dll) {
    FreeLibrary((HMODULE) dll);
}

static void* GetProcAddress(void* dll, const char* procName) {
	return (void*) GetProcAddress((HMODULE) dll, procName);
}

#elif __linux__

static void* LoadEmeraldDLL(int index) {
	// TODO: Implement
	return nullptr;
}

static void UnloadEmeraldDLL(void* dll) {
	// TODO: Implement
}

static void* GetProcAddress(void* dll, const char* procName) {
	return nullptr;	// TODO: Implement
}

#elif __APPLE__

static void* LoadEmeraldDLL() {
	// TODO: Implement
	return nullptr;
}

static void UnloadEmeraldDLL(void* dll) {
	// TODO: Implement
}

static void* GetProcAddress(void* dll, const char* procName) {
	return nullptr;	// TODO: Implement
}

#endif

thread_local static struct EmeraldAddresses eme;

static bool GetEmeraldDLLAddresses(void* dll) {
#define GRAB_ADDRESS(member)											\
	eme.member = (typeof(eme.member))GetProcAddress(dll, #member);		\
	if (!eme.member) {													\
		std::cerr << "Failed to get address of " #member << std::endl;	\
		return false;													\
	}

	GRAB_ADDRESS(Platform_Set);
	GRAB_ADDRESS(RunDMAs);
	GRAB_ADDRESS(AgbInit);
	GRAB_ADDRESS(AgbRunFrame);

	GRAB_ADDRESS(gIntrTable);
	GRAB_ADDRESS(gFlash);
	GRAB_ADDRESS(REG_BASE);

	#ifdef ENABLE_SDL2
		GRAB_ADDRESS(VRAM_);
		GRAB_ADDRESS(PLTT);
		GRAB_ADDRESS(OAM);
	#endif
	return true;
}

// some hax because some of the following commands refer to REG_BASE directly!
#define REG_BASE (eme.REG_BASE)

void VBlankIntrWait() {
	REG_VCOUNT = 161;
	REG_DISPSTAT |= INTR_FLAG_VBLANK;
	eme.RunDMAs(DMA_HBLANK);

	if (REG_DISPSTAT & DISPSTAT_VBLANK_INTR) {
		eme.gIntrTable[4]();
	}
	REG_DISPSTAT &= ~INTR_FLAG_VBLANK;
}

thread_local struct SiiRtcInfo internalClock;

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

void Platform_ReadFlash(u16 sectorNum, u32 offset, u8 *dest, u32 size) {
	memcpy(dest, flash + offset + (sectorNum << (*eme.gFlash)->sector.shift), size);
}

void Platform_SetAlarm(u8 *alarmData) {

}

void Platform_StoreSaveFile(void) {

}

void Platform_QueueAudio(float *audioBuffer, s32 samplesPerFrame) {
	std::cerr << "Platform_QueueAudio()" << std::endl;
}

void SoftReset(u32 flags) {
	std::cerr << "SoftReset()" << std::endl;
}

extern volatile uint16_t keys;

u16 Platform_GetKeyInput(void){
	return 1 << (rand() % 10);
}

static const struct DLL_Platform dll_platform = {
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
	.SetAlarm = Platform_SetAlarm,
	.HasAudio = false,
	.SkipToGame = true,
};

thread_local static size_t lastFrame = -1;

void runAgent(int generation, int index) {
	auto dllHandle = LoadEmeraldDLL(index);
	if(!dllHandle) {
		return;
	}

	std::memset(&internalClock, 0, sizeof(internalClock));
	internalClock.status = SIIRTCINFO_24HOUR;
	UpdateInternalClock();
	eme.Platform_Set(&dll_platform);

	{
		std::lock_guard<std::mutex> lock(agentMutex);
		agentWaitingSync = agentWaitingSync + 1;
	}

	while(agentWaitSync) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	REG_VCOUNT = 161;
	eme.AgbInit();

	while(!agentStop) {
		// time this loop
		auto start = std::chrono::high_resolution_clock::now();

		// run for multiple frames before we check SDL for updates or ask for input updates
		while((std::chrono::high_resolution_clock::now() - start) < 7ms) {
			for(int i = 0; i < 8; i++) {
				eme.AgbRunFrame();
				VBlankIntrWait();
			}
		}

		#ifdef ENABLE_SDL2
		{
			std::lock_guard<std::mutex> lock(sdlMutex);

			// if frame changed from SDL, draw screens
			if(sdlAgentsFinishedDrawing >= 0 && sdlCurrentFrame != lastFrame) {
				lastFrame = sdlCurrentFrame;
				DrawFrame(screens[index], &eme);
				sdlAgentsFinishedDrawing = sdlAgentsFinishedDrawing + 1;
			}
		}
		#endif
	}

	UnloadEmeraldDLL(dllHandle);
}
