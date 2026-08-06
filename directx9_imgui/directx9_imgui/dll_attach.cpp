#include <Windows.h>

// Defined in directx9_imgui.cpp
DWORD WINAPI DirectXInit(LPVOID lpParameter);
void         HookShutdown();

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        {
            HANDLE hThread = CreateThread(nullptr, 0, DirectXInit, hInstance, 0, nullptr);
            if (hThread)
                CloseHandle(hThread);
        }
        break;

    case DLL_PROCESS_DETACH:
        // lpReserved != NULL means the process is terminating —
        // skip cleanup to avoid deadlocks in the loader lock.
        if (!lpReserved)
            HookShutdown();
        break;
    }

    return TRUE;
}
