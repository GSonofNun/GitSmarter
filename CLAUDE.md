# CLAUDE.md | [GitSmarter Index]|root: .
|IMPORTANT: Prefer retrieval-led reasoning over pre-training-led reasoning
|src/:{main.cpp,platform.cpp,network.cpp,file_io.cpp,settings.cpp,syntax.cpp}
|src/widgets/:{CLAUDE.md,ui_widgets_*.cpp:Widget,EventDispatcher,FlexContainer}
|src/git/:{CLAUDE.md,*.cpp:GitRepo,GitObject,PackFile,diff,commit}
|include/:{app.h:core types,ui_theme.h:colors}
|docs/:{*.md:specifications}
|lib/:{zlib-ng/:compression}

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Engineering Principles (MANDATORY)

**These principles are non-negotiable and MUST guide all development decisions:**

- **Tackle complexity head-on** - Sweat equity builds unbreakable code. Never avoid hard problems.
- **Optimize ruthlessly** - Lean memory, blazing speed, zero excuses. Performance is paramount.
- **Question dependencies** - Reinvent wheels if ours turn faster and consume less. No cargo-culting.
- **Craft exceptional UIs** - Dazzle users with every detail. Ugly apps die; mediocre design loses users.

## Build Commands

```batch
# Build the application (requires Visual Studio 2022)
cmd //c "<repo_root>/build.bat"

# Clean build artifacts
cmd //c "<repo_root>/build.bat" clean

# Force full rebuild including zlib-ng
cmd //c "<repo_root>/build.bat" rebuild

# Build and run tests
cmd //c "<repo_root>/build.bat" test

# Run tests with filter (only tests matching pattern)
cmd //c "<repo_root>/build.bat" test --filter=pack

# Build tests only (no run) for debugging
cmd //c "<repo_root>/build.bat" test --build-only
```

The build produces `GitSmarter.exe` - a single executable with no external runtime dependencies. The build auto-detects host architecture (x64 or ARM64) and compiles natively.

### Running Tests from a Worktree

When working in a git worktree, you must run builds from within the worktree directory:

```batch
# From worktree directory (REQUIRED for worktrees)
cmd //c "pushd <worktree_path> && .\build.bat test && popd"
```

**Important:** Running `build.bat` with just a path prefix (e.g., `C:\path\build.bat test`) will use the wrong current directory and may compile files from the main repo instead of the worktree.

### GitHub CLI Commands

GitHub CLI may not be in PATH on Windows. Use the full path:

```batch
# Create PR with GitHub CLI
"C:\Program Files\GitHub CLI\gh.exe" pr create --title "Title" --body "Body"

# View PR inline comments
"C:\Program Files\GitHub CLI\gh.exe" api repos/OWNER/REPO/pulls/NUM/comments
```

### Git Operations with PowerShell

Use PowerShell for git commands instead of cmd - it handles quoting and multiline strings better:

```powershell
# Simple git commands
powershell -Command "cd '<repo_path>'; git status"

# Multiline commit messages using here-string
powershell -Command "cd '<repo_path>'; git commit -m @'
Summary line
- Bullet point 1
- Bullet point 2
'@"
```

### Architecture Detection

The build queries the machine-level `PROCESSOR_ARCHITECTURE` from the registry because process-level `%PROCESSOR_ARCHITECTURE%` returns `AMD64` when cmd.exe runs under x64 emulation on ARM64 Windows. This ensures ARM64 machines build native ARM64 binaries.

## Code Style

- **C++20** with MSVC, `/W4` warnings enabled
- **Unity build**: `main.cpp` includes all `.cpp` files - never add `.cpp` to build separately
- **Naming**: `snake_case` for functions/variables, `PascalCase` for types/enums, `g_` prefix for globals
- **Constants**: `constexpr` in namespaces (`Config::`, `Theme::`, `Git::`)
- **Memory**: `new[]`/`delete[]` for dynamic allocations, `MemoryMap` for file I/O
- **Strings**: Fixed-size `char[]` buffers with `strcpy_s`/`strncpy_s`, wide strings for Windows API paths
- **Error handling**: Return `bool` or `nullptr`, check with early returns, no exceptions
- **File organization**: Declarations in `include/app.h`, implementations in `src/*.cpp`
- **Globals**: File-scoped `static` (e.g., `static ID2D1Factory* g_d2d_factory`)
- **Comments**: Single-line `//` preferred, section headers with `// ===` separators
- **Safe string functions**: MSVC signatures - `strncpy_s(dest, dest_size, src, count)` requires 4 params, `swprintf(buf, _countof(buf), fmt, ...)` uses `_countof()` not magic numbers

