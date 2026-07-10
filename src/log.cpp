// log.cpp - Category-based logging system with telemetry ring buffer
//
// Features:
// - Compile-time zero-overhead for disabled categories (via constexpr mask)
// - Runtime filtering by category and subsystem
// - In-memory telemetry ring buffer (always-on, dump on demand)
// - Session-based log files (no unbounded growth)
// - Structured telemetry API (type-safe, no string parsing)

#include "app.h"
#include "log_types.h"
#include <cstdarg>
#include <cstdio>

// Maximum number of log files to keep
constexpr int MAX_LOG_FILES = 10;

// Clean up old log files, keeping only the most recent MAX_LOG_FILES
static void cleanup_old_log_files(const char* temp_path) {
    char search_pattern[MAX_PATH];
    snprintf(search_pattern, MAX_PATH, "%sGitSmarter_*.log", temp_path);

    // Collect all matching log files
    char files[64][MAX_PATH];  // Support up to 64 files
    int file_count = 0;

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && file_count < 64) {
            snprintf(files[file_count], MAX_PATH, "%s%s", temp_path, fd.cFileName);
            file_count++;
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    // If we have more than MAX_LOG_FILES, delete the oldest ones
    if (file_count <= MAX_LOG_FILES) return;

    // Sort files by name (alphabetical = chronological due to timestamp format)
    // Using bubble sort for simplicity since file_count is small
    for (int i = 0; i < file_count - 1; i++) {
        for (int j = 0; j < file_count - i - 1; j++) {
            if (strcmp(files[j], files[j + 1]) > 0) {
                char temp[MAX_PATH];
                strcpy_s(temp, sizeof(temp), files[j]);
                strcpy_s(files[j], sizeof(files[j]), files[j + 1]);
                strcpy_s(files[j + 1], sizeof(files[j + 1]), temp);
            }
        }
    }

    // Delete oldest files (keeping the last MAX_LOG_FILES)
    int files_to_delete = file_count - MAX_LOG_FILES;
    for (int i = 0; i < files_to_delete; i++) {
        DeleteFileA(files[i]);
    }
}

// Logger state (file-scoped static)
static struct {
    std::atomic<uint32_t> category_mask{static_cast<uint32_t>(Log::Category::All)};
    std::atomic<uint32_t> subsystem_mask{static_cast<uint32_t>(Log::Subsystem::All)};
    Log::TelemetryRing telemetry;

    CRITICAL_SECTION file_lock;
    FILE* log_file = nullptr;
    bool initialized = false;
    bool file_enabled = true;

    char session_path[MAX_PATH];
    LARGE_INTEGER perf_freq;
    uint64_t session_start_us;
} g_log;

