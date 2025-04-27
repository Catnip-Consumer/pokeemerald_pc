#include <iostream>
#include <ai/main.h>

bool GetEmeraldDLLAddresses(struct EmeraldAddresses* eme, void* dll) {
	#define GRAB_ADDRESS(member)											\
		eme->member = (typeof(eme->member))GetProcAddress(dll, #member);	\
		if (!eme->member) {													\
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