## Architecture Overview

GitSmarter is a native Windows Git client written in C++ using Direct2D for rendering. It follows a **unity build** pattern where `main.cpp` includes all other `.cpp` files, enabling whole-program optimization and producing a compact binary (~2.5 MB).

### File Structure

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point, includes other .cpp files (unity build) |
| `src/platform.cpp` | Windows API: window creation, Direct2D, message loop, dialogs |
| `src/network.cpp` | HTTP/HTTPS: Smart Protocol v2, OAuth device flow, fetch/push/pull |
| `src/file_io.cpp` | File I/O, memory mapping, zlib decompression |
| `src/settings.cpp` | Application settings persistence |
| `src/syntax.cpp` | Syntax highlighting for diff viewer |
| `src/git/*.cpp` | Git operations (see `src/git/CLAUDE.md`) |
| `src/widgets/*.cpp` | Widget UI framework (see `src/widgets/CLAUDE.md`) |
| `include/app.h` | Core types, structs, constants, function declarations |
| `include/ui_theme.h` | Theme colors (ARGB), layout constants |
| `lib/zlib/` | Bundled zlib for decompressing git objects |

### Widget UI Framework

The UI is built on a custom Direct2D widget system in `src/widgets/`. See `src/widgets/CLAUDE.md` for file details.

**Key widget patterns:**
- Arena allocator: `g_widget_arena.create<MyWidget>()`
- Tree structure: `widget_add_child(parent, child)`
- Layout: Override `measure()` and `layout()` methods
- Events: Override `on_mouse_*()`, `on_key_*()` methods, return `true` to consume
- Flags: `WidgetFlags::Visible`, `Enabled`, `Focusable`, `Hovered`, `Pressed`, `ClipChildren`

For detailed widget development, use the `/widget-development` skill.

### Code Navigation

Large `.cpp` files contain auto-generated tables of contents. TOCs are delimited by:
- Start marker: `// <AUTO-GENERATED TOC> DO NOT EDIT MANUALLY`
- End marker: `// </AUTO-GENERATED TOC>`

**Files with TOCs**: `platform.cpp`, `core.cpp`, `network.cpp`

When looking for a specific type or function, check the TOC first and use line numbers to jump directly.

### Key Design Patterns

**File-scoped static state** - Global state is `static` at file scope (e.g., `g_d2d_factory`, `g_main_window` in platform.cpp)

**Memory-mapped I/O** - Large files (pack files) use `MemoryMap` struct via `mmap_open()`/`mmap_close()`

**Git implementation** - Direct parsing of git internals (loose objects, pack files, refs) without libgit2 or CLI

**Rename detection** - `GitStatusEntry` includes `old_path` and `similarity` fields; detection happens in `git_status_compute()` by comparing content of staged deleted vs added files using hash-based line matching

**Credential priority chain** - `git_credential_fill()` checks sources in order: Windows Credential Manager → URL-embedded → .gitconfig → environment variables (GH_TOKEN/GITHUB_TOKEN). Higher priority sources shadow lower ones even if invalid.

**Timer ID allocation** - All timer IDs centralized in `app.h` with documented allocation table (IDs 1-12). Always check the table before adding new timers to avoid collisions.

