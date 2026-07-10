# Header Files

## Organization
- `app.h` - Core types, all function declarations, constants
- `ui_theme.h` - Theme colors (ARGB), layout constants
- `dialog_widgets.h` - Dialog-specific widget declarations

## Patterns
- All declarations in headers, implementations in src/*.cpp
- Use `constexpr` in namespaces for constants (Config::, Theme::, Git::)
- Forward declare structs when possible to reduce coupling

## Session Learnings
<!-- Populated by /save-memories -->
- `ipc.h` - IPC system header, wrapped in `#ifdef GITSMARTER_DEBUG` to exclude from release builds
- IPC custom messages use `WM_USER + 200` range (200-202) to avoid collision with existing messages (100-106)
- Dialog layout constants: Use `DialogBase::` for common constants, dialog-specific namespaces for unique values
- `DialogBase::COMPACT_FIELD_SPACING` (4.0f) for simpler dialogs like rename/tag; `DialogBase::FIELD_SPACING` (16.0f) for standard dialogs
- All dialog button rows should use consistent `DialogBase::BUTTON_SPACING` (12.0f)
- Dialog namespaces (AuthDialog, OAuthDialog, BranchDialog, StashDialog, CloneDialog) alias DialogBase for common values
- Timer IDs are centralized in `app.h` with a documented allocation table (IDs 1-12) - always check before adding new timers to avoid collisions
- `AnimationScheduler` struct added after `FrameClock` (line ~1700) with `std::atomic<int> active_count`
- Uses `std::memory_order_relaxed` for counter operations (single-threaded UI, no synchronization needed)
- Defensive underflow check in `animation_ended()`: clamps to 0 if count would go negative
- `AnimationScheduler` stores `HWND hwnd` member and calls `InvalidateRect()` on 0→1 transition in `animation_started()` to kick render loop without timer dependency
- Timer ID 11 now unused (was `ANIMATION_TICK_TIMER_ID`) - animation system is fully self-driving via render_frame() self-scheduling
- `FrameClock` struct extended with `last_refresh_check` (LARGE_INTEGER) and `maybe_update_refresh_rate()` for VRR support
- Use `static constexpr` for interval constants inside structs (e.g., `REFRESH_CHECK_INTERVAL = 5.0f`)
- `CommandLineArgs` struct in app.h enables shared access between main.cpp (defines `g_cli_args`) and platform.cpp (reads via `get_cli_args()`) in unity build
- `clone_dialog_widget_show_with_args(url, dest, auto_start)` allows pre-populating clone dialog from CLI arguments
- `log_types.h` defines Log namespace with Category/Subsystem enums, TelemetryEntry (64-byte aligned), TelemetryRing
- Compile-time mask `COMPILE_TIME_MASK` differs between debug (all categories) and release (Error, Warning, Telemetry only)
- `LOG_CATEGORY_ENABLED(cat)` is constexpr check used in `if constexpr` for zero-overhead disabled categories
