// platform_worker.cpp - Worker thread for async operations (fetch, push, pull, checkout)
// Part of GitSmarter platform layer

#include "app.h"
#include "ui_theme.h"
#include <shobjidl.h>
#include <ctime>
#include <cmath>

// Forward declarations for globals defined in platform_window.cpp
extern HWND g_main_window;
extern struct GitRepository g_repo;

// Worker thread for async operations (fetch, push, pull, checkout)
struct WorkerThreadState {
    HANDLE thread = nullptr;
    HANDLE work_event = nullptr;      // Signaled when work is available
    HANDLE shutdown_event = nullptr;  // Signaled to shutdown thread

    // Current job
    enum class JobType { None, Fetch, Push, Pull, Checkout, Clone } job_type = JobType::None;
    char remote_name[64] = {0};
    bool silent = false;              // True for background sync (no message box)

    // Fetch progress tracking (shared with main thread via atomics)
    FetchProgress progress;
    FetchResult result;

    // Push-specific fields
    char push_ref[Git::MAX_REF_NAME] = {0};
    bool push_force = false;
    PushProgress push_progress;
    PushResult push_result;

    // Pull-specific fields
    PullProgress pull_progress;
    PullResult pull_result;

    // Checkout-specific fields
    char checkout_ref_name[Git::MAX_REF_NAME] = {0};
    GitRef::Type checkout_ref_type = GitRef::Type::Local;
    char checkout_sha[Git::SHA1_HEX_SIZE + 1] = {0};
    bool checkout_create_tracking = false;
    char checkout_new_branch[Git::MAX_REF_NAME] = {0};
    CheckoutProgress checkout_progress;
    CheckoutResult checkout_result;

    // Clone-specific fields
    char clone_url[Remote::MAX_URL] = {0};
    wchar_t clone_destination[Git::MAX_PATH_LEN] = {0};
    CloneProgress clone_progress;
    CloneResult clone_result;

    // Speed calculation
    uint64_t speed_samples[10] = {0};  // Rolling window of bytes/sec
    int speed_sample_index = 0;
    int64_t last_speed_time = 0;
    uint64_t last_speed_bytes = 0;
};

static WorkerThreadState g_worker = {};
static CRITICAL_SECTION g_worker_lock;  // Protects g_worker.job_type, remote_name, silent

// Timer IDs for progress updates are now in app.h

// Forward declarations
static DWORD WINAPI worker_thread_proc(LPVOID param);

