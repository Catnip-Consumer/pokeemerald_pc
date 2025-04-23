#ifndef GUARD_DLL_H
#define GUARD_DLL_H

#include "global.h"

struct DLL_Platform {
	void (*VBlankIntrWait)(void);
	void (*SoftReset)(u32);
	u16 (*GetKeyInput)(void);
	void (*StoreSaveFile)(void);
	void (*ReadFlash)(u16 sectorNum, u32 offset, u8 *dest, u32 size);
	void (*QueueAudio)(float *audioBuffer, s32 samplesPerFrame);
	void (*GetStatus)(struct SiiRtcInfo *rtc);
	void (*SetStatus)(struct SiiRtcInfo *rtc);
	void (*GetDateTime)(struct SiiRtcInfo *rtc);
	void (*SetDateTime)(struct SiiRtcInfo *rtc);
	void (*GetTime)(struct SiiRtcInfo *rtc);
	void (*SetTime)(struct SiiRtcInfo *rtc);
	void (*SetAlarm)(u8 *alarmData);
	bool8 HasAudio;
	bool8 SkipToGame;
};

_DLL_ void Platform_Set(const struct DLL_Platform *platform);

#endif