**Frame-driven animation system** - Fully self-driving render loop with no timer dependency. `FrameClock` uses `QueryPerformanceCounter` for real delta time (100ms clamp prevents debugging jumps). Periodic refresh rate re-detection every 5 seconds via `DwmGetCompositionTimingInfo` handles VRR/monitor changes. `AnimationScheduler` tracks active animations via atomic counter; `animation_started()` calls `InvalidateRect()` on 0→1 transition to kick the loop. `render_frame()` ticks all animations before drawing and self-schedules via `InvalidateRect()` when animations active. `DwmFlush()` in message loop syncs to vsync during animations. Debug overlay (F3 key, debug builds) shows FPS, frame time, refresh rate, animation count. Near-zero CPU when idle, smooth 60+ fps when animating.

**IPC System (Debug Only)** - Named pipe server (`\\.\pipe\GitSmarterIPC`) enables external control for automated testing. JSON protocol supports screenshot capture (GDI BitBlt → WIC PNG), mouse/keyboard injection (PostMessage), and window info queries. Only compiled in `GITSMARTER_DEBUG` builds.

**Direct-to-disk streaming** - For large downloads, write directly to temp file during HTTP read, memory-map for concurrent parsing with sliding window, then rename to final location. Eliminates double-copy (network→memory→disk) and reduces memory from O(file_size) to O(window_size).

**Lock-free delta resolution** - Treiber stack-based work queue eliminates level-by-level barriers in delta resolution. Workers pop ready items, resolve deltas, atomically decrement dependent counters, and push newly-ready children. Pre-allocated node pool avoids allocation during parallel phase.

**zlib-ng integration** - zlib-ng added as git submodule at `lib/zlib-ng/`, built on-demand via CMake from `build.bat`. First build runs CMake (~30s), subsequent builds use cached `zlibstatic.lib`. Provides 4.2x faster decompression via SIMD (SSE2/AVX2/AVX512 on x64, NEON on ARM64). Build auto-detects host architecture and compiles natively.

**Native ARM64 build support** - Build auto-detects host architecture by querying machine-level registry (not process-level `%PROCESSOR_ARCHITECTURE%` which reports AMD64 under x64 emulation). Uses `vcvarsall.bat arm64` and `cmake -A arm64` for native ARM64 compilation with NEON SIMD. Architecture-specific build directories (`build-arm64/`, `build-x64/`) allow caching both.

**Concurrent index build for clone** - Clone optimization that builds Git index during parallel checkout instead of separate stat phase. `git_checkout_tree_for_clone()` collects metadata (size from blob read, synthetic timestamps, incrementing inode) in workers, then builds index from work items in O(N) with no syscalls. Eliminates 4N stat syscalls (CreateFileW + GetFileInformationByHandle + GetFileSizeEx + CloseHandle per file).

**Cache sharding for large clones** - `ShardedObjectCache` wraps `BoundedObjectCache` with 16 independent shards, each with its own SRWLOCK. Routes by `obj_index & 0xF` to distribute lock contention across shards (~6% collision probability with 32 threads vs 100% with single lock). Configurable via `clone_cache_size_gb` in settings.json (0 = auto 25% RAM, >0 = fixed GB); defaults to 25% of RAM with 512MB min, 4GB max.

**Command-line clone feature** - Supports `GitSmarter.exe clone [url] [dest]` with GUI mode and `--headless` for console mode. URL shorthand (`gh:owner/repo` or `owner/repo`) expands to full GitHub HTTPS URLs. When both URL and dest provided in GUI mode, clone auto-starts; otherwise dialog opens for user input.

**Category-based logging system** - `LOG_CAT(category, ...)` macros with compile-time zero-overhead via `if constexpr` and `LOG_CATEGORY_ENABLED()`. Categories (Error, Warning, Info, Debug, etc.) use bitfield enums for efficient filtering. Subsystem filtering (`LOG_DEBUG(CommitView, ...)`) enables granular debug output. Telemetry ring buffer (lock-free, 64-byte aligned entries) captures structured performance data. Session-based log files (`%TEMP%\GitSmarter_YYYYMMDD_HHMMSS_PID.log`) prevent unbounded growth.

### Threading Model

- Main thread: Windows message loop, Direct2D rendering
- Worker thread: Dedicated thread for fetch/push/pull operations (not thread pool)
- Communication: `PostMessage()` for completion signals, atomics for progress

### Configuration Namespaces

