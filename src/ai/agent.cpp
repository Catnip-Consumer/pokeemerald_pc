#include <thread>
#include <iostream>
#include <cstring>
#include <chrono>

#include <ai/main.h>

extern "C" {
	#include <rtc.h>
	#include <main.h>
	#include <platform/dll.h>
	#include <platform/dma.h>
	#include <gba/flash_internal.h>
}

using namespace std::chrono_literals;
#define REG_BASE (eme.REG_BASE)

static thread_local struct EmeraldAddresses eme;

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
	std::memcpy(dest, flash + offset + (sectorNum << (*eme.gFlash)->sector.shift), size);
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
	return (1 << (rand() % 10)) & ~(START_BUTTON | SELECT_BUTTON);
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

#ifdef ENABLE_SDL2
thread_local static uint8_t lastFrame = -1;

/* Update the number of frames ran to SDL */
void updateFrameCount(int index, size_t count) {
	auto* fpsAddr = &(sdlState.sds.frameCounts[index][sdlState.gos.fpsIndex]);
	*fpsAddr = count + *fpsAddr;
}

/* aiframe should match this table when reading SDLPlaybackSpeed to check whether to update draw at all. */
constexpr bool isAiFrameUpdate[] = {
	[(size_t) SDLPlaybackSpeed::PAUSED] = false,
	[(size_t) SDLPlaybackSpeed::REALTIME] = false,
	[(size_t) SDLPlaybackSpeed::FAST] = false,
	[(size_t) SDLPlaybackSpeed::SLIDESHOW] = true,
	[(size_t) SDLPlaybackSpeed::MAX] = true,
};

size_t fpsNotUpdated = 0;
uint8_t missedFramesCount = 0;

bool checkDrawUpdate(int index, bool aiframe) {
	// Check if the current frame is ai update frame or any frame
	if(aiframe != isAiFrameUpdate[(size_t) sdlState.gos.playbackSpeed]) {
		return false;
	}

	if(!aiframe && missedFramesCount > 0) {
		if(sdlState.gos.currentFrame == lastFrame) {
			// We missed rendering a previous frame
			--missedFramesCount;
			fpsNotUpdated++;
			return false;
		}

		// SDL has requested yet another frame..... Just go render it
		missedFramesCount += sdlState.gos.currentFrame - lastFrame - 1;
		goto renderIt;
	}

	// Check if the current frame is the same as the last frame drawn
	if(sdlState.gos.currentFrame == lastFrame) {
		if(aiframe) {
			// if there isn't a frame available, but this is an ai frame, keep running
			fpsNotUpdated += AI_FRAMES_BEFORE_POLL;
			return false;
		}

		{
			// sleep until frame is received
			std::unique_lock<std::mutex> lock(sdlState.signal.mutex);
			sdlState.signal.cv.wait(lock, [] { return sdlState.gos.currentFrame != lastFrame; });

			// Calculate the number of frames we missed processing
			missedFramesCount = sdlState.gos.currentFrame - lastFrame - 1;
		}
	}

	renderIt:
	// We are here, so that means a new frame was available
	lastFrame = sdlState.gos.currentFrame;

	// update frame index
	updateFrameCount(index, fpsNotUpdated + 1);
	fpsNotUpdated = 0;

	// Render frame to screen buffer
	DrawFrame(sdlState.aos.screens[index], &eme);
	return true;
}

#else
bool checkDrawUpdate(int index, bool aiframe) {
	return false;
}
#endif

void runAgent(int generation, int index) {
	/* Set affinity to not run on core0. See main.cpp for more info. */
	auto threadHandle = getCurrentThreadHandle();
	setThreadAffinity(threadHandle, false);
	closeThreadHandle(threadHandle);

	// Load the emerald.dll for this agent
	auto dllHandle = LoadEmeraldDLL(&eme, index);
	if(!dllHandle) {
		return;
	}

	srand(time(NULL) + index + generation);

	/* Initialize the internal clock of this agent. This is used for GBA-compatibility purposes. */
	std::memset(&internalClock, 0, sizeof(internalClock));
	internalClock.status = SIIRTCINFO_24HOUR;
	UpdateInternalClock();

	/* Set platform functions and initialize the game. */
	eme.Platform_Set(&dll_platform);
	REG_VCOUNT = 161;
	eme.AgbInit();

	{
		/* Let main thread know this agent is done initializing */
		std::lock_guard<std::mutex> lock(agentMutex);
		agentData.agentsCounter = agentData.agentsCounter - 1;
	}

	/* Sleep while other agents are still trying to initialize */
	while(AgentState::WAIT_SYNC == agentState) {
		std::this_thread::sleep_for(1ms);
	}

	while(AgentState::RUNNING == agentState) {
		/* Run for number of frames before AI is polled for inputs */
		for(int i = 0; i < AI_FRAMES_BEFORE_POLL; i++) {
			eme.AgbRunFrame();
			VBlankIntrWait();
			checkDrawUpdate(index, false);

			if(AgentState::RUNNING != agentState) {
				goto exit;
			}
		}

		checkDrawUpdate(index, true);
	}

	/* Simulation completed, unload DLL and exit. */
	exit:
	UnloadEmeraldDLL(dllHandle);
	agentData.agentsCounter = agentData.agentsCounter - 1;
}
