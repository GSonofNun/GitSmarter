// GitSmarter - A fast native git client for Windows
// Unity build: include implementation files

#include "log.cpp"
#include "file_io.cpp"
// TSF (Text Services Framework) for IME support - globals must come before widgets
#include "tsf/tsf_globals.cpp"
#include "syntax.cpp"
#include "fuzzy_match.cpp"
#include "command_registry.cpp"
// Git core operations (src/git/)
#include "git/git_syntax.cpp"
#include "git/git_history.cpp"
#include "git/git_test.cpp"
#include "git/git_objects.cpp"
#include "git/git_index.cpp"
#include "git/git_push.cpp"
#include "git/git_diff.cpp"
#include "git/git_commit.cpp"
#include "git/git_repo.cpp"
#include "git/git_checkout.cpp"
#include "git/git_clone.cpp"
#include "network.cpp"
#include "settings.cpp"
// Platform layer (lowest level - worker and file watcher)
#include "platform_worker.cpp"
#include "platform_file_watcher.cpp"
// UI Framework (src/widgets/)
#include "widgets/ui_widgets_core.cpp"
#include "widgets/ui_widgets_basic.cpp"
// TSF text store must come after ui_widgets_basic.cpp (needs TextInput definition)
#include "tsf/tsf_text_store.cpp"
#include "widgets/ui_widgets_list.cpp"
#include "widgets/ui_widgets_dialog.cpp"
#include "widgets/ui_widgets_context_menu.cpp"
#include "widgets/ui_widgets_sidebar.cpp"
#include "widgets/ui_widgets_diff.cpp"
#include "widgets/ui_widgets_commit.cpp"
#include "widgets/ui_widgets_welcome.cpp"
#include "widgets/ui_widgets_titlebar.cpp"
#include "widgets/ui_widgets_palette.cpp"
// Code Editor (src/widgets/code_editor/)
#include "widgets/code_editor/piece_table.cpp"
#include "widgets/code_editor/text_document.cpp"
#include "widgets/code_editor/line_index.cpp"
#include "widgets/code_editor/content_widget.cpp"
#include "widgets/code_editor/gutter_widget.cpp"
#include "widgets/code_editor/code_editor.cpp"
// IPC layer (debug only - for external app control, included before platform.cpp)
#include "ipc_screenshot.cpp"
#include "ipc_input.cpp"
#include "ipc_server.cpp"
// Platform layer (dialogs need to be after platform.cpp for globals access)
#include "platform.cpp"
#include "platform_dialogs.cpp"

// Command-line argument state (struct defined in app.h)
static CommandLineArgs g_cli_args;

// Accessor for command-line args (used by platform.cpp)
const CommandLineArgs* get_cli_args() { return &g_cli_args; }

// Forward declaration
static void show_help();
static int run_headless_clone();

// Thread context for headless clone operation
struct HeadlessCloneContext {
    CloneOptions* opts;
    CloneProgress* prog;
    CloneResult* res;
};

// Thread function for headless clone
static DWORD WINAPI headless_clone_thread_func(LPVOID param) {
    HeadlessCloneContext* ctx = static_cast<HeadlessCloneContext*>(param);
    git_clone(ctx->opts, ctx->prog, ctx->res);
    return 0;
}

// Parse command-line arguments
static void parse_command_line_args() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return;

    // Reset state
    g_cli_args = CommandLineArgs();

    for (int i = 1; i < argc; i++) {
        // Global --help flag
        if (wcscmp(argv[i], L"--help") == 0 || wcscmp(argv[i], L"-h") == 0) {
            g_cli_args.mode = CommandLineArgs::Mode::Help;
            break;
        }

        // clone subcommand
        if (wcscmp(argv[i], L"clone") == 0) {
            g_cli_args.mode = CommandLineArgs::Mode::Clone;

            // Parse clone arguments
            for (int j = i + 1; j < argc; j++) {
                if (wcscmp(argv[j], L"--headless") == 0) {
                    g_cli_args.headless = true;
                }
                else if (wcscmp(argv[j], L"--help") == 0 || wcscmp(argv[j], L"-h") == 0) {
                    g_cli_args.mode = CommandLineArgs::Mode::Help;
                    break;
                }
                else if (argv[j][0] != L'-') {
                    // Positional argument
                    if (!g_cli_args.has_url) {
                        // First positional: URL
                        char url_raw[512] = {};
                        WideCharToMultiByte(CP_UTF8, 0, argv[j], -1, url_raw, sizeof(url_raw), nullptr, nullptr);
                        git_clone_expand_url_shorthand(url_raw, g_cli_args.clone_url, sizeof(g_cli_args.clone_url));
                        g_cli_args.has_url = true;
                    }
                    else if (!g_cli_args.has_dest) {
                        // Second positional: destination
                        wcsncpy_s(g_cli_args.clone_dest, argv[j], _TRUNCATE);
                        g_cli_args.has_dest = true;
                    }
                }
            }
            break;
        }
    }

    LocalFree(argv);
}

