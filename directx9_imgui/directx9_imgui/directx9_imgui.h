#pragma once
#include <d3d9.h>

#pragma comment(lib, "d3d9.lib")

// ---------------------------------------------------------------------------
// Configuration — change these to match your target application
// ---------------------------------------------------------------------------
#define HOOK_MENU_KEY     VK_INSERT   // Key to toggle the overlay menu
#define GAME_WINDOW_NAME  nullptr     // nullptr = auto-detect from D3D9 device
                                      // Set to e.g. "My Game" to target a specific window

// ---------------------------------------------------------------------------
// Hook typedefs
// ---------------------------------------------------------------------------
typedef HRESULT(APIENTRY* tEndScene)(LPDIRECT3DDEVICE9);
typedef HRESULT(APIENTRY* tReset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT(APIENTRY* tDrawIndexedPrimitive)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);

// ---------------------------------------------------------------------------
// Globals (defined in directx9_imgui.cpp)
// ---------------------------------------------------------------------------
extern tEndScene               oEndScene;
extern tReset                  oReset;
extern tDrawIndexedPrimitive   oDrawIndexedPrimitive;
extern WNDPROC                 oWndProc;
extern HWND                    g_hTargetWnd;
extern bool                    g_bMenuOpen;
extern bool                    g_bInitialized;

// ---------------------------------------------------------------------------
// Hook callbacks
// ---------------------------------------------------------------------------
HRESULT APIENTRY hkEndScene(LPDIRECT3DDEVICE9 pDevice);
HRESULT APIENTRY hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pParams);
HRESULT APIENTRY hkDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type,
                                        INT BaseVertexIndex, UINT MinVertexIndex,
                                        UINT NumVertices, UINT StartIndex, UINT PrimCount);

// ---------------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------------
DWORD WINAPI DirectXInit(LPVOID lpParameter);
void        HookShutdown();

// ImGui WndProc handler (defined in imgui_impl_win32.cpp)
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