// Worker thread implementation
static DWORD WINAPI worker_thread_proc(LPVOID param) {
    (void)param;  // Required by WINAPI signature
    HANDLE events[2] = { g_worker.shutdown_event, g_worker.work_event };

    while (true) {
        // Wait for work or shutdown
        DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (result == WAIT_OBJECT_0) {
            // Shutdown requested
            break;
        }

        if (result == WAIT_OBJECT_0 + 1) {
            // Work available - read job parameters under lock
            WorkerThreadState::JobType job;
            char remote_name[64];

            char push_ref[Git::MAX_REF_NAME];
            bool push_force;

            EnterCriticalSection(&g_worker_lock);
            job = g_worker.job_type;
            strncpy_s(remote_name, g_worker.remote_name, sizeof(remote_name) - 1);
            strncpy_s(push_ref, g_worker.push_ref, sizeof(push_ref) - 1);
            push_force = g_worker.push_force;
            LeaveCriticalSection(&g_worker_lock);

            if (job == WorkerThreadState::JobType::Fetch) {
                // Reset progress
                g_worker.progress.phase.store(FetchPhase::Connecting);
                g_worker.progress.objects_done.store(0);
                g_worker.progress.objects_total.store(0);
                g_worker.progress.bytes_received.store(0);
                g_worker.progress.bytes_total.store(0);
                g_worker.progress.base.cancel_requested.store(false);
                g_worker.progress.base.start_time_ms.store(GetTickCount64());
                g_worker.progress.base.generation.fetch_add(1);

                // Perform fetch
                FetchOptions options = {};
                options.progress_callback = [](const FetchProgress* progress, void*) {
                    // Copy progress to worker state
                    g_worker.progress.phase.store(progress->phase.load());
                    g_worker.progress.objects_done.store(progress->objects_done.load());
                    g_worker.progress.objects_total.store(progress->objects_total.load());
                    g_worker.progress.bytes_received.store(progress->bytes_received.load());
                    g_worker.progress.bytes_total.store(progress->bytes_total.load());
                    // Increment generation to signal update completion
                    g_worker.progress.base.generation.fetch_add(1);
                };

                bool success;
                {
                    RepoLockGuard lock;
                    success = git_fetch(&g_repo, remote_name, &options, &g_worker.result);
                }
                g_worker.result.success = success;

                // Signal completion to main thread
                constexpr UINT WM_FETCH_COMPLETE = WM_USER + 100;
                EnterCriticalSection(&g_worker_lock);
                g_worker.job_type = WorkerThreadState::JobType::None;
                LeaveCriticalSection(&g_worker_lock);
                PostMessageW(g_main_window, WM_FETCH_COMPLETE, success ? 1 : 0, 0);
            }
            else if (job == WorkerThreadState::JobType::Push) {
                // Reset push progress
                g_worker.push_progress.phase.store(PushPhase::Connecting);
                g_worker.push_progress.objects_done.store(0);
                g_worker.push_progress.objects_total.store(0);
                g_worker.push_progress.bytes_sent.store(0);
                g_worker.push_progress.bytes_total.store(0);
                g_worker.push_progress.base.cancel_requested.store(false);
                g_worker.push_progress.base.start_time_ms.store(GetTickCount64());
                g_worker.push_progress.base.generation.fetch_add(1);

                // Perform push
                PushOptions options = {};
                options.force = push_force;
                options.progress_callback = [](const PushProgress* progress, void*) {
                    // Copy progress to worker state
                    g_worker.push_progress.phase.store(progress->phase.load());
                    g_worker.push_progress.objects_done.store(progress->objects_done.load());
                    g_worker.push_progress.objects_total.store(progress->objects_total.load());
                    g_worker.push_progress.bytes_sent.store(progress->bytes_sent.load());
                    g_worker.push_progress.bytes_total.store(progress->bytes_total.load());
                    // Increment generation to signal update completion
                    g_worker.push_progress.base.generation.fetch_add(1);
                };

                bool success;
                {
                    RepoLockGuard lock;
                    success = git_push(&g_repo, remote_name, push_ref,
                                      &options, &g_worker.push_result);
                }
                g_worker.push_result.success = success;

                // Signal completion to main thread
                constexpr UINT WM_PUSH_COMPLETE = WM_USER + 101;
                EnterCriticalSection(&g_worker_lock);
                g_worker.job_type = WorkerThreadState::JobType::None;
                LeaveCriticalSection(&g_worker_lock);
                PostMessageW(g_main_window, WM_PUSH_COMPLETE, success ? 1 : 0, 0);
            }
            else if (job == WorkerThreadState::JobType::Pull) {
                // Reset pull progress
                g_worker.pull_progress.phase.store(PullPhase::Fetching);
                g_worker.pull_progress.files_done.store(0);
                g_worker.pull_progress.files_total.store(0);
                g_worker.pull_progress.fetch_phase.store(FetchPhase::Idle);
                g_worker.pull_progress.fetch_bytes.store(0);
                g_worker.pull_progress.base.cancel_requested.store(false);
                g_worker.pull_progress.base.start_time_ms.store(GetTickCount64());
                g_worker.pull_progress.base.generation.fetch_add(1);

                // Perform pull
                PullOptions options = {};
                options.progress_callback = [](const PullProgress* progress, void*) {
                    // Copy progress to worker state
                    g_worker.pull_progress.phase.store(progress->phase.load());
                    g_worker.pull_progress.files_done.store(progress->files_done.load());
                    g_worker.pull_progress.files_total.store(progress->files_total.load());
                    g_worker.pull_progress.fetch_phase.store(progress->fetch_phase.load());
                    g_worker.pull_progress.fetch_bytes.store(progress->fetch_bytes.load());
                    // Increment generation to signal update completion
                    g_worker.pull_progress.base.generation.fetch_add(1);
                };

                bool success;
                {
                    RepoLockGuard lock;
                    success = git_pull(&g_repo, remote_name, &options, &g_worker.pull_result);
                }
                g_worker.pull_result.success = success;

                // Signal completion to main thread
                constexpr UINT WM_PULL_COMPLETE = WM_USER + 102;
                EnterCriticalSection(&g_worker_lock);
                g_worker.job_type = WorkerThreadState::JobType::None;
                LeaveCriticalSection(&g_worker_lock);
                PostMessageW(g_main_window, WM_PULL_COMPLETE, success ? 1 : 0, 0);
            }
            else if (job == WorkerThreadState::JobType::Checkout) {
                // Reset checkout progress
                g_worker.checkout_progress.phase.store(CheckoutPhase::CheckingWorkdir);
                g_worker.checkout_progress.files_done.store(0);
                g_worker.checkout_progress.files_total.store(0);
                g_worker.checkout_progress.base.cancel_requested.store(false);
                g_worker.checkout_progress.base.start_time_ms.store(GetTickCount64());
                g_worker.checkout_progress.base.generation.fetch_add(1);

                // Build GitRef from stored data
                GitRef ref = {};
                strncpy_s(ref.name, g_worker.checkout_ref_name, sizeof(ref.name) - 1);
                strncpy_s(ref.sha, g_worker.checkout_sha, sizeof(ref.sha) - 1);
                ref.type = g_worker.checkout_ref_type;

                // Build options
                CheckoutOptions options = {};
                options.create_branch = g_worker.checkout_create_tracking;
                options.new_branch_name = g_worker.checkout_new_branch[0] ?
                                          g_worker.checkout_new_branch : nullptr;
                options.progress_callback = [](const CheckoutProgress* progress, void*) {
                    // Copy progress to worker state
                    g_worker.checkout_progress.phase.store(progress->phase.load());
                    g_worker.checkout_progress.files_done.store(progress->files_done.load());
                    g_worker.checkout_progress.files_total.store(progress->files_total.load());
                    // Increment generation to signal update completion
                    g_worker.checkout_progress.base.generation.fetch_add(1);
                };

                bool success;
                {
                    RepoLockGuard lock;
                    success = git_branch_checkout(&g_repo, &ref, &options, &g_worker.checkout_result);
                }
                g_worker.checkout_result.success = success;

                // Signal completion to main thread
                constexpr UINT WM_CHECKOUT_COMPLETE = WM_USER + 104;
                EnterCriticalSection(&g_worker_lock);
                g_worker.job_type = WorkerThreadState::JobType::None;
                LeaveCriticalSection(&g_worker_lock);
                PostMessageW(g_main_window, WM_CHECKOUT_COMPLETE, success ? 1 : 0, 0);
            }
            else if (job == WorkerThreadState::JobType::Clone) {
                // Reset clone progress
                g_worker.clone_progress.phase.store(ClonePhase::Initializing);
                g_worker.clone_progress.objects_done.store(0);
                g_worker.clone_progress.objects_total.store(0);
                g_worker.clone_progress.bytes_received.store(0);
                g_worker.clone_progress.files_done.store(0);
                g_worker.clone_progress.files_total.store(0);
                g_worker.clone_progress.fetch_phase.store(FetchPhase::Idle);
                g_worker.clone_progress.base.cancel_requested.store(false);
                int64_t clone_start = GetTickCount64();
                g_worker.clone_progress.base.start_time_ms.store(clone_start);
                g_worker.clone_progress.base.generation.fetch_add(1);
                
                LOG("worker_telemetry: job=clone status=start url=%s destination=...", g_worker.clone_url);

                // Build clone options
                CloneOptions options = {};
                strncpy_s(options.url, g_worker.clone_url, sizeof(options.url) - 1);
                wcsncpy_s(options.destination, g_worker.clone_destination, _countof(options.destination) - 1);
                options.checkout = true;
                options.progress_callback = [](const CloneProgress* progress, void*) {
                    // Copy progress to worker state
                    g_worker.clone_progress.phase.store(progress->phase.load());
                    g_worker.clone_progress.fetch_phase.store(progress->fetch_phase.load());
                    g_worker.clone_progress.objects_done.store(progress->objects_done.load());
                    g_worker.clone_progress.objects_total.store(progress->objects_total.load());
                    g_worker.clone_progress.bytes_received.store(progress->bytes_received.load());
                    g_worker.clone_progress.files_done.store(progress->files_done.load());
                    g_worker.clone_progress.files_total.store(progress->files_total.load());
                    // Increment generation to signal update completion
                    g_worker.clone_progress.base.generation.fetch_add(1);
                };

                // Perform clone (no repo lock needed - we're creating a new repo)
                bool success = git_clone(&options, &g_worker.clone_progress, &g_worker.clone_result);
                g_worker.clone_result.success = success;
                
                int64_t clone_elapsed = GetTickCount64() - clone_start;
                LOG("worker_telemetry: job=clone status=complete elapsed=%lldms success=%d objects=%zu bytes=%zu files=%zu branch=%s",
                    clone_elapsed, success ? 1 : 0, g_worker.clone_result.objects_received,
                    g_worker.clone_result.bytes_received, g_worker.clone_result.files_checked_out,
                    g_worker.clone_result.default_branch);

                // Signal completion to main thread
                constexpr UINT WM_CLONE_COMPLETE = WM_USER + 106;
                EnterCriticalSection(&g_worker_lock);
                g_worker.job_type = WorkerThreadState::JobType::None;
                LeaveCriticalSection(&g_worker_lock);
                PostMessageW(g_main_window, WM_CLONE_COMPLETE, success ? 1 : 0, 0);
            }
        }
    }

    return 0;
}