// Show help text
static void show_help() {
    // Attach to parent console for output
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }
    FILE* f_out = nullptr;
    freopen_s(&f_out, "CONOUT$", "w", stdout);

    printf("\n");
    printf("GitSmarter - A native Windows Git client\n");
    printf("\n");
    printf("Usage:\n");
    printf("  GitSmarter.exe                              Open GitSmarter\n");
    printf("  GitSmarter.exe clone [url] [dest]           Clone a repository\n");
    printf("  GitSmarter.exe clone --headless url dest    Clone without GUI\n");
    printf("  GitSmarter.exe --help                       Show this help\n");
    printf("\n");
    printf("Clone Options:\n");
    printf("  --headless    Run without GUI (requires both url and destination)\n");
    printf("\n");
    printf("URL Formats:\n");
    printf("  https://github.com/owner/repo.git   Full URL\n");
    printf("  https://github.com/owner/repo       .git suffix auto-added\n");
    printf("  gh:owner/repo                       GitHub shorthand\n");
    printf("  owner/repo                          GitHub shorthand (no prefix)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  GitSmarter.exe clone gh:microsoft/vscode C:\\repos\\vscode\n");
    printf("  GitSmarter.exe clone --headless owner/repo .\\local-repo\n");
    printf("  GitSmarter.exe clone https://github.com/user/project.git\n");
    printf("\n");
}

