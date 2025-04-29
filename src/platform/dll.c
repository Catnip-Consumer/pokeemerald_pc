

#include "global.h"
#include "platform.h"
#include "rtc.h"
#include "gba/defines.h"
#include "gba/m4a_internal.h"
#include "cgb_audio.h"
#include "gba/flash_internal.h"
#include "platform/dma.h"
#include "platform/framedraw.h"

#include "platform/dll.h"

const struct DLL_Platform *gPlatform = NULL;
const struct DLL_Events *gDllEvents = NULL;

_DLL_ void Platform_EventSet(const struct DLL_Events *events) {
	gDllEvents = events;
}

_DLL_ void Platform_Set(const struct DLL_Platform *platform) {
	gPlatform = platform;
}

bool8 Platform_HasAudio(void)
{
	if(gPlatform != NULL) {
		return gPlatform->HasAudio;
	}

	return FALSE;
}

bool8 Platform_SkipToGame(void)
{
	if(gPlatform != NULL) {
		return gPlatform->SkipToGame;
	}

	return FALSE;
}

void Platform_StoreSaveFile(void)
{
	if(gPlatform != NULL) {
		gPlatform->StoreSaveFile();
	}
}

void Platform_ReadFlash(u16 sectorNum, u32 offset, u8 *dest, u32 size)
{
	if(gPlatform != NULL) {
		gPlatform->ReadFlash(sectorNum, offset, dest, size);
	}
}

void Platform_QueueAudio(float *audioBuffer, s32 samplesPerFrame)
{
	if(gPlatform != NULL) {
		gPlatform->QueueAudio(audioBuffer, samplesPerFrame);
	}
}

u16 Platform_GetKeyInput(void)
{
	if(gPlatform != NULL) {
		return gPlatform->GetKeyInput();
	}

	return 0;
}

void Platform_GetStatus(struct SiiRtcInfo *rtc)
{
	if(gPlatform != NULL) {
		gPlatform->GetStatus(rtc);
	}
}

void Platform_SetStatus(struct SiiRtcInfo *rtc)
{
	if(gPlatform != NULL) {
		gPlatform->SetStatus(rtc);
	}
}

void Platform_GetDateTime(struct SiiRtcInfo *rtc)
{
	if(gPlatform != NULL) {
		gPlatform->GetDateTime(rtc);
	}
}

void Platform_SetDateTime(struct SiiRtcInfo *rtc)
{
	if(gPlatform != NULL) {
		gPlatform->SetDateTime(rtc);
	}
}

void Platform_GetTime(struct SiiRtcInfo *rtc)
{
	if(gPlatform != NULL) {
		gPlatform->GetTime(rtc);
	}
}

void Platform_SetTime(struct SiiRtcInfo *rtc)
{
	if(gPlatform != NULL) {
		gPlatform->SetTime(rtc);
	}
}

void Platform_SetAlarm(u8 *alarmData)
{
	if(gPlatform != NULL) {
		gPlatform->SetAlarm(alarmData);
	}
}

void SoftReset(u32 resetFlags)
{
	if(gPlatform != NULL) {
		gPlatform->SoftReset(resetFlags);
	}

}

void VBlankIntrWait(void)
{
	if(gPlatform != NULL) {
		gPlatform->VBlankIntrWait();
	}
}
