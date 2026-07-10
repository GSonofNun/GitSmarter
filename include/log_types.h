#pragma once
#include <atomic>
#include <cstdint>

namespace Log {
    // Categories (bitfield for efficient filtering)
    enum class Category : uint32_t {
        None        = 0,
        Error       = 1 << 0,   // Always enabled in release
        Warning     = 1 << 1,
        Info        = 1 << 2,
        Debug       = 1 << 3,   // Disabled in release
        Performance = 1 << 4,
        Telemetry   = 1 << 5,
        Network     = 1 << 6,
        Git         = 1 << 7,
        UI          = 1 << 8,
        All         = 0xFFFFFFFF
    };

    // Bitwise operators for Category
    inline constexpr Category operator|(Category a, Category b) {
        return static_cast<Category>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    inline constexpr Category operator&(Category a, Category b) {
        return static_cast<Category>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    // Debug subsystems for granular filtering
    enum class Subsystem : uint32_t {
        None        = 0,
        CommitView  = 1 << 0,
        DiffView    = 1 << 1,
        Sidebar     = 1 << 2,
        Clone       = 1 << 3,
        Fetch       = 1 << 4,
        Push        = 1 << 5,
        Checkout    = 1 << 6,
        PackParse   = 1 << 7,
        DeltaResolve= 1 << 8,
        Index       = 1 << 9,
        All         = 0xFFFFFFFF
    };

    // Bitwise operators for Subsystem
    inline constexpr Subsystem operator|(Subsystem a, Subsystem b) {
        return static_cast<Subsystem>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    inline constexpr Subsystem operator&(Subsystem a, Subsystem b) {
        return static_cast<Subsystem>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    // Compile-time category mask
    #ifdef GITSMARTER_DEBUG
    constexpr uint32_t COMPILE_TIME_MASK = static_cast<uint32_t>(Category::All);
    #else
    constexpr uint32_t COMPILE_TIME_MASK =
        static_cast<uint32_t>(Category::Error) |
        static_cast<uint32_t>(Category::Warning) |
        static_cast<uint32_t>(Category::Telemetry);
    #endif

    // Telemetry event types
    enum class TelemetryEvent : uint8_t {
        ClonePhase,
        FetchOperation,
        CheckoutPhase,
        PackParsing,
        DeltaResolution,
        ObjectCache,
        SHA1Stats,
        HTTPTransfer,
        WorkerJob
    };

    // Clone phases (for structured telemetry)
    enum class ClonePhase : uint32_t {
        Initializing,
        AddingRemote,
        Fetching,
        DeterminingBranch,
        CheckingOut,
        Complete
    };

    // Fetch operations
    enum class FetchOp : uint32_t {
        ServerDiscovery,
        ListRefs,
        FetchPack,
        InstallPack
    };

    // Telemetry entry (64 bytes, cache-aligned)
    struct alignas(64) TelemetryEntry {
        uint64_t timestamp_us;
        TelemetryEvent event_type;
        uint8_t flags;
        uint16_t reserved;
        uint32_t thread_id;

        union {
            struct { uint32_t phase; uint32_t elapsed_ms; uint64_t objects; uint64_t bytes; } clone;
            struct { uint32_t operation; uint32_t elapsed_ms; uint32_t refs; uint32_t status; uint64_t pack_size; } fetch;
            struct { uint32_t commits; uint32_t trees; uint32_t blobs; uint32_t tags; uint32_t ofs_delta; uint32_t ref_delta; } pack_parsing;
            struct { uint32_t max_depth; uint32_t resolved; uint64_t delta_apply_us; uint64_t sha_compute_us; } delta;
            struct { uint32_t hits; uint32_t misses; uint32_t evictions; uint32_t puts; uint64_t evicted_bytes; } cache;
            uint8_t raw[48];
        } data;
    };
    static_assert(sizeof(TelemetryEntry) == 64, "TelemetryEntry must be 64 bytes");

    // Lock-free ring buffer size
    constexpr size_t TELEMETRY_RING_SIZE = 1024;  // 64KB total

    // Ring buffer for telemetry (implementation in log.cpp)
    // Uses heap allocation to avoid large static array issues
    struct TelemetryRing {
        alignas(64) std::atomic<uint64_t> write_index{0};
        alignas(64) std::atomic<uint64_t> read_index{0};
        alignas(64) std::atomic<uint64_t> overflow_count{0};  // Tracks entries lost to overwrite
        TelemetryEntry* entries = nullptr;  // Heap-allocated, initialized in log_system_init()

        bool push(const TelemetryEntry& entry);
        void dump_to_json(const char* path);
        size_t count() const;
        uint64_t get_overflow_count() const;
        void clear();
        void init();
        void shutdown();
    };
}