- `Config::` - Window dimensions, app constants
- `Theme::` - Colors (ARGB format), spacing, corner radii
- `Git::` - Git format constants (SHA sizes, object types)

## Technical Constraints

- **Zero external dependencies** except Windows SDK and bundled zlib
- **C++20 standard** with MSVC
- **Direct2D/DirectWrite** for all rendering (no GDI)
- **Borderless window** with custom title bar and DWM integration

## Status

- [x] Project structure (unity build, source layout)
- [x] Build pipeline (build.bat, auto-vcvars, ~2.5 MB exe)
- [x] Specifications (docs/GitSmarter_Development_Spec.md)
- [x] Custom title bar (borderless window, DWM rounded corners/shadow)
- [x] Window controls (minimize/maximize/close with hover states)
- [x] Close animation (fade+scale effect)
- [x] Memory-mapped file I/O
- [x] Zlib decompression (for git loose objects)
- [x] Git repository detection and opening
- [x] HEAD parsing (branch name, detached state)
- [x] Pack file loading (.idx and .pack parsing)
- [x] Commit reading (from loose objects and pack files)
- [x] Reference listing (branches, tags, remotes)
- [x] Sidebar layout (branches, changed files, staging)
- [x] Diff viewer (unified mode with line numbers and scrolling)
- [x] File staging/unstaging
- [x] Commit creation
- [x] Fetch operations (Smart HTTP Protocol v2, async worker thread)
- [x] Progress UI (status bar with phase text and progress bar)
- [x] Background sync (5-min timer with ahead/behind indicators)
- [x] Push operations (Smart HTTP Protocol v2, pack generation, fast-forward check)
- [x] Pull operations (fast-forward only, tree checkout, index rebuild)
- [x] Commit history with graph visualization
- [x] Widget UI framework migration complete
- [ ] Diff viewer keyboard scrolling (Page Up/Down, Home/End)
- [ ] Side-by-side diff mode

## Decisions

- **Custom title bar over standard Win32**: WS_POPUP + WM_NCHITTEST for borderless window with drag/resize
- **Direct git parsing over libgit2/CLI**: Zero dependencies, full control, educational value
- **Unity build**: Single TU for whole-program optimization, faster builds, smaller binaries
- **Memory-mapped pack files**: Efficient random access for large repositories
- **Custom widget system over Win32 controls**: Direct2D rendering, virtualized lists, full styling control
- **GITSMARTER_DEBUG over _DEBUG**: Custom macro for debug builds avoids CRT debug library dependencies (_CrtDbgReport linker errors) while still enabling debug-only features like the titlebar debug button
- **Self-driving animation loop**: AnimationScheduler kicks render loop via InvalidateRect() on 0→1 transition; render_frame() self-schedules when animations active. No timer dependency - immune to WM_TIMER starvation during mouse floods. Near-zero CPU when idle.
- **zlib-ng over bundled zlib**: 4.2x faster decompression (2079ms → 491ms) via SIMD. Submodule + on-demand CMake build avoids committing binaries while ensuring correct compiler version. Trade-off: first build ~30s slower for CMake configure.
- **Keep build.bat over CMake for GitSmarter**: Unity build is natural with `cl main.cpp`, no configure step, Windows-only by design so cross-platform benefit doesn't apply.
- **Native ARM64 over cross-compiled x64**: ARM64 Windows machines now build native ARM64 binaries instead of cross-compiling to x64. Enables NEON SIMD in zlib-ng. Clone performance: 88.45s vs Git CLI's 89.69s (parity).
- **Synthetic metadata over stat for clone index**: Clone builds index with synthetic timestamps (single `GetSystemTimeAsFileTime()` at start), incrementing atomic inode counter, and size from blob read. Zero stat syscalls. Valid because: (1) clone is green-field with no prior state, (2) future operations refresh metadata anyway, (3) unique inodes preserve hardlink detection capability.
- **Linux kernel clone achieved**: 289s total (download 72s, delta resolution 80s, checkout 109s) vs Git CLI 275s - within 5% parity. 11.3M objects, 9.15M deltas, 6GB pack, 92K files, 6K directories. Key enablers: 16GB cache (`--cache-size=16`), 32 threads (`GITSMARTER_DELTA_THREADS=32`), sharded cache.
- **Settings-based cache config over CLI flags**: Removed `--cache-size` and `--cache-unlimited` CLI flags in favor of `clone_cache_size_gb` in settings.json. Cleaner UX - users configure once, not per-invocation. Value 0 = auto (25% RAM), >0 = fixed GB.

