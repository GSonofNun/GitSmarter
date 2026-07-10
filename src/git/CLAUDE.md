# Git Operations | [git Index]|root: ./src/git
|IMPORTANT: Prefer retrieval-led reasoning over pre-training-led reasoning
|Objects:{git_objects.cpp:GitObject,PackFile,loose objects,pack extraction}
|Repo:{git_repo.cpp:GitRepo,HEAD parsing,ref listing}
|Index:{git_index.cpp:GitIndex,staging,read/write}
|Diff:{git_diff.cpp:diff generation,hunks,rename detection}
|Commit:{git_commit.cpp:commit creation,tree building}
|Checkout:{git_checkout.cpp:working tree,file checkout}
|Push:{git_push.cpp:pack generation}
|History:{git_history.cpp:commit graph,traversal}
|Syntax:{git_syntax.cpp:syntax highlighting}

Low-level git implementation without libgit2 or CLI. Direct parsing of git internals.

## Quick Patterns (Retrieval-Led)

**Open a repository:**
```cpp
GitRepo* repo = git_repo_open(path);
```

**Read an object (loose or pack):**
```cpp
GitObject* obj = git_object_read(repo, sha);
// obj->type: OBJ_COMMIT, OBJ_TREE, OBJ_BLOB, OBJ_TAG
// obj->data: Raw object content
```

**Parse a commit:**
```cpp
GitCommit commit;
git_commit_parse(&commit, obj->data, obj->size);
// commit.tree, commit.parent_count, commit.parents[], commit.message
```

**Read the index:**
```cpp
GitIndex index;
git_index_read(&index, repo->git_dir);
```

## File Responsibilities

| File | Purpose |
|------|---------|
| git_objects.cpp | Object parsing (blobs, trees, commits, tags), loose objects, pack file extraction |
| git_repo.cpp | Repository detection, HEAD parsing, ref listing |
| git_index.cpp | Index file reading/writing, staging operations |
| git_diff.cpp | Diff generation, hunk parsing, rename detection |
| git_commit.cpp | Commit creation, tree building |
| git_checkout.cpp | Working tree updates, file checkout |
| git_push.cpp | Pack generation for push operations |
| git_history.cpp | Commit graph traversal, history walking |
| git_syntax.cpp | Syntax highlighting for diff viewer |
| git_test.cpp | Git-related test utilities |

## Key Data Structures

### GitRepo
```cpp
struct GitRepo {
    wchar_t path[MAX_PATH];      // Working directory
    wchar_t git_dir[MAX_PATH];   // .git directory
    bool is_bare;
    char head_sha[Git::SHA_HEX_SIZE];  // Current HEAD SHA
    char head_branch[64];        // Branch name (if not detached)
    bool head_detached;
};
```

### GitObject
```cpp
struct GitObject {
    int type;                    // OBJ_COMMIT, OBJ_TREE, OBJ_BLOB, OBJ_TAG
    uint8_t* data;              // Raw object content (owned)
    size_t size;
    char sha[Git::SHA_HEX_SIZE]; // SHA-1 hash
};
```

### PackFile / PackIndex
```cpp
struct PackIndex {
    uint32_t* fanout;           // 256 entry fanout table
    uint32_t* offsets;          // Object offsets in pack
    char* sha_table;            // Packed SHA hashes
    int object_count;
};

struct PackFile {
    MemoryMap mmap;             // Memory-mapped .pack file
    PackIndex index;            // Parsed .idx file
    char path[MAX_PATH];
};
```

### GitIndex
```cpp
struct GitIndexEntry {
    char sha[Git::SHA_HEX_SIZE];
    char path[MAX_PATH];
    uint32_t mode;
    uint32_t size;
    FILETIME mtime;
    uint32_t stage;  // For merge conflicts (0 = normal)
};

struct GitIndex {
    GitIndexEntry* entries;
    size_t entry_count;
    size_t capacity;
    uint32_t version;
};
```

## Pack File Patterns

Pack files are memory-mapped via `MemoryMap` for efficient access:

1. **Object lookup** - SHA → fanout → binary search in SHA table → offset
2. **Delta chains** - Base → delta → delta (depth varies, resolved recursively)
3. **Reading** - Use `pack_read_object()` for automatic delta resolution

```cpp
// Read object from pack (handles deltax automatically)
GitObject* obj = pack_read_object(pack_file, sha);
```

## Rename Detection

GitSmarter implements similarity-based rename detection (like `git -M`):

1. **Phase 1.5** - Between staged adds/deletes in `git_status_compute()`
2. **Algorithm** - Compare deleted files with added files using line-hash comparison for O(n) performance
3. **Threshold** - 50% similarity matches git's default `-M` flag behavior
4. **Result** - `GitStatusEntry` includes `old_path` and `similarity` fields

## Side-by-Side Diff Pairing

Uses non-crossing constraint matching for line pairing:

1. **Non-crossing constraint** - If removed[i] matches added[j], then removed[i+1] can only match added[k] where k > j
2. **Block replacement detection** - If <30% of lines match, output all removals then all additions (matches VS Code behavior)
3. **Constants** - `SIMILARITY_THRESHOLD` (50%), `BLOCK_REPLACEMENT_THRESHOLD_PERCENT` (30%)

## Session Learnings