// Start async fetch operation
bool worker_start_async_fetch(const char* remote_name, bool silent) {
    if (!remote_name) return false;
    EnterCriticalSection(&g_worker_lock);

    // Check if already busy
    if (g_worker.job_type != WorkerThreadState::JobType::None) {
        LeaveCriticalSection(&g_worker_lock);
        return false;
    }

    // Set up job
    strncpy_s(g_worker.remote_name, remote_name, sizeof(g_worker.remote_name) - 1);
    g_worker.job_type = WorkerThreadState::JobType::Fetch;
    g_worker.silent = silent;

    LeaveCriticalSection(&g_worker_lock);

    // Start progress timer (only show progress UI for manual fetches)
    if (!silent) {
        SetTimer(g_main_window, FETCH_PROGRESS_TIMER_ID, FETCH_PROGRESS_INTERVAL_MS, nullptr);
    }

    // Signal worker thread
    SetEvent(g_worker.work_event);

    return true;
}

// Start async push operation
bool worker_start_async_push(const char* remote_name, const char* ref_name, bool force) {
    if (!remote_name || !ref_name) return false;
    EnterCriticalSection(&g_worker_lock);

    // Check if already busy
    if (g_worker.job_type != WorkerThreadState::JobType::None) {
        LeaveCriticalSection(&g_worker_lock);
        return false;
    }

    // Set up job
    strncpy_s(g_worker.remote_name, remote_name, sizeof(g_worker.remote_name) - 1);
    strncpy_s(g_worker.push_ref, ref_name, sizeof(g_worker.push_ref) - 1);
    g_worker.push_force = force;
    g_worker.job_type = WorkerThreadState::JobType::Push;
    g_worker.silent = false;  // Always show push progress

    LeaveCriticalSection(&g_worker_lock);

    // Start progress timer
    SetTimer(g_main_window, PUSH_PROGRESS_TIMER_ID, FETCH_PROGRESS_INTERVAL_MS, nullptr);

    // Signal worker thread
    SetEvent(g_worker.work_event);

    return true;
}