## Blockers

(none)

## Known Issues & Fixes

| Issue | Root Cause | Fix | Date |
|-------|------------|-----|------|
| Unity build naming conflicts | Static functions with same name in different .cpp files conflict in unity build | Prefix function names with context (e.g., `rename_is_binary` vs `is_binary_content`) | 2026-01-02 |
| GitHub re-auth loop | Git Credential Manager (GCM) overwrites GitSmarter's OAuth token with different credential format using same target `git:https://github.com` | Changed credential target from `git:` to `gitsmarter:` prefix to isolate from GCM | 2026-01-03 |
| Side-by-side diff lines jumbled | Greedy similarity matching output pairs in similarity order, not line order | Use non-crossing constraint matching + block replacement detection (<30% matches → output all removals then additions) | 2026-01-03 |
| ScrollContainer reserving space for horizontal scrollbar in side-by-side mode | `show_scrollbar_h = true` caused 14px gap at bottom even though DiffViewerWidget handles its own h-scrollbar | Set `show_scrollbar_h` based on view mode (false for side-by-side, true for inline) | 2026-01-03 |
| strncpy_s wrong param order | MSVC strncpy_s requires 4 params: (dest, dest_size, src, count) | Use `strncpy_s(buf, sizeof(buf), src, n)` not `strncpy_s(buf, src, n)` | 2026-01-07 |
| Timer ID collision breaks animations | `CLONE_PROGRESS_TIMER_ID` (10) and `ANIMATION_TICK_TIMER_ID` (10) both used same value; progress timer check returns early, blocking animation ticks | Changed `ANIMATION_TICK_TIMER_ID` to 11 | 2026-01-07 |
| Dialog content height miscalculation | `Dialog::layout()` subtracted `PADDING * 2` but only applied padding to bottom, wasting 24px of content area | Changed to `dialog_height - title_height - PADDING` (single padding) | 2026-01-07 |
| Clone dialog tick not called | `tick_clone_dialog_tree()` existed but wasn't called in timer handler, and `dialog_widgets.h` wasn't included in `platform.cpp` | Added tick call and include | 2026-01-07 |
| Debug build LNK2019 _CrtDbgReport | Using `_DEBUG` macro triggers CRT debug library dependencies | Use custom `GITSMARTER_DEBUG` macro instead of `_DEBUG` for debug-only code | 2026-01-07 |
| Dialog crashes when opening from another dialog | Arena reset in `show()` invalidates DialogWidget allocated before `show()` is called | Move arena reset to `dialog_arena_prepare()` called before DialogWidget allocation; currently disabled as persistent widgets share same arena | 2026-01-07 |
| IPC ReadLine hangs | Response messages missing newline character | Add `\n` to all JSON response strings for PowerShell ReadLine() compatibility | 2026-01-07 |
| Dialog not rendering after creation | Missing registration in `dialog_widgets_render()` and `any_dialog_widget_is_active()` | Add dialog instance check and render/is_active calls to both functions | 2026-01-08 |
| Menu item appears disabled unexpectedly | MenuItem `checked` and `enabled` flags swapped in initializer | Field order is `{..., checked, enabled, ...}` - ensure `false, true` for unchecked+enabled | 2026-01-08 |
| Hidden widgets render at wrong position when made visible | Called `set_dirty()` instead of `measure_and_layout()` | Use `measure_and_layout()` when toggling visibility to recalculate positions | 2026-01-08 |
| Stack overflow in packed-refs functions | 256KB stack buffers (`char content[1024 * 256]`) exceed default 1MB Windows stack when nested | Use heap allocation (`new[]`/`delete[]`) for large buffers in `git_branch_exists`, `git_branch_delete`, `git_branch_rename`, `git_tag_exists` | 2026-01-08 |
| `stat_file_for_index` overwrites file mode | Function unconditionally sets `entry->mode = Git::MODE_FILE`, losing executable bit from tree | Only set mode if `entry->mode == 0` (preserve mode set by caller) | 2026-01-08 |
| Timer ID collision (OAUTH vs CHECKOUT) | Both `OAUTH_COUNTDOWN_TIMER_ID` and `CHECKOUT_PROGRESS_TIMER_ID` used ID 8 | Reassigned OAUTH to 12; centralized all timer IDs in app.h with allocation table | 2026-01-08 |
| Scroll animations snap instead of animating | Animation duration (100ms) matched delta time clamp (100ms), allowing single frame to complete entire animation | Increased `SCROLL_ANIM_DURATION` from 0.1f to 0.25f (250ms) in ui_widgets_diff.cpp | 2026-01-09 |
| CodeEditor scrollbar not updating on mouse wheel | ContentWidget consumed wheel event (returned true), so parent CodeEditorWidget never updated scrollbar geometry | Have ContentWidget return false from on_mouse_wheel, let parent handle scroll + scrollbar update | 2026-01-10 |
| CodeEditor scrollbar drag stops at widget boundary | Missing `WidgetFlags::CapturesMouse` flag; EventDispatcher only sends events to widget during drag if flag is set | Add `flags |= WidgetFlags::CapturesMouse` in constructor | 2026-01-10 |
| Focus invariants violated by manual flag toggle | CodeEditorWidget::on_focus() manually set `content_->flags |= Focused`, violating dispatcher ownership | Have child widget check parent's focus state instead: `static bool editor_has_focus(Widget* p) { return p && (p->flags & Focused); }` | 2026-01-10 |
| UTF-8 chunk boundary corruption | Chunked UTF-8 scanning advanced by `read` bytes even when multi-byte sequence incomplete at boundary | Track `bytes_consumed` separately; only advance by consumed; break when `i + char_bytes > read` | 2026-01-10 |
| Redo moves cursor to position 0 | `undo()` saved redo snapshot with `cursor_pos = 0` | Add `current_cursor_pos` parameter to `undo()`: `size_t undo(size_t current_cursor_pos = 0)` | 2026-01-10 |
| UTF-8 word boundary splitting | `get_word_boundary_left/right()` used `pos++/--` byte iteration, splitting multibyte UTF-8 characters during double-click word select and Ctrl+Arrow navigation | Use `find_char_start()` to locate UTF-8 leading bytes (not continuation `10xxxxxx`), advance by `utf8_char_len()` to step by codepoints; categorize as ASCII-word, ASCII-non-word, or non-ASCII | 2026-01-12 |
| set_selection() cursor mismatch | Header comment claimed "cursor moves to end_byte" but implementation only set selection range, causing cursor/selection mismatch after undo/redo | Update `set_selection()` to also set `cursor_byte_pos_ = end_byte`, recompute line/column/preferred_column/target_x, call `reset_cursor_blink()`, `ensure_cursor_visible()`, `set_dirty()` | 2026-01-12 |
| Bash piped commands scan entire drive | Piping `findstr` to `find` with unquoted Windows path like `C:\temp\file.txt` causes shell to interpret backslashes as path separators, running `find` on `/c/` | Always quote Windows paths in piped commands: `findstr "pattern" "C:\temp\file.txt"` or use PowerShell instead | 2026-01-13 |
| Memory-mapped view sees stale data during streaming | File mapping created with old file size, not refreshed as file grows; 1MB remap threshold too conservative near end of file | Force remap when `download_complete=true` AND file size > mapping end (no threshold); add `FlushFileBuffers()` before signaling download complete | 2026-01-14 |
| CMake builds ARM64 instead of x64 on ARM64 Windows | CMake detects native architecture, not target | Build now auto-detects and compiles for native arch (ARM64 or x64) | 2026-01-14 |
| Process-level PROCESSOR_ARCHITECTURE wrong on ARM64 | cmd.exe under x64 emulation reports AMD64 even on ARM64 hardware | Query machine-level arch from registry: `HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\PROCESSOR_ARCHITECTURE` | 2026-01-14 |
| Clone files_checked_out included submodules | `clone_index.entry_count` includes submodule entries which aren't files on disk | Use `progress->files_done.load()` which tracks actual files written | 2026-01-15 |
| SHA-1 validation fails for pack files >4GB | `BCryptHashData` takes `ULONG` (32-bit) size; casting 6GB value truncates to ~1.6GB, computing wrong hash | Chunk data into 1GB pieces in `sha1_compute_bcrypt()` loop | 2026-01-15 |
| Clone only checks out 32K files on large repos | `Git::MAX_TREE_ENTRIES` was 32768, truncating tree traversal for repos like Linux kernel (92K files) | Increased to 200000 | 2026-01-15 |
| Clone checkout fails with 20K missing directories | `batch_create_directories()` had `MAX_DIRS = 4096`; Linux kernel has ~4500 directories, loop stopped collecting at limit | Increased MAX_DIRS to 8192 | 2026-01-19 |
| Single file checkout failure on case collision | Windows NTFS is case-insensitive; files like `xt_CONNMARK.h` and `xt_connmark.h` collide | Detect collision (file exists after write fails), return success, increment `case_collisions` counter | 2026-01-19 |
| Lock contention causes slow delta resolution | Single SRWLOCK on `BoundedObjectCache` causes 10-15s `cache_put` times with 32 threads | Implemented `ShardedObjectCache` with 16 shards, each with independent lock | 2026-01-19 |
| `log_get_timestamp_us()` division by zero | `g_log.perf_freq.QuadPart` is 0 when logging system uninitialized; telemetry macros call this before `log_system_init()` | Auto-initialize `perf_freq` in `log_get_timestamp_us()` if QuadPart is 0 | 2026-01-22 |
| ANSI cursor save/restore broken on Windows | `\033[s` and `\033[u` sequences don't work reliably in Windows Terminal/PowerShell | Use `\033[4A` (move up N lines) instead; document why hardcoded line count is safe | 2026-01-22 |
| WM_TIMER starvation during animations | Other animations calling `InvalidateRect()` in render loop flood message queue, starving WM_TIMER messages | Convert timer-based animations to AnimationScheduler pattern; all animations tick cooperatively from `tick_all_animations()` | 2026-01-22 |