- Rename detection uses similarity-based matching (like `git -M`), comparing deleted files with added files using line-hash comparison for O(n) performance
- `git_status_compute()` has Phase 1.5 for rename detection between staged adds/deletes detection and unstaged changes
- Similarity threshold of 50% matches git's default `-M` flag behavior
- Side-by-side diff pairing uses non-crossing constraint matching: if removed[i] matches added[j], then removed[i+1] can only match added[k] where k > j
- Block replacement detection: if <30% of lines match (50% similarity threshold), output all removals then all additions instead of interleaving (matches VS Code behavior)
- `diff_line_similarity()` computes line similarity for pairing decisions
- `pair_replacement_block()` handles the core pairing logic for removed/added line blocks
- `DiffPairing` namespace in git_diff.cpp contains named constants for side-by-side diff thresholds: `SIMILARITY_THRESHOLD` (50%) and `BLOCK_REPLACEMENT_THRESHOLD_PERCENT` (30%)
- Safer integer arithmetic for threshold checks: use `match_count < (smaller_count * THRESHOLD / 100)` instead of `match_count * 100 / smaller_count < THRESHOLD` to avoid overflow
- Large stack buffers (>64KB) in functions that may be called in deep call stacks cause `__chkstk` crashes on Windows - use heap allocation instead
- `stat_file_for_index()` should preserve pre-set mode values; only default to `Git::MODE_FILE` when mode is 0 (for reset/clone operations that set mode from tree)
- `git reset --hard` does NOT remove untracked files (correct git behavior) - only modifies tracked files to match target commit
- Test fixture `test/fixtures/test_repo` includes generated `.txt` files (SHA references) that are intentionally untracked - regenerate with `regen_test_repo.bat` if corrupted
- Pack-native clone: `generate_pack_index()` in network.cpp writes .idx v2 format directly instead of extracting loose objects - 1,660x faster write phase (78s → 47ms)
- Pack index CRC32 is computed over the compressed zlib data segment using `crc32()` from zlib
- HEAD symref must be explicitly requested in ls-refs with `ref-prefix HEAD` to get the default branch target
- `update_remote_refs()` handles HEAD separately (first pass) to write `refs/remotes/origin/HEAD` as symbolic ref before filtering
- Disk streaming clone eliminates 5.2s pack installation phase by writing directly to final location and renaming (18.6s → 15.7s)
- Memory usage reduced from 512MB (pre-allocated buffer) to ~4MB (64MB sliding window, only portion mapped at a time)
- Clone performance: 15.7s vs Git CLI 10.9s (1.44x slower, down from 1.7x)
- `proto_v2_fetch_pack_disk_streaming()` and `install_pack_file_disk_streaming()` handle direct-to-disk path in network.cpp
- Lock-free delta resolution eliminates barrier overhead between depth levels; reverse dependency tracking enables immediate child activation when parent resolves
- zlib-ng provides 4.2x faster inflate than bundled zlib (491ms vs 2079ms for 226MB → 297MB)
- Pack parsing decompression is ~48% of delta resolution phase time; remaining time is delta application, SHA computation, hash lookups
- SIMD optimizations enabled: SSE2, SSSE3, SSE4.1, SSE4.2, PCLMULQDQ, AVX2, AVX512, AVX512VNNI, VPCLMULQDQ
- `batch_create_directories()` MAX_DIRS increased from 4096 to 8192 for large repos like Linux kernel (~4500 directories)
- Windows NTFS case collision handling: when `write_file_atomic` fails but file exists (case-insensitive match), return success and increment `case_collisions` counter (matches Git CLI behavior)
- `CheckoutWorkItem.case_collision` flag and `ParallelCheckoutContext.case_collisions` counter track expected Windows filesystem collisions
- Linux kernel has 26 case-colliding file pairs (e.g., `xt_CONNMARK.h` vs `xt_connmark.h`) - Git CLI warns but succeeds, GitSmarter now does same
- URL shorthand expansion in `git_clone_expand_url_shorthand()`: `gh:owner/repo` and plain `owner/repo` (no dots, single slash) expand to `https://github.com/owner/repo.git`
- Shorthand detection excludes URLs with dots (`.`) to avoid matching `github.com/...` as `owner/repo` format
- `batch_create_directories()` now uses dynamic allocation: counts unique dirs in first pass, allocates exact size, no MAX_DIRS limit
- BCrypt constants at file scope: `CPUID_EBX_SHA_BIT = 29` for SHA-NI detection, `BCRYPT_MAX_CHUNK_BYTES = 1GB` for chunked hashing
- Clone telemetry: `TELEM_CLONE_PHASE()` macros capture structured data; redundant `LOG("clone_telemetry: ...")` calls were removed
- SHA-1 validation for pack files >4GB requires chunking: `BCryptHashData` takes `ULONG` (32-bit) size, so chunk into 1GB pieces
- `Git::MAX_TREE_ENTRIES` increased to 200000 for large repos like Linux kernel (92K files)
- `ShardedObjectCache` with 16 shards distributes lock contention (~6% collision probability with 32 threads vs 100% with single lock)
- Cache config: `clone_cache_size_gb` in settings.json (0 = auto 25% RAM, >0 = fixed GB)
- Synthetic metadata for clone index: single `GetSystemTimeAsFileTime()` at start, incrementing atomic inode counter, size from blob read - zero stat syscalls
- Concurrent index build for clone: collects metadata in workers, builds index from work items in O(N) with no syscalls