// Start async pull operation
bool worker_start_async_pull(const char* remote_name) {
    if (!remote_name) return false;
    EnterCriticalSection(&g_worker_lock);

    // Check if already busy
    if (g_worker.job_type != WorkerThreadState::JobType::None) {
        LeaveCriticalSection(&g_worker_lock);
        return false;
    }

    // Set up job
    strncpy_s(g_worker.remote_name, remote_name, sizeof(g_worker.remote_name) - 1);
    g_worker.job_type = WorkerThreadState::JobType::Pull;
    g_worker.silent = false;  // Always show pull progress

    LeaveCriticalSection(&g_worker_lock);

    // Start progress timer
    SetTimer(g_main_window, PULL_PROGRESS_TIMER_ID, FETCH_PROGRESS_INTERVAL_MS, nullptr);

    // Signal worker thread
    SetEvent(g_worker.work_event);

    return true;
}

// Start async checkout operation
bool worker_start_async_checkout(const GitRef* ref, bool create_tracking, const char* new_branch_name) {
    if (!ref) return false;

    EnterCriticalSection(&g_worker_lock);

    // Check if already busy
    if (g_worker.job_type != WorkerThreadState::JobType::None) {
        LeaveCriticalSection(&g_worker_lock);
        return false;
    }

    // Set up job
    strncpy_s(g_worker.checkout_ref_name, ref->name, sizeof(g_worker.checkout_ref_name) - 1);
    strncpy_s(g_worker.checkout_sha, ref->sha, sizeof(g_worker.checkout_sha) - 1);
    g_worker.checkout_ref_type = ref->type;
    g_worker.checkout_create_tracking = create_tracking;
    if (new_branch_name && new_branch_name[0]) {
        strncpy_s(g_worker.checkout_new_branch, new_branch_name, sizeof(g_worker.checkout_new_branch) - 1);
    } else {
        g_worker.checkout_new_branch[0] = '\0';
    }
    g_worker.job_type = WorkerThreadState::JobType::Checkout;

    LeaveCriticalSection(&g_worker_lock);

    // Start progress timer
    SetTimer(g_main_window, CHECKOUT_PROGRESS_TIMER_ID, FETCH_PROGRESS_INTERVAL_MS, nullptr);

    // Signal worker thread
    SetEvent(g_worker.work_event);

    return true;
}

