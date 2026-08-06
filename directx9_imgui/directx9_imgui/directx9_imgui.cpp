#include "directx9_imgui.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "min_hook/include/MinHook.h"

// ---------------------------------------------------------------------------
// Global state (declared extern in header)
// ---------------------------------------------------------------------------
tEndScene             oEndScene             = nullptr;
tReset                oReset                = nullptr;
tDrawIndexedPrimitive oDrawIndexedPrimitive = nullptr;
WNDPROC               oWndProc              = nullptr;
HWND                  g_hTargetWnd          = nullptr;
bool                  g_bMenuOpen           = false;
bool                  g_bInitialized        = false;

// ---------------------------------------------------------------------------
// WndProc — only consume input when the menu is open
// ---------------------------------------------------------------------------
static LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Toggle menu on key-up so it doesn't repeat
    if (uMsg == WM_KEYUP && wParam == HOOK_MENU_KEY)
        g_bMenuOpen = !g_bMenuOpen;

    // When menu is visible, let ImGui handle mouse/keyboard
    if (g_bMenuOpen && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return TRUE;

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Helper: retrieve the HWND that owns the D3D9 swap-chain
// ---------------------------------------------------------------------------
static HWND GetDeviceWindow(LPDIRECT3DDEVICE9 pDevice)
{
    if (g_hTargetWnd)
        return g_hTargetWnd;

    // If the user set a specific window name, use it
    if (GAME_WINDOW_NAME)
    {
        g_hTargetWnd = FindWindowA(nullptr, GAME_WINDOW_NAME);
        if (g_hTargetWnd)
            return g_hTargetWnd;
    }

    // Auto-detect from the device's creation parameters
    D3DDEVICE_CREATION_PARAMETERS cp = {};
    if (SUCCEEDED(pDevice->GetCreationParameters(&cp)))
        g_hTargetWnd = cp.hFocusWindow;

    // Fallback: walk the swap-chain
    if (!g_hTargetWnd)
    {
        IDirect3DSwapChain9* pSwap = nullptr;
        if (SUCCEEDED(pDevice->GetSwapChain(0, &pSwap)) && pSwap)
        {
            D3DPRESENT_PARAMETERS pp = {};
            pSwap->GetPresentParameters(&pp);
            g_hTargetWnd = pp.hDeviceWindow;
            pSwap->Release();
        }
    }

    return g_hTargetWnd;
}

// ---------------------------------------------------------------------------
// DirectXInit — runs on its own thread from DllMain
// ---------------------------------------------------------------------------
DWORD WINAPI DirectXInit(LPVOID lpParameter)
{
    // Wait for the game to load d3d9.dll
    while (!GetModuleHandleA("d3d9.dll"))
        Sleep(100);

    // Create a throwaway D3D9 device to grab the vtable
    HMODULE hSelf = static_cast<HMODULE>(lpParameter);

    HWND tmpWnd = CreateWindowA("BUTTON", "DX9Hook", WS_SYSMENU | WS_MINIMIZEBOX,
                                CW_USEDEFAULT, CW_USEDEFAULT, 300, 300,
                                nullptr, nullptr, hSelf, nullptr);
    if (!tmpWnd)
        return 0;

    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D)
    {
        DestroyWindow(tmpWnd);
        return 0;
    }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow    = tmpWnd;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    LPDIRECT3DDEVICE9 pDevice = nullptr;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmpWnd,
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                    &d3dpp, &pDevice);
    if (FAILED(hr))
    {
        pD3D->Release();
        DestroyWindow(tmpWnd);
        return 0;
    }

    // Read vtable pointers (works for both x86 and x64)
    void** vtable = *reinterpret_cast<void***>(pDevice);

    // D3D9 vtable indices: Reset = 16, EndScene = 42, DrawIndexedPrimitive = 82
    void* pEndScene  = vtable[42];
    void* pReset     = vtable[16];
    void* pDIP       = vtable[82];

    // Clean up the throwaway device before hooking
    pDevice->Release();
    pD3D->Release();
    DestroyWindow(tmpWnd);

    // Install hooks via MinHook
    if (MH_Initialize() != MH_OK)
        return 0;

    bool ok = true;
    ok = ok && MH_CreateHook(pEndScene, &hkEndScene,
                             reinterpret_cast<void**>(&oEndScene)) == MH_OK;
    ok = ok && MH_CreateHook(pReset, &hkReset,
                             reinterpret_cast<void**>(&oReset)) == MH_OK;
    ok = ok && MH_CreateHook(pDIP, &hkDrawIndexedPrimitive,
                             reinterpret_cast<void**>(&oDrawIndexedPrimitive)) == MH_OK;

    if (!ok)
    {
        MH_Uninitialize();
        return 0;
    }

    // Enable all hooks at once
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        MH_Uninitialize();
        return 0;
    }

    return 1;
}

// ---------------------------------------------------------------------------
// EndScene hook — ImGui init + render
// ---------------------------------------------------------------------------
HRESULT APIENTRY hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
    if (!g_bInitialized)
    {
        HWND hWnd = GetDeviceWindow(pDevice);
        if (!hWnd)
            return oEndScene(pDevice);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX9_Init(pDevice);

        // Subclass the game's window
#ifdef _WIN64
        oWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrA(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc)));
#else
        oWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrA(hWnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc)));
#endif

        g_bInitialized = true;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Show/hide the cursor with the menu
    ImGui::GetIO().MouseDrawCursor = g_bMenuOpen;

    if (g_bMenuOpen)
    {
        ImGui::ShowDemoWindow(&g_bMenuOpen);

        // ---------------------------------------------------------------
        // Add your own ImGui windows here, e.g.:
        //   ImGui::Begin("My Menu");
        //   ImGui::Text("Hello!");
        //   ImGui::End();
        // ---------------------------------------------------------------
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    return oEndScene(pDevice);
}

// ---------------------------------------------------------------------------
// Reset hook — must invalidate / recreate ImGui device objects
// ---------------------------------------------------------------------------
HRESULT APIENTRY hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pParams)
{
    // Invalidate GPU resources BEFORE the device resets
    if (g_bInitialized)
        ImGui_ImplDX9_InvalidateDeviceObjects();

    HRESULT hr = oReset(pDevice, pParams);

    // Recreate them AFTER a successful reset
    if (g_bInitialized && SUCCEEDED(hr))
        ImGui_ImplDX9_CreateDeviceObjects();

    return hr;
}

// ---------------------------------------------------------------------------
// DrawIndexedPrimitive hook — placeholder for model/texture mods
// ---------------------------------------------------------------------------
HRESULT APIENTRY hkDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type,
                                        INT BaseVertexIndex, UINT MinVertexIndex,
                                        UINT NumVertices, UINT StartIndex, UINT PrimCount)
{
    return oDrawIndexedPrimitive(pDevice, Type, BaseVertexIndex, MinVertexIndex,
                                NumVertices, StartIndex, PrimCount);
}

// ---------------------------------------------------------------------------
// Clean shutdown — called from DllMain DLL_PROCESS_DETACH
// ---------------------------------------------------------------------------
void HookShutdown()
{
    // Disable hooks first so no more callbacks fire
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    // Restore the original WndProc
    if (oWndProc && g_hTargetWnd)
    {
#ifdef _WIN64
        SetWindowLongPtrA(g_hTargetWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));
#else
        SetWindowLongPtrA(g_hTargetWnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));
#endif
        oWndProc = nullptr;
    }

    // Now safe to tear down ImGui
    if (g_bInitialized)
    {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_bInitialized = false;
    }
}