## Automated Testing via IPC (Debug Builds)

Debug builds include an IPC server that allows Claude Code to control and inspect the running app without user interaction.

### Starting the Test Environment
1. Build debug: `cmd //c "build.bat debug"`
2. Launch `GitSmarterDebug.exe`
3. The IPC server starts automatically on pipe `\\.\pipe\GitSmarterIPC`

### Available Commands
```powershell
# Get window info (size, title, focus state)
.\scripts\test_ipc.ps1 -Command info

# Capture screenshot to file
.\scripts\test_ipc.ps1 -Command screenshot -OutputFile capture.png

# Click at coordinates
.\scripts\test_ipc.ps1 -Command click -X 100 -Y 200 -Button left

# Move mouse
.\scripts\test_ipc.ps1 -Command move -X 100 -Y 200

# Press key (virtual key code)
.\scripts\test_ipc.ps1 -Command key -VK 13  # Enter key

# Type character
.\scripts\test_ipc.ps1 -Command char -Char "A"
```

### Testing Workflow
1. Make code changes
2. Build debug version
3. Launch app (or restart if already running)
4. Use IPC to navigate UI and capture screenshots
5. Verify visual state matches expectations

### JSON Protocol (Direct Pipe Access)
```json
{"type":"get_info"}
{"type":"screenshot","format":"png"}
{"type":"mouse_click","x":100,"y":200,"button":"left"}
{"type":"key_down","vk":13,"ctrl":false,"shift":false,"alt":false}
{"type":"char","char":"A"}
```

### Virtual Key Codes (Common)
- Enter: 13, Tab: 9, Escape: 27, Space: 32
- Arrow keys: Left=37, Up=38, Right=39, Down=40
- Letters A-Z: 65-90, Numbers 0-9: 48-57