// Start async clone operation
bool worker_start_async_clone(const char* url, const wchar_t* destination) {
    if (!url || !destination) return false;

    EnterCriticalSection(&g_worker_lock);

    // Check if already busy
    if (g_worker.job_type != WorkerThreadState::JobType::None) {
        LeaveCriticalSection(&g_worker_lock);
        return false;
    }

    // Set up job
    strncpy_s(g_worker.clone_url, url, sizeof(g_worker.clone_url) - 1);
    wcsncpy_s(g_worker.clone_destination, destination, _countof(g_worker.clone_destination) - 1);
    g_worker.job_type = WorkerThreadState::JobType::Clone;
    g_worker.silent = false;  // Always show clone progress

    LeaveCriticalSection(&g_worker_lock);

    // Start progress timer
    SetTimer(g_main_window, CLONE_PROGRESS_TIMER_ID, FETCH_PROGRESS_INTERVAL_MS, nullptr);

    // Signal worker thread
    SetEvent(g_worker.work_event);

    return true;
}

// Check if worker is currently busy
bool worker_is_busy() {
    EnterCriticalSection(&g_worker_lock);
    bool busy = (g_worker.job_type != WorkerThreadState::JobType::None);
    LeaveCriticalSection(&g_worker_lock);
    return busy;
}

