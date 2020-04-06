#include <Windows.h>
#include "min_hook/include/MinHook.h"

DWORD WINAPI DirectXInit(__in  LPVOID lpParameter);

void c_imgui_halt(void);

void c_imgui_unhook(void)
{
	c_imgui_halt();
}

BOOL WINAPI DllMain(const HINSTANCE instance, const DWORD reason, const LPVOID reserved) {
	switch (reason) {
	case DLL_PROCESS_ATTACH: {
		const auto thread = CreateThread(nullptr, NULL, DirectXInit, instance, NULL, nullptr);
		if (thread != nullptr)
			CloseHandle(thread);
	} break;

	case DLL_PROCESS_DETACH: {
		if (!reserved)
			if (MH_Uninitialize() != MH_OK) { return 1; }
		c_imgui_unhook();
	} break;

	default: break;
	}

	return TRUE;
}