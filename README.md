# DirectX9 ImGui Hook

A DirectX 9 overlay hook using [Dear ImGui](https://github.com/ocornut/imgui) and [MinHook](https://github.com/TsudaKageworked/minhook). Inject this DLL into a D3D9 application to render an ImGui overlay on top of the game's scene.

> **Educational / authorized testing use only.** This project is provided for learning purposes — game modding on your own software, CTF challenges, and security research with proper authorization.

![Screenshot](https://user-images.githubusercontent.com/34404195/78591004-0b8aa900-7843-11ea-91ec-609f0a16c4c6.png)

## Features

- Hooks `EndScene`, `Reset`, and `DrawIndexedPrimitive` via MinHook
- Renders the ImGui demo window as a starting point
- **INSERT** key toggles the overlay on/off
- Input passthrough — game receives input when the menu is closed
- Proper `Reset` handling — no crash on alt-tab or resolution change
- Auto-detects the game window from the D3D9 device (no hardcoded window name)
- Clean DLL unload — hooks are disabled before ImGui shutdown
- Supports both x86 and x64 targets

## Building

### Requirements

- Visual Studio 2022 (v143 toolset) — older versions work if you retarget
- Windows SDK 10.0+
- No external DirectX SDK needed — uses the headers shipped with the Windows SDK

### Steps

1. Open `directx9_imgui/directx9_imgui.sln` in Visual Studio
2. Select **Release | x86** or **Release | x64** (match your target application)
3. Build → the DLL appears in `build/Release/x86/` or `build/Release/x64/`

## Usage

1. Build the DLL for the correct architecture (x86 or x64) matching your target
2. Inject the DLL into a running D3D9 application using any DLL injector
3. Press **INSERT** to toggle the ImGui overlay
4. Customize `hkEndScene` in `directx9_imgui.cpp` to add your own ImGui windows

### Configuration

Edit the defines at the top of `directx9_imgui.h`:

```cpp
#define HOOK_MENU_KEY     VK_INSERT   // Change the toggle key
#define GAME_WINDOW_NAME  nullptr     // Set to e.g. "My Game" to target a specific window
```

## Project Structure

```
directx9_imgui/
├── directx9_imgui.sln          # Visual Studio solution
└── directx9_imgui/
    ├── directx9_imgui.h        # Hook declarations and config
    ├── directx9_imgui.cpp      # Hook implementations + ImGui rendering
    ├── dll_attach.cpp           # DllMain entry point
    ├── imgui/                   # Dear ImGui (vendored)
    └── min_hook/                # MinHook (vendored)
```

## License

This project is provided as-is for educational purposes.