// Initialize the logging system
bool log_system_init() {
    if (g_log.initialized) return true;

    InitializeCriticalSection(&g_log.file_lock);
    QueryPerformanceFrequency(&g_log.perf_freq);

    // Initialize telemetry ring buffer
    g_log.telemetry.init();

    // Session-based filename: GitSmarter_YYYYMMDD_HHMMSS_PID.log
    SYSTEMTIME st;
    GetLocalTime(&st);
    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    snprintf(g_log.session_path, MAX_PATH,
        "%sGitSmarter_%04d%02d%02d_%02d%02d%02d_%lu.log",
        temp, st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, GetCurrentProcessId());

    if (g_log.file_enabled) {
        fopen_s(&g_log.log_file, g_log.session_path, "w");
    }

    g_log.session_start_us = log_get_timestamp_us();
    g_log.initialized = true;

    // Write session header
    if (g_log.log_file) {
        fprintf(g_log.log_file, "=== GitSmarter Log Session ===\n");
        fprintf(g_log.log_file, "Started: %04d-%02d-%02d %02d:%02d:%02d\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        fprintf(g_log.log_file, "PID: %lu\n", GetCurrentProcessId());
        fprintf(g_log.log_file, "================================\n\n");
        fflush(g_log.log_file);
    }

    // Clean up old log files (keep only the last 10)
    cleanup_old_log_files(temp);

    return g_log.log_file != nullptr || !g_log.file_enabled;
}

// Shutdown the logging system
void log_system_shutdown() {
    if (!g_log.initialized) return;

    EnterCriticalSection(&g_log.file_lock);
    if (g_log.log_file) {
        fprintf(g_log.log_file, "\n=== Session End ===\n");
        fclose(g_log.log_file);
        g_log.log_file = nullptr;
    }
    LeaveCriticalSection(&g_log.file_lock);

    // Shutdown telemetry ring buffer
    g_log.telemetry.shutdown();

    DeleteCriticalSection(&g_log.file_lock);
    g_log.initialized = false;
}

// Get high-precision timestamp in microseconds
uint64_t log_get_timestamp_us() {
    // Auto-initialize if needed (perf_freq is 0 when uninitialized)
    if (g_log.perf_freq.QuadPart == 0) {
        QueryPerformanceFrequency(&g_log.perf_freq);
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    // Compute (now * 1e6) / freq without overflowing int64.
    // At a 10 MHz QPC freq, `now * 1e6` overflows int64 after ~10 days uptime.
    int64_t freq = g_log.perf_freq.QuadPart;
    if (freq <= 0) return 0;
    int64_t secs = now.QuadPart / freq;
    int64_t frac = now.QuadPart % freq;
    return static_cast<uint64_t>(secs) * 1000000ULL +
           static_cast<uint64_t>((frac * 1000000) / freq);
}

// Runtime check if category is enabled
bool log_category_enabled_runtime(Log::Category cat) {
    return (g_log.category_mask.load(std::memory_order_relaxed) &
            static_cast<uint32_t>(cat)) != 0;
}

// Runtime check if subsystem is enabled
bool log_subsystem_enabled_runtime(Log::Subsystem sub) {
    return (g_log.subsystem_mask.load(std::memory_order_relaxed) &
            static_cast<uint32_t>(sub)) != 0;
}

// Set category filter mask
void log_set_category_mask(uint32_t mask) {
    g_log.category_mask.store(mask, std::memory_order_relaxed);
}

// Set subsystem filter mask
void log_set_subsystem_mask(uint32_t mask) {
    g_log.subsystem_mask.store(mask, std::memory_order_relaxed);
}

// Get current category mask
uint32_t log_get_category_mask() {
    return g_log.category_mask.load(std::memory_order_relaxed);
}

// Get current subsystem mask
uint32_t log_get_subsystem_mask() {
    return g_log.subsystem_mask.load(std::memory_order_relaxed);
}

// Enable/disable file logging
void log_set_file_enabled(bool enabled) {
    g_log.file_enabled = enabled;
}

// Get session log file path
const char* log_get_session_path() {
    return g_log.session_path;
}

// Category name lookup
static const char* category_name(Log::Category cat) {
    switch (cat) {
        case Log::Category::Error:       return "ERROR";
        case Log::Category::Warning:     return "WARN";
        case Log::Category::Info:        return "INFO";
        case Log::Category::Debug:       return "DEBUG";
        case Log::Category::Performance: return "PERF";
        case Log::Category::Telemetry:   return "TELEM";
        case Log::Category::Network:     return "NET";
        case Log::Category::Git:         return "GIT";
        case Log::Category::UI:          return "UI";
        default:                         return "?";
    }
}

// Write a log message
void log_write_impl(Log::Category cat, const char* subsystem, const char* fmt, ...) {
    if (!g_log.initialized) log_system_init();
    if (!g_log.log_file && g_log.file_enabled) return;

    // Format OUTSIDE the lock so concurrent workers don't serialize on vsnprintf.
    // Only the final fputs needs the critical section.
    char buffer[2048];
    va_list args;
    va_start(args, fmt);

    // Format: [Elapsed_ms][Category][Subsystem?][ThreadID] Message
    uint64_t elapsed_us = log_get_timestamp_us() - g_log.session_start_us;
    uint64_t elapsed_ms = elapsed_us / 1000;

    int prefix_len;
    if (subsystem && subsystem[0]) {
        prefix_len = snprintf(buffer, sizeof(buffer), "[%llu][%s][%s][T%lu] ",
                              elapsed_ms, category_name(cat), subsystem, GetCurrentThreadId());
    } else {
        prefix_len = snprintf(buffer, sizeof(buffer), "[%llu][%s][T%lu] ",
                              elapsed_ms, category_name(cat), GetCurrentThreadId());
    }

    // Clamp prefix_len so the remaining-space subtraction below can't underflow
    // (size_t) when an absurdly long subsystem name produces a prefix bigger
    // than the buffer.
    if (prefix_len < 0) prefix_len = 0;
    if (prefix_len > (int)sizeof(buffer) - 2) prefix_len = (int)sizeof(buffer) - 2;

    int msg_len = vsnprintf(buffer + prefix_len, sizeof(buffer) - prefix_len - 2, fmt, args);
    va_end(args);

    if (msg_len < 0) msg_len = 0;
    // vsnprintf with truncation returns the would-have-been length; clamp to actual
    // bytes written so the newline write stays in bounds.
    int max_msg = (int)sizeof(buffer) - prefix_len - 2;
    if (msg_len > max_msg) msg_len = max_msg;

    // Add newline
    int total_len = prefix_len + msg_len;
    if (total_len >= 0 && total_len < (int)sizeof(buffer) - 1) {
        buffer[total_len] = '\n';
        buffer[total_len + 1] = '\0';
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    // Write to file. Don't fflush on every call — with 32 delta-worker threads
    // logging concurrently, that adds a kernel-mode write per LOG and serializes
    // all workers on the file lock. CRT line buffering + fflush at shutdown is
    // sufficient. Errors still flush, see log_flush() callers.
    if (g_log.log_file) {
        EnterCriticalSection(&g_log.file_lock);
        fputs(buffer, g_log.log_file);
        if (cat == Log::Category::Error) {
            // Make sure errors hit disk in case we crash next.
            fflush(g_log.log_file);
        }
        LeaveCriticalSection(&g_log.file_lock);
    }
}

// Push telemetry entry to ring buffer
void log_telemetry_impl(const Log::TelemetryEntry& entry) {
    g_log.telemetry.push(entry);
}

// Get telemetry ring buffer for inspection
Log::TelemetryRing* log_get_telemetry_ring() {
    return &g_log.telemetry;
}

// Dump telemetry to JSON file
void log_dump_telemetry_json(const char* path) {
    g_log.telemetry.dump_to_json(path);
}

// TelemetryRing implementation
void Log::TelemetryRing::init() {
    if (!entries) {
        entries = new (std::nothrow) TelemetryEntry[TELEMETRY_RING_SIZE];
        if (entries) {
            memset(entries, 0, sizeof(TelemetryEntry) * TELEMETRY_RING_SIZE);
        }
    }
}

void Log::TelemetryRing::shutdown() {
    delete[] entries;
    entries = nullptr;
}

bool Log::TelemetryRing::push(const TelemetryEntry& entry) {
    if (!entries) return false;
    uint64_t write_pos = write_index.fetch_add(1, std::memory_order_relaxed);

    // Check if we're overwriting unread data
    uint64_t read_pos = read_index.load(std::memory_order_relaxed);
    if (write_pos >= read_pos + TELEMETRY_RING_SIZE) {
        overflow_count.fetch_add(1, std::memory_order_relaxed);
    }

    size_t slot = write_pos % TELEMETRY_RING_SIZE;
    entries[slot] = entry;
    return true;
}

uint64_t Log::TelemetryRing::get_overflow_count() const {
    return overflow_count.load(std::memory_order_relaxed);
}

size_t Log::TelemetryRing::count() const {
    uint64_t write_pos = write_index.load(std::memory_order_relaxed);
    uint64_t read_pos = read_index.load(std::memory_order_relaxed);
    uint64_t diff = write_pos - read_pos;
    return (diff > TELEMETRY_RING_SIZE) ? TELEMETRY_RING_SIZE : static_cast<size_t>(diff);
}

void Log::TelemetryRing::clear() {
    uint64_t write_pos = write_index.load(std::memory_order_relaxed);
    read_index.store(write_pos, std::memory_order_relaxed);
}

// Event type name for JSON output
static const char* telemetry_event_name(Log::TelemetryEvent ev) {
    switch (ev) {
        case Log::TelemetryEvent::ClonePhase:      return "clone_phase";
        case Log::TelemetryEvent::FetchOperation:  return "fetch_op";
        case Log::TelemetryEvent::CheckoutPhase:   return "checkout_phase";
        case Log::TelemetryEvent::PackParsing:     return "pack_parsing";
        case Log::TelemetryEvent::DeltaResolution: return "delta_resolution";
        case Log::TelemetryEvent::ObjectCache:     return "object_cache";
        case Log::TelemetryEvent::SHA1Stats:       return "sha1_stats";
        case Log::TelemetryEvent::HTTPTransfer:    return "http_transfer";
        case Log::TelemetryEvent::WorkerJob:       return "worker_job";
        default:                                   return "unknown";
    }
}

// Clone phase name for JSON output
static const char* clone_phase_name(Log::ClonePhase phase) {
    switch (phase) {
        case Log::ClonePhase::Initializing:      return "initializing";
        case Log::ClonePhase::AddingRemote:      return "adding_remote";
        case Log::ClonePhase::Fetching:          return "fetching";
        case Log::ClonePhase::DeterminingBranch: return "determining_branch";
        case Log::ClonePhase::CheckingOut:       return "checking_out";
        case Log::ClonePhase::Complete:          return "complete";
        default:                                 return "unknown";
    }
}

// Fetch operation name for JSON output
static const char* fetch_op_name(Log::FetchOp op) {
    switch (op) {
        case Log::FetchOp::ServerDiscovery: return "server_discovery";
        case Log::FetchOp::ListRefs:        return "list_refs";
        case Log::FetchOp::FetchPack:       return "fetch_pack";
        case Log::FetchOp::InstallPack:     return "install_pack";
        default:                            return "unknown";
    }
}

void Log::TelemetryRing::dump_to_json(const char* path) {
    if (!entries) return;

    FILE* f = nullptr;
    fopen_s(&f, path, "w");
    if (!f) return;

    fprintf(f, "{\n  \"telemetry\": [\n");

    uint64_t write_pos = write_index.load(std::memory_order_relaxed);
    uint64_t start_pos = (write_pos > TELEMETRY_RING_SIZE) ? (write_pos - TELEMETRY_RING_SIZE) : 0;

    bool first = true;
    for (uint64_t i = start_pos; i < write_pos; i++) {
        size_t slot = i % TELEMETRY_RING_SIZE;
        const TelemetryEntry& e = entries[slot];

        if (!first) fprintf(f, ",\n");
        first = false;

        fprintf(f, "    {\n");
        fprintf(f, "      \"timestamp_us\": %llu,\n", e.timestamp_us);
        fprintf(f, "      \"event_type\": \"%s\",\n", telemetry_event_name(e.event_type));
        fprintf(f, "      \"thread_id\": %lu,\n", e.thread_id);

        // Write event-specific data
        switch (e.event_type) {
            case TelemetryEvent::ClonePhase:
                fprintf(f, "      \"phase\": \"%s\",\n", clone_phase_name(static_cast<ClonePhase>(e.data.clone.phase)));
                fprintf(f, "      \"elapsed_ms\": %u,\n", e.data.clone.elapsed_ms);
                fprintf(f, "      \"objects\": %llu,\n", e.data.clone.objects);
                fprintf(f, "      \"bytes\": %llu\n", e.data.clone.bytes);
                break;

            case TelemetryEvent::FetchOperation:
                fprintf(f, "      \"operation\": \"%s\",\n", fetch_op_name(static_cast<FetchOp>(e.data.fetch.operation)));
                fprintf(f, "      \"elapsed_ms\": %u,\n", e.data.fetch.elapsed_ms);
                fprintf(f, "      \"refs\": %u,\n", e.data.fetch.refs);
                fprintf(f, "      \"status\": %u,\n", e.data.fetch.status);
                fprintf(f, "      \"pack_size\": %llu\n", e.data.fetch.pack_size);
                break;

            case TelemetryEvent::PackParsing:
                fprintf(f, "      \"commits\": %u,\n", e.data.pack_parsing.commits);
                fprintf(f, "      \"trees\": %u,\n", e.data.pack_parsing.trees);
                fprintf(f, "      \"blobs\": %u,\n", e.data.pack_parsing.blobs);
                fprintf(f, "      \"tags\": %u,\n", e.data.pack_parsing.tags);
                fprintf(f, "      \"ofs_delta\": %u,\n", e.data.pack_parsing.ofs_delta);
                fprintf(f, "      \"ref_delta\": %u\n", e.data.pack_parsing.ref_delta);
                break;

            case TelemetryEvent::DeltaResolution:
                fprintf(f, "      \"max_depth\": %u,\n", e.data.delta.max_depth);
                fprintf(f, "      \"resolved\": %u,\n", e.data.delta.resolved);
                fprintf(f, "      \"delta_apply_us\": %llu,\n", e.data.delta.delta_apply_us);
                fprintf(f, "      \"sha_compute_us\": %llu\n", e.data.delta.sha_compute_us);
                break;

            case TelemetryEvent::ObjectCache:
                fprintf(f, "      \"hits\": %u,\n", e.data.cache.hits);
                fprintf(f, "      \"misses\": %u,\n", e.data.cache.misses);
                fprintf(f, "      \"evictions\": %u,\n", e.data.cache.evictions);
                fprintf(f, "      \"puts\": %u,\n", e.data.cache.puts);
                fprintf(f, "      \"evicted_bytes\": %llu\n", e.data.cache.evicted_bytes);
                break;

            default:
                fprintf(f, "      \"raw\": true\n");
                break;
        }

        fprintf(f, "    }");
    }

    fprintf(f, "\n  ]\n}\n");
    fclose(f);
}
