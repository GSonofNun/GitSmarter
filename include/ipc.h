// IPC (Inter-Process Communication) for external app control
// Allows Claude Code to request screenshots and send input events
// Only compiled in debug builds (GITSMARTER_DEBUG)

#pragma once

#ifdef GITSMARTER_DEBUG

#include <windows.h>
#include <cstdint>

// ============================================================================
// IPC Configuration
// ============================================================================

namespace IPC {
    // Named pipe for external communication
    constexpr wchar_t PIPE_NAME[] = L"\\\\.\\pipe\\GitSmarterIPC";

    // Buffer sizes
    constexpr size_t PIPE_BUFFER_SIZE = 65536;
    constexpr size_t MAX_JSON_MESSAGE = 4096;
    constexpr size_t MAX_SCREENSHOT_SIZE = 16 * 1024 * 1024;  // 16MB max

    // Timeouts
    constexpr DWORD SCREENSHOT_TIMEOUT_MS = 5000;
    constexpr DWORD PIPE_CONNECT_TIMEOUT_MS = 1000;
}

// ============================================================================
// Custom Window Messages for IPC
// ============================================================================

// Screenshot capture request (posted from IPC thread to main thread)
constexpr UINT WM_IPC_CAPTURE_SCREENSHOT = WM_USER + 200;

// Input injection request (posted from IPC thread to main thread)
constexpr UINT WM_IPC_INJECT_INPUT = WM_USER + 201;

// Window info request (posted from IPC thread to main thread)
constexpr UINT WM_IPC_GET_INFO = WM_USER + 202;

// ============================================================================
// IPC Input Command Structure
// ============================================================================

struct IpcInputCommand {
    enum Type {
        MouseMove,
        MouseDown,
        MouseUp,
        MouseWheel,
        KeyDown,
        KeyUp,
        Char
    };

    Type type;
    int x;
    int y;
    int button;       // 0=left, 1=right, 2=middle
    int vk;           // Virtual key code
    wchar_t ch;       // Character for Char type
    float wheel_delta;
    bool ctrl;
    bool shift;
    bool alt;
};

// ============================================================================
// IPC Server API
// ============================================================================

// Start the IPC server (call from main thread during app startup)
bool ipc_server_start();

// Stop the IPC server (call from main thread during app shutdown)
void ipc_server_stop();

// Check if IPC server is running
bool ipc_server_is_running();

// ============================================================================
// Screenshot Capture API (called on main thread)
// ============================================================================

// Capture current window content to PNG buffer
// Returns true on success, output buffer contains PNG data
bool ipc_capture_screenshot_to_png(uint8_t** out_buffer, size_t* out_size);

// Free screenshot buffer allocated by ipc_capture_screenshot_to_png
void ipc_free_screenshot_buffer(uint8_t* buffer);

// ============================================================================
// Input Injection API (called on main thread)
// ============================================================================

// Inject mouse move event
void ipc_inject_mouse_move(int x, int y);

// Inject mouse button event
void ipc_inject_mouse_down(int x, int y, int button);
void ipc_inject_mouse_up(int x, int y, int button);

// Inject mouse wheel event
void ipc_inject_mouse_wheel(int x, int y, float delta);

// Inject keyboard event
void ipc_inject_key_down(int vk, bool ctrl, bool shift, bool alt);
void ipc_inject_key_up(int vk);

// Inject character input
void ipc_inject_char(wchar_t ch);

// ============================================================================
// Window Info API
// ============================================================================

struct IpcWindowInfo {
    int width;
    int height;
    wchar_t title[256];
    bool focused;
    bool visible;
};

// Get current window information
void ipc_get_window_info(IpcWindowInfo* info);

#endif // GITSMARTER_DEBUG