// Get current worker job type
WorkerJobType worker_get_job_type() {
    EnterCriticalSection(&g_worker_lock);
    WorkerJobType job = static_cast<WorkerJobType>(g_worker.job_type);
    LeaveCriticalSection(&g_worker_lock);
    return job;
}

// Get fetch progress (for UI rendering)
const FetchProgress* worker_get_fetch_progress() {
    return &g_worker.progress;
}

// Get push progress (for UI rendering)
const PushProgress* worker_get_push_progress() {
    return &g_worker.push_progress;
}

// Get pull progress (for UI rendering)
const PullProgress* worker_get_pull_progress() {
    return &g_worker.pull_progress;
}

// Get checkout progress (for UI rendering)
const CheckoutProgress* worker_get_checkout_progress() {
    return &g_worker.checkout_progress;
}

// Get clone progress (for UI rendering)
const CloneProgress* worker_get_clone_progress() {
    return &g_worker.clone_progress;
}

// Request cancellation of clone operation
void worker_request_clone_cancel() {
    g_worker.clone_progress.base.cancel_requested.store(true);
}

// Get fetch result (after completion)
const FetchResult* worker_get_fetch_result() {
    return &g_worker.result;
}

// Get push result (after completion)
const PushResult* worker_get_push_result() {
    return &g_worker.push_result;
}

// Get pull result (after completion)
const PullResult* worker_get_pull_result() {
    return &g_worker.pull_result;
}

// Get checkout result (after completion)
const CheckoutResult* worker_get_checkout_result() {
    return &g_worker.checkout_result;
}

// Get clone result (after completion)
const CloneResult* worker_get_clone_result() {
    return &g_worker.clone_result;
}

// Get clone destination path (after completion)
const wchar_t* worker_get_clone_destination() {
    return g_worker.clone_destination;
}

// Get clone URL (for auth retry)
const char* worker_get_clone_url() {
    return g_worker.clone_url;
}

// Initialize worker thread
bool worker_init() {
    InitializeCriticalSection(&g_worker_lock);
    g_worker.work_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);  // Auto-reset
    g_worker.shutdown_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);  // Manual reset
    g_worker.thread = CreateThread(nullptr, 0, worker_thread_proc, nullptr, 0, nullptr);

    return (g_worker.thread != nullptr &&
            g_worker.work_event != nullptr &&
            g_worker.shutdown_event != nullptr);
}

// Cleanup worker thread
void worker_cleanup() {
    if (g_worker.thread) {
        // Signal shutdown and wait
        SetEvent(g_worker.shutdown_event);
        DWORD wait_result = WaitForSingleObject(g_worker.thread, 5000);
        if (wait_result != WAIT_OBJECT_0) {
            // Thread did not exit in time - likely stuck in a network call.
            // Intentionally leak handles/events rather than close them while a live
            // worker may still be reading/writing them, which would be a UAF.
            LOG("[worker] cleanup: thread did not exit within 5s (result=%lu) - "
                "skipping handle close to avoid UAF",
                wait_result);
            g_worker.thread = nullptr;
            g_worker.work_event = nullptr;
            g_worker.shutdown_event = nullptr;
            // CRITICAL section is owned by the (still-live) thread; leave it as-is.
            return;
        }
        CloseHandle(g_worker.thread);
        g_worker.thread = nullptr;
    }
    if (g_worker.work_event) {
        CloseHandle(g_worker.work_event);
        g_worker.work_event = nullptr;
    }
    if (g_worker.shutdown_event) {
        CloseHandle(g_worker.shutdown_event);
        g_worker.shutdown_event = nullptr;
    }
    DeleteCriticalSection(&g_worker_lock);
}