// Run headless clone mode
static int run_headless_clone() {
    // Attach to parent console for output
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }
    FILE* f_out = nullptr;
    FILE* f_err = nullptr;
    freopen_s(&f_out, "CONOUT$", "w", stdout);
    freopen_s(&f_err, "CONOUT$", "w", stderr);

    // Validate arguments
    if (!g_cli_args.has_url) {
        fprintf(stderr, "\nError: URL required for headless clone\n");
        fprintf(stderr, "Usage: GitSmarter.exe clone --headless <url> <destination>\n\n");
        return 1;
    }

    if (!g_cli_args.has_dest) {
        fprintf(stderr, "\nError: Destination required for headless clone\n");
        fprintf(stderr, "Usage: GitSmarter.exe clone --headless <url> <destination>\n\n");
        return 1;
    }

    // Validate URL
    if (!git_clone_validate_url(g_cli_args.clone_url)) {
        fprintf(stderr, "\nError: Invalid URL '%s'\n", g_cli_args.clone_url);
        fprintf(stderr, "Expected formats:\n");
        fprintf(stderr, "  https://github.com/owner/repo\n");
        fprintf(stderr, "  gh:owner/repo\n");
        fprintf(stderr, "  owner/repo\n\n");
        return 1;
    }

    // Initialize settings and apply cache config
    settings_init();
    AppSettings app_settings;
    settings_load_app_settings(&app_settings);
    if (app_settings.clone_cache_size_gb > 0) {
        set_clone_cache_size(static_cast<size_t>(app_settings.clone_cache_size_gb) * 1024 * 1024 * 1024);
    }

    // Normalize URL: append .git if missing
    char url[512];
    strncpy_s(url, g_cli_args.clone_url, _TRUNCATE);
    size_t url_len = strlen(url);
    if (url_len > 0 && url_len < sizeof(url) - 5) {
        if (url_len < 4 || strcmp(url + url_len - 4, ".git") != 0) {
            strncat_s(url, ".git", sizeof(url) - url_len - 1);
        }
    }

    // Set up clone options
    CloneOptions options = {};
    strncpy_s(options.url, url, _TRUNCATE);
    wcsncpy_s(options.destination, g_cli_args.clone_dest, _TRUNCATE);
    options.checkout = true;

    // Set up progress tracking
    CloneProgress progress = {};
    CloneResult result = {};

    // Print initial message
    char repo_name[256] = {};
    git_clone_extract_repo_name(url, repo_name, sizeof(repo_name));
    printf("\nCloning %s into %ls...\n\n", repo_name[0] ? repo_name : url, g_cli_args.clone_dest);

    // Thread context for clone operation
    HeadlessCloneContext ctx = { &options, &progress, &result };

    // Start clone in background thread for progress display
    HANDLE clone_thread = CreateThread(nullptr, 0, headless_clone_thread_func, &ctx, 0, nullptr);

    // Check if ANSI escape codes are supported (Windows 10+ with virtual terminal processing)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    bool use_ansi = GetConsoleMode(hConsole, &dwMode) && SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Helper to format bytes
    auto format_bytes = [](uint64_t bytes, char* out, size_t out_size) {
        if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
            snprintf(out, out_size, "%.2fGB", bytes / (1024.0 * 1024.0 * 1024.0));
        } else if (bytes >= 1024ULL * 1024ULL) {
            snprintf(out, out_size, "%.1fMB", bytes / (1024.0 * 1024.0));
        } else if (bytes >= 1024ULL) {
            snprintf(out, out_size, "%.0fKB", bytes / 1024.0);
        } else {
            snprintf(out, out_size, "%lluB", bytes);
        }
    };

    // Helper to format counts (e.g., 11.3M)
    auto format_count = [](uint64_t count, char* out, size_t out_size) {
        if (count >= 1000000ULL) {
            snprintf(out, out_size, "%.1fM", count / 1000000.0);
        } else if (count >= 1000ULL) {
            snprintf(out, out_size, "%.1fK", count / 1000.0);
        } else {
            snprintf(out, out_size, "%llu", count);
        }
    };

    // Helper to format elapsed time
    auto format_elapsed = [](uint64_t ms, char* out, size_t out_size) {
        uint64_t s = ms / 1000;
        if (s >= 60) {
            snprintf(out, out_size, "%llum%02llus", s / 60, s % 60);
        } else {
            snprintf(out, out_size, "%llus", s);
        }
    };

    // Progress bar helper (10-char wide)
    auto render_progress_bar = [](float ratio, char* out, size_t out_size) {
        int filled = static_cast<int>(ratio * 10);
        if (filled > 10) filled = 10;
        char bar[12] = "          ";
        for (int i = 0; i < filled; i++) bar[i] = '#';
        snprintf(out, out_size, "[%s]", bar);
    };

    // Track displayed lines for ANSI refresh
    bool first_print = true;
    uint64_t last_speed_time = GetTickCount64();
    uint64_t last_speed_bytes = 0;
    double download_speed_mbps = 0.0;

    // Poll progress
    while (WaitForSingleObject(clone_thread, 100) == WAIT_TIMEOUT) {
        ClonePhase phase = progress.phase.load();
        FetchPhase fetch_phase = progress.fetch_phase.load();
        uint64_t now = GetTickCount64();

        // Update download speed
        // Note: 250ms minimum interval prevents division by zero and smooths speed display.
        // With 100ms poll interval, worst case is time_delta_ms=250ms which gives valid speed.
        uint64_t bytes_received = progress.bytes_received.load();
        if (now - last_speed_time >= 250) {
            uint64_t bytes_delta = bytes_received - last_speed_bytes;
            uint64_t time_delta_ms = now - last_speed_time;
            download_speed_mbps = (bytes_delta / (1024.0 * 1024.0)) / (time_delta_ms / 1000.0);
            last_speed_time = now;
            last_speed_bytes = bytes_received;
        }

        if (use_ansi) {
            // ANSI multi-line display
            // Move cursor up 4 lines to overwrite previous output (safe because we always print exactly 4 lines)
            if (!first_print) {
                printf("\033[4A");
            }
            first_print = false;

            // Download line
            {
                char icon = ' ';
                char line[128] = "";
                char elapsed[16] = "--";
                uint64_t start = progress.download_start_ms.load();
                uint64_t end = progress.download_end_ms.load();
                if (end > 0) {
                    icon = '+';  // Complete
                    format_elapsed(end - start, elapsed, sizeof(elapsed));
                    snprintf(line, sizeof(line), "Download - %s", elapsed);
                } else if (phase == ClonePhase::Fetching &&
                           (fetch_phase == FetchPhase::Receiving || fetch_phase == FetchPhase::Negotiating ||
                            fetch_phase == FetchPhase::Connecting || fetch_phase == FetchPhase::Authenticating)) {
                    icon = '>';  // Active
                    if (start > 0) format_elapsed(now - start, elapsed, sizeof(elapsed));
                    char bytes_str[32], total_str[32];
                    format_bytes(bytes_received, bytes_str, sizeof(bytes_str));
                    uint64_t bytes_total = progress.bytes_total.load();
                    if (bytes_total > 0) {
                        format_bytes(bytes_total, total_str, sizeof(total_str));
                        snprintf(line, sizeof(line), "Download: %s / %s (%.1f MB/s) %s", bytes_str, total_str, download_speed_mbps, elapsed);
                    } else {
                        snprintf(line, sizeof(line), "Download: %s (%.1f MB/s) %s", bytes_str, download_speed_mbps, elapsed);
                    }
                } else {
                    icon = ' ';  // Pending
                    snprintf(line, sizeof(line), "Download");
                }
                printf("[%c] %-60s\n", icon, line);
            }

            // Parse line
            {
                char icon = ' ';
                char line[128] = "";
                char elapsed[16] = "--";
                uint64_t start = progress.parse_start_ms.load();
                uint64_t end = progress.parse_end_ms.load();
                if (end > 0) {
                    icon = '+';
                    format_elapsed(end - start, elapsed, sizeof(elapsed));
                    snprintf(line, sizeof(line), "Parse - %s", elapsed);
                } else if (phase == ClonePhase::Fetching &&
                           (fetch_phase == FetchPhase::Receiving || fetch_phase == FetchPhase::Negotiating)) {
                    icon = '>';
                    if (start > 0) format_elapsed(now - start, elapsed, sizeof(elapsed));
                    char done_str[32], total_str[32];
                    uint64_t done = progress.objects_done.load();
                    uint64_t total = progress.objects_total.load();
                    format_count(done, done_str, sizeof(done_str));
                    if (total > 0) {
                        format_count(total, total_str, sizeof(total_str));
                        snprintf(line, sizeof(line), "Parse: %s / %s objects %s", done_str, total_str, elapsed);
                    } else {
                        snprintf(line, sizeof(line), "Parse: %s objects %s", done_str, elapsed);
                    }
                } else {
                    snprintf(line, sizeof(line), "Parse");
                }
                printf("[%c] %-60s\n", icon, line);
            }

            // Delta Resolution line
            {
                char icon = ' ';
                char line[128] = "";
                char elapsed[16] = "--";
                uint64_t start = progress.delta_start_ms.load();
                uint64_t end = progress.delta_end_ms.load();
                if (end > 0) {
                    icon = '+';
                    format_elapsed(end - start, elapsed, sizeof(elapsed));
                    snprintf(line, sizeof(line), "Delta Resolution - %s", elapsed);
                } else if (phase == ClonePhase::Fetching && fetch_phase == FetchPhase::ResolvingDeltas) {
                    icon = '>';
                    if (start > 0) format_elapsed(now - start, elapsed, sizeof(elapsed));
                    char done_str[32], total_str[32];
                    uint64_t done = progress.deltas_done.load();
                    uint64_t total = progress.deltas_total.load();
                    format_count(done, done_str, sizeof(done_str));
                    if (total > 0) {
                        format_count(total, total_str, sizeof(total_str));
                        snprintf(line, sizeof(line), "Delta Resolution: %s / %s %s", done_str, total_str, elapsed);
                    } else {
                        snprintf(line, sizeof(line), "Delta Resolution: %s %s", done_str, elapsed);
                    }
                } else {
                    snprintf(line, sizeof(line), "Delta Resolution");
                }
                printf("[%c] %-60s\n", icon, line);
            }

            // Checkout line
            {
                char icon = ' ';
                char line[128] = "";
                char elapsed[16] = "--";
                uint64_t start = progress.checkout_start_ms.load();
                uint64_t end = progress.checkout_end_ms.load();
                if (end > 0) {
                    icon = '+';
                    format_elapsed(end - start, elapsed, sizeof(elapsed));
                    snprintf(line, sizeof(line), "Checkout - %s", elapsed);
                } else if (phase == ClonePhase::CheckingOut) {
                    icon = '>';
                    if (start > 0) format_elapsed(now - start, elapsed, sizeof(elapsed));
                    char done_str[32], total_str[32];
                    uint64_t done = progress.files_done.load();
                    uint64_t total = progress.files_total.load();
                    format_count(done, done_str, sizeof(done_str));
                    if (total > 0) {
                        format_count(total, total_str, sizeof(total_str));
                        snprintf(line, sizeof(line), "Checkout: %s / %s files %s", done_str, total_str, elapsed);
                    } else {
                        snprintf(line, sizeof(line), "Checkout: %s files %s", done_str, elapsed);
                    }
                } else {
                    snprintf(line, sizeof(line), "Checkout");
                }
                printf("[%c] %-60s\n", icon, line);
            }
        } else {
            // Fallback: simple phase-based logging (no ANSI)
            static ClonePhase last_phase = ClonePhase::Idle;
            static FetchPhase last_fetch_phase = FetchPhase::Idle;

            if (phase != last_phase || (phase == ClonePhase::Fetching && fetch_phase != last_fetch_phase)) {
                const char* phase_text = "";
                switch (phase) {
                    case ClonePhase::Initializing:      phase_text = "Initializing..."; break;
                    case ClonePhase::AddingRemote:      phase_text = "Adding remote..."; break;
                    case ClonePhase::Fetching:
                        switch (fetch_phase) {
                            case FetchPhase::Connecting:      phase_text = "Connecting..."; break;
                            case FetchPhase::Authenticating:  phase_text = "Authenticating..."; break;
                            case FetchPhase::Negotiating:     phase_text = "Counting objects..."; break;
                            case FetchPhase::Receiving:       phase_text = "Receiving objects..."; break;
                            case FetchPhase::ResolvingDeltas: phase_text = "Resolving deltas..."; break;
                            case FetchPhase::Updating:        phase_text = "Updating refs..."; break;
                            default:                          phase_text = "Fetching..."; break;
                        }
                        break;
                    case ClonePhase::DeterminingBranch: phase_text = "Determining branch..."; break;
                    case ClonePhase::CheckingOut:       phase_text = "Checking out files..."; break;
                    default: break;
                }
                if (phase_text[0]) {
                    printf("%s\n", phase_text);
                }
                last_phase = phase;
                last_fetch_phase = fetch_phase;
            }
        }
    }

    // Final update with completion status
    if (use_ansi) {
        printf("\033[4A");  // Move cursor up 4 lines to overwrite
        // Print final status
        auto print_final_line = [&](const char* name, uint64_t start, uint64_t end, bool failed) {
            char elapsed[16] = "--";
            char icon = failed ? 'X' : '+';
            if (end > 0 && start > 0) {
                uint64_t ms = end - start;
                if (ms / 1000 >= 60) {
                    snprintf(elapsed, sizeof(elapsed), "%llum%02llus", ms / 1000 / 60, (ms / 1000) % 60);
                } else {
                    snprintf(elapsed, sizeof(elapsed), "%llus", ms / 1000);
                }
            }
            printf("[%c] %-20s %s\n", icon, name, elapsed);
        };

        bool failed = result.success ? false : true;
        print_final_line("Download", progress.download_start_ms.load(), progress.download_end_ms.load(), failed && progress.download_end_ms.load() == 0);
        print_final_line("Parse", progress.parse_start_ms.load(), progress.parse_end_ms.load(), failed && progress.parse_end_ms.load() == 0);
        print_final_line("Delta Resolution", progress.delta_start_ms.load(), progress.delta_end_ms.load(), failed && progress.delta_end_ms.load() == 0);
        print_final_line("Checkout", progress.checkout_start_ms.load(), progress.checkout_end_ms.load(), failed && progress.checkout_end_ms.load() == 0);
    }

    CloseHandle(clone_thread);

    // Print result
    if (result.success) {
        printf("Clone completed successfully.\n");
        printf("  Branch: %s\n", result.default_branch[0] ? result.default_branch : "main");
        printf("  Objects: %zu\n", result.objects_received);
        printf("  Files: %zu\n", result.files_checked_out);
        printf("  Duration: %.1fs\n\n", result.duration_ms / 1000.0);
        return 0;
    } else {
        fprintf(stderr, "Clone failed: %s\n\n", result.error_message);
        return 1;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    parse_command_line_args();

    // Handle --help
    if (g_cli_args.mode == CommandLineArgs::Mode::Help) {
        show_help();
        return 0;
    }

    // Handle headless clone mode
    if (g_cli_args.mode == CommandLineArgs::Mode::Clone && g_cli_args.headless) {
        return run_headless_clone();
    }

    // Normal GUI mode (with or without clone args)
    return app_run(hInstance, nCmdShow);
}
