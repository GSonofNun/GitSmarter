// platform_file_watcher.cpp - File system watcher for repository changes
// Part of GitSmarter platform layer

#include "app.h"
#include "ui_theme.h"
#include <shobjidl.h>
#include <ctime>
#include <cmath>

// Forward declarations for globals defined in platform_window.cpp
extern HWND g_main_window;

// File system watcher state
struct FileWatcherState {
    HANDLE thread = nullptr;
    HANDLE dir_handle = INVALID_HANDLE_VALUE;
    HANDLE shutdown_event = nullptr;
    bool running = false;
};

static FileWatcherState g_file_watcher = {};

// Timer IDs and constants are now in app.h

// Forward declaration
static DWORD WINAPI file_watcher_thread_proc(LPVOID param);

// File watcher thread - monitors repository for file changes
static DWORD WINAPI file_watcher_thread_proc(LPVOID param) {
    HWND hwnd = (HWND)param;

    alignas(DWORD) char buffer[4096];
    DWORD bytes_returned;
    OVERLAPPED overlapped = {};
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (!overlapped.hEvent) {
        LOG("[FileWatcher] Failed to create event: %lu", GetLastError());
        return 1;
    }

    HANDLE wait_handles[2] = { overlapped.hEvent, g_file_watcher.shutdown_event };

    while (g_file_watcher.running) {
        // Start async read for directory changes
        if (!ReadDirectoryChangesW(
            g_file_watcher.dir_handle,
            buffer, sizeof(buffer),
            TRUE,  // Watch subtree
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytes_returned,
            &overlapped,
            nullptr)) {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING) {
                LOG("[FileWatcher] ReadDirectoryChangesW failed: %lu", error);
                break;
            }
        }

        // Wait for change notification or shutdown signal
        DWORD wait_result = WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE);

        if (wait_result == WAIT_OBJECT_0 + 1) {
            // Shutdown requested. Cancel the pending I/O and wait for it to complete
            // so the OS isn't still writing into `buffer` after we leave this function.
            CancelIo(g_file_watcher.dir_handle);
            DWORD ignored = 0;
            GetOverlappedResult(g_file_watcher.dir_handle, &overlapped, &ignored, TRUE);
            break;
        }

        if (wait_result == WAIT_OBJECT_0) {
            if (GetOverlappedResult(g_file_watcher.dir_handle, &overlapped, &bytes_returned, FALSE)) {
                // Filter changes - ignore most .git/ internals but react to index/refs
                bool should_notify = false;
                FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)buffer;

                while (info) {
                    // Extract filename from notification
                    int filename_len = info->FileNameLength / sizeof(wchar_t);
                    wchar_t filename[512] = {};
                    if (filename_len > 0) {
                        // Truncate over-long names but still preserve the prefix so the
                        // ".git\" check below still works (otherwise filename remains
                        // empty and a long path inside .git/ is treated as a working-tree
                        // change, triggering refresh storms).
                        int copy_len = (filename_len < 511) ? filename_len : 511;
                        memcpy(filename, info->FileName, copy_len * sizeof(wchar_t));
                        filename[copy_len] = L'\0';
                    }

                    // Check if this is a .git internal change
                    bool is_git_internal = (wcsncmp(filename, L".git\\", 5) == 0 ||
                                           wcsncmp(filename, L".git/", 5) == 0);

                    if (is_git_internal) {
                        // Only react to index, refs, logs, and HEAD changes (external git operations)
                        if (wcsncmp(filename, L".git\\index", 10) == 0 ||
                            wcsncmp(filename, L".git/index", 10) == 0 ||
                            wcsncmp(filename, L".git\\refs", 9) == 0 ||
                            wcsncmp(filename, L".git/refs", 9) == 0 ||
                            wcsncmp(filename, L".git\\logs", 9) == 0 ||
                            wcsncmp(filename, L".git/logs", 9) == 0 ||
                            wcscmp(filename, L".git\\HEAD") == 0 ||
                            wcscmp(filename, L".git/HEAD") == 0) {
                            should_notify = true;
                            LOG("[FileWatcher] Git internal change detected: %ls", filename);
                        }
                    } else {
                        // Working tree change - always notify
                        should_notify = true;
                    }

                    if (info->NextEntryOffset == 0) break;
                    info = (FILE_NOTIFY_INFORMATION*)((char*)info + info->NextEntryOffset);
                }

                if (should_notify) {
                    PostMessage(hwnd, WM_FILE_CHANGED, 0, 0);
                }
            }
            ResetEvent(overlapped.hEvent);
        }
    }

    CloseHandle(overlapped.hEvent);
    LOG("[FileWatcher] Thread exiting");
    return 0;
}

// Start file system watcher for repository
bool file_watcher_start(const wchar_t* repo_path) {
    if (g_file_watcher.running) return true;  // Already running

    g_file_watcher.dir_handle = CreateFileW(
        repo_path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (g_file_watcher.dir_handle == INVALID_HANDLE_VALUE) {
        LOG("[FileWatcher] Failed to open directory: %lu", GetLastError());
        return false;
    }

    g_file_watcher.shutdown_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_file_watcher.shutdown_event) {
        CloseHandle(g_file_watcher.dir_handle);
        g_file_watcher.dir_handle = INVALID_HANDLE_VALUE;
        LOG("[FileWatcher] Failed to create shutdown event: %lu", GetLastError());
        return false;
    }

    g_file_watcher.running = true;
    g_file_watcher.thread = CreateThread(nullptr, 0, file_watcher_thread_proc, g_main_window, 0, nullptr);

    if (!g_file_watcher.thread) {
        g_file_watcher.running = false;
        CloseHandle(g_file_watcher.dir_handle);
        CloseHandle(g_file_watcher.shutdown_event);
        g_file_watcher.dir_handle = INVALID_HANDLE_VALUE;
        g_file_watcher.shutdown_event = nullptr;
        LOG("[FileWatcher] Failed to create thread: %lu", GetLastError());
        return false;
    }

    LOG("[FileWatcher] Started monitoring: %ls", repo_path);
    return true;
}

// Stop file system watcher
void file_watcher_stop() {
    if (!g_file_watcher.running) return;

    g_file_watcher.running = false;
    SetEvent(g_file_watcher.shutdown_event);

    if (g_file_watcher.thread) {
        DWORD wait_result = WaitForSingleObject(g_file_watcher.thread, 3000);
        if (wait_result != WAIT_OBJECT_0) {
            // Thread didn't exit - leak handles to avoid UAF on shutdown_event.
            LOG("[FileWatcher] Thread did not exit within 3s (result=%lu) - leaking handles", wait_result);
            g_file_watcher.thread = nullptr;
            g_file_watcher.dir_handle = INVALID_HANDLE_VALUE;
            g_file_watcher.shutdown_event = nullptr;
            return;
        }
        CloseHandle(g_file_watcher.thread);
        g_file_watcher.thread = nullptr;
    }

    if (g_file_watcher.dir_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_file_watcher.dir_handle);
        g_file_watcher.dir_handle = INVALID_HANDLE_VALUE;
    }

    if (g_file_watcher.shutdown_event) {
        CloseHandle(g_file_watcher.shutdown_event);
        g_file_watcher.shutdown_event = nullptr;
    }

    LOG("[FileWatcher] Stopped");
}

// Check if file watcher is currently running
bool file_watcher_is_running() {
    return g_file_watcher.running;
}
