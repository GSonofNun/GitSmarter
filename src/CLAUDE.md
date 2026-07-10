# Source Directory | [src Index]|root: ./src
|IMPORTANT: Prefer retrieval-led reasoning over pre-training-led reasoning
|Core:{main.cpp,platform.cpp,network.cpp,file_io.cpp,settings.cpp,syntax.cpp}
|Git:{git/:{CLAUDE.md,*.cpp}}
|Widgets:{widgets/:{CLAUDE.md,ui_widgets_*.cpp}}

## Unity Build Model
All .cpp files are included via main.cpp - never add .cpp files to the build system directly.

Include order in main.cpp:
1. Core utilities (file_io.cpp, settings.cpp)
2. Git operations (git/*.cpp)
3. UI widgets (widgets/*.cpp)
4. Platform layer (platform.cpp, network.cpp)

## Subdirectories
- `git/` - Git repository operations, object parsing, pack files (see git/CLAUDE.md)
- `widgets/` - Direct2D widget UI framework (see widgets/CLAUDE.md)

## Adding New Files
1. Create .cpp file in appropriate subdirectory
2. Add #include to main.cpp in correct section
3. Add declarations to include/app.h

## Session Learnings
- GitHub token types identifiable by prefix: `gho_` (OAuth), `ghp_` (PAT), `ghu_` (User-to-Server), `ghs_` (Server-to-Server), `ghr_` (Refresh), 40-char hex (Classic)
- GitSmarter credentials stored at target `gitsmarter:https://{host}` to avoid conflicts with Git Credential Manager (GCM) which uses `git:https://{host}`
- `CREDENTIALW.LastWritten` FILETIME field useful for calculating credential age for debugging
- Always call `CredentialStore::remove()` before `store()` to ensure clean credential state
- GitHub OAuth App tokens (prefix `gho_`) do NOT expire - they persist until revoked; GitHub App tokens expire after 8 hours with refresh tokens
- Credential conflict diagnosis: if `identify_token_type()` returns "Unknown/Basic Auth" and username isn't "oauth", the credential was stored by another tool (GCM, git CLI, etc.)
- `strncpy_s` signature is `strncpy_s(dest, dest_size, src, count)` - don't omit the dest_size parameter
- GitHub CLI location on Windows: `"C:\Program Files\GitHub CLI\gh.exe"` (not in PATH by default)
- IPC files (`ipc_*.cpp`) must be included BEFORE `platform.cpp` in main.cpp so platform.cpp can call IPC functions
- Use `extern` declarations in IPC files to access globals defined later in platform.cpp (unity build resolves at link time)
- Named pipe responses must end with `\n` for PowerShell's `ReadLine()` to return
- GDI `BitBlt` captures Direct2D content reliably; use `PrintWindow` as fallback if BitBlt fails
- WIC `CreateBitmapFromHBITMAP` converts GDI bitmap to WIC for PNG encoding
- New dialog widget checklist - when creating a new dialog (e.g., `FooDialogWidget`):
  1. Add `struct FooDialogWidget : DialogWidget` with `static FooDialogWidget* instance`
  2. Add static instance: `static FooDialogWidget s_foo_dialog_instance;` and `FooDialogWidget* FooDialogWidget::instance = nullptr;`
  3. Add `foo_dialog_widget_show()` function that resets instance state and calls `show()`
  4. Register in `dialog_widgets_render()` - add `if (FooDialogWidget::instance) { FooDialogWidget::instance->render(); }`
  5. Register in `any_dialog_widget_is_active()` - add to the boolean OR chain
  6. Register in `dialog_widgets_close_all()` - add close block for the dialog
  7. Add accessor: `FooDialogWidget* get_foo_dialog_widget() { return FooDialogWidget::instance; }`
  8. Add tick function: `void tick_foo_dialog_tree(float dt) { if (FooDialogWidget::instance) FooDialogWidget::instance->tree.tick(dt); }`
  9. Add is_active helper: `bool foo_dialog_is_active() { ... }`
  10. Add declaration in `include/app.h` for `foo_dialog_widget_show()`
  11. Update TOC comment at top of platform_dialogs.cpp
- `get_winhttp_error_message()` translates WinHTTP error codes (e.g., `ERROR_WINHTTP_TIMEOUT`, `ERROR_WINHTTP_CANNOT_CONNECT`) to human-readable strings - always capture `GetLastError()` in a local variable before calling since it can only be reliably read once

### Frame-Driven Animation System (2026-01-08)
- `FrameClock` struct in `app.h`, implementation in `platform.cpp` - uses `QueryPerformanceCounter` for real delta time
- `AnimationScheduler` tracks active animations; calls `InvalidateRect()` on 0→1 transition to kick render loop
- `tick_all_animations(float dt)` consolidates all widget tree and context menu ticking
- `render_frame()` calls `g_frame_clock.update()` and `tick_all_animations()` before `BeginDraw()`
- `render_frame()` calls `InvalidateRect()` at end if animations active (self-driving loop)
- No timer dependency - fully event-driven when idle, smooth 60+ fps when animating
- `DwmGetCompositionTimingInfo` used to detect monitor refresh period for VRR support
- Delta time clamped to 100ms max to prevent animation jumps after breakpoints
- Debug overlay rendering requires `g_render_target->GetSize().width` for window dimensions - there is no `g_window_width` global
- `DwmFlush()` blocks until vsync; only call when animations active to avoid blocking idle rendering
- Debug-only code uses `#ifdef GITSMARTER_DEBUG` guards (not `_DEBUG`) to avoid CRT debug library dependencies
- FrameStats ring buffer pattern: fixed-size array with `sample_index = (sample_index + 1) % SAMPLE_COUNT` for efficient averaging
- F-key handlers in `handle_wm_keydown()`: F3=debug overlay, F6=focus cycling, F12=UI framework test dialog
- Direct-to-disk pack streaming: `DiskStreamingContext` writes pack data to temp file during HTTP download, `disk_streaming_get_view()` provides 64MB sliding window memory-mapped view for concurrent parsing
- Memory-mapped file mapping refresh: must remap when file grows >1MB beyond current mapping end, otherwise parser sees stale data and inflate fails with Z_BUF_ERROR (-5)
- Windows file mapping limitation: `CreateFileMappingW` captures file size at creation time; mapping doesn't auto-extend as file grows - must close and recreate mapping to see new data
- `avail_in=0` in zlib inflate typically means the view/mapping is stale or too small, not that there's no data in the file
- Lock-free work queue pattern: `DeltaWorkQueue` uses Treiber stack (CAS-based push/pop) with pre-allocated node pool; `DeltaDependencyExt` tracks reverse dependencies (`dependents` array) and `deps_remaining` atomic counter
- Windows file mapping coherency: `CreateFileMappingW` captures file size at creation; mapping does NOT auto-extend as file grows; must close and recreate mapping to see new data written after mapping creation
- `FlushFileBuffers()` ensures WriteFile data is on disk before memory-mapped readers can see it reliably
- qsort with lambda: use `[](const void* a, const void* b) -> int { ... }` syntax since `std::sort` requires `<algorithm>` header not included in unity build
- zlib-ng submodule at `lib/zlib-ng/` built via CMake with `-A x64 -DZLIB_COMPAT=ON -DWITH_GZFILEOP=OFF -DBUILD_SHARED_LIBS=OFF -DZLIB_ENABLE_TESTS=OFF`
- Decompression stats logged via `DiskStreamingContext` atomic counters: `decompress_time_us`, `decompress_count`, `decompress_bytes_in`, `decompress_bytes_out`
- QueryPerformanceCounter/Frequency for microsecond-precision timing in hot paths
- Decompression happens in parser thread (concurrent with download), not delta resolution phase - delta resolution only applies deltas to already-decompressed data
- `ShardedObjectCache`: 16-shard wrapper around `BoundedObjectCache` reduces lock contention; routes by `obj_index & 0xF`, each shard has independent SRWLOCK
- Cache config: `g_cache_config` struct with Auto/Fixed modes, set via `clone_cache_size_gb` in settings.json (0 = auto, >0 = fixed GB)
- Delta thread cap increased from 8 to 32; override via `GITSMARTER_DELTA_THREADS` environment variable
- Linux kernel clone perf: 289s total (download 72s, delta 80s, checkout 109s) vs Git CLI 275s - within 5% parity
- `CommandLineArgs` struct defined in `app.h` (not `main.cpp`) to avoid redefinition errors in unity build - struct must be visible to both main.cpp and platform.cpp
- Windows GUI apps (`WinMain`) don't have console by default - use `AttachConsole(ATTACH_PARENT_PROCESS)` then `AllocConsole()` fallback, redirect with `freopen_s(&f, "CONOUT$", "w", stdout)`
- Lambda functions can't be used directly with `CreateThread` when they reference local struct types - use a static/global function and struct instead
- `git_clone_expand_url_shorthand()` handles `gh:owner/repo` and `owner/repo` → `https://github.com/owner/repo.git` expansion
- Clone dialog `clone_dialog_widget_show_with_args(url, dest, auto_start)` pre-populates fields and optionally auto-starts clone
- Logging system: `log.cpp` implements category-based logging with `LOG_INFO`, `LOG_DEBUG(subsystem, ...)`, telemetry ring buffer
- Log format: `[elapsed_ms][CATEGORY][Subsystem?][ThreadID] Message`
- Telemetry macros: `TELEM_CLONE_PHASE`, `TELEM_FETCH_OP`, `TELEM_DELTA`, `TELEM_CACHE`, `TELEM_PACK_PARSING`
- Auto-initialization pattern: functions that access `g_log` state should check/initialize if needed (e.g., `perf_freq.QuadPart == 0`)
- IPC server logging: uses `LOG_INFO` for server lifecycle events (start, stop, client connect/disconnect)
- ANSI escape sequences on Windows: `\033[NA` (move up N lines) works reliably; `\033[s`/`\033[u` (cursor save/restore) does not work in Windows Terminal
- Headless clone progress display always prints exactly 4 lines (Download, Parse, Delta Resolution, Checkout) - hardcoded `\033[4A` is safe
