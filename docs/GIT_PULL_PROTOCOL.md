# Git Pull Protocol (Fast-Forward Only)

This document explains the Git Pull operation for GitSmarter, focusing on the fast-forward merge strategy. Pull is implemented as **fetch + fast-forward merge**.

## Table of Contents
1. [Pull Protocol Overview](#pull-protocol-overview)
2. [Prerequisites](#prerequisites)
3. [Fast-Forward Algorithm](#fast-forward-algorithm)
4. [Tree Checkout Process](#tree-checkout-process)
5. [Index Update](#index-update)
6. [Error Scenarios](#error-scenarios)
7. [Implementation Notes](#implementation-notes)

---

## Pull Protocol Overview

Git pull is a compound operation that:
1. **Fetches** new objects from a remote repository
2. **Merges** the fetched changes into the current branch

GitSmarter implements **fast-forward only** pull, which means:
- The local branch must be a direct ancestor of the remote branch
- No merge commit is created
- Simply moves HEAD forward to the remote tip

### When Fast-Forward Works

```
Before Pull:
    A---B---C  (origin/main, FETCH_HEAD)
         \
          (HEAD, main)

After Pull:
    A---B---C  (HEAD, main, origin/main)
```

The local `main` is at commit B, remote `origin/main` is at commit C. Since B is an ancestor of C, we can fast-forward HEAD from B to C.

### When Fast-Forward Fails

```
Before Pull (diverged):
    A---B---C  (origin/main)
         \
          D---E  (HEAD, main)

Cannot fast-forward - local has commits D and E not on remote.
```

In this case, GitSmarter rejects the pull with an error message directing the user to push first or manually merge.

---

## Prerequisites

Before pull can proceed, these conditions must be met:

### 1. Valid Branch State

- HEAD must not be detached (must point to a branch)
- The current branch must have an upstream remote configured

**Check:**
```cpp
if (repo->head_detached) {
    return error("Cannot pull: HEAD is detached");
}
if (repo->head_ref[0] == '\0') {
    return error("Cannot pull: no branch checked out");
}
```

### 2. Clean Working Directory

- No staged changes (index matches HEAD)
- No unstaged changes (working directory matches index)

**Why Required:**
- Fast-forward updates the working directory to match the new commit
- Uncommitted changes would be overwritten or cause conflicts
- Unlike full merge, we don't handle conflict markers

**Check:**
```cpp
if (!git_workdir_is_clean(repo)) {
    return error("Cannot pull: working directory has uncommitted changes");
}
```

### 3. Successful Fetch

- Network connectivity to remote
- Valid authentication (if required)
- Remote branch exists

**FETCH_HEAD:**
After fetch, Git writes `.git/FETCH_HEAD` with the fetched refs:
```
abc123def456...  branch 'main' of https://github.com/user/repo
```

---

## Fast-Forward Algorithm

### Determining Fast-Forward Eligibility

To check if local can fast-forward to remote:

```
is_fast_forward(local_sha, remote_sha):
    # Same commit = already up to date (trivial FF)
    if local_sha == remote_sha:
        return true, 0  # 0 commits to pull

    # Walk ancestors of remote looking for local
    current = remote_sha
    depth = 0

    while current != null:
        depth++
        if current == local_sha:
            return true, depth  # local is ancestor, FF possible

        commit = read_commit(current)
        current = commit.parent_sha  # Follow first parent

    return false, 0  # local is not an ancestor - diverged
```

### Ancestor Walking

The algorithm walks the commit graph from remote back to local:

```
Remote (FETCH_HEAD): C
                     |
                     v
                     B  <-- Found local SHA here (depth=2)
                     |
                     v
                     A
                     |
                     v
                   (root)

Result: Fast-forward possible, 2 commits to pull (B->C)
```

### Handling Merge Commits

For merge commits (multiple parents), we follow the **first parent** only:

```cpp
GitCommit commit;
git_read_commit(repo, current, &commit);
current = commit.parent_sha;  // First parent only
```

This matches Git's `--first-parent` behavior and ensures we stay on the main branch line.

---

## Tree Checkout Process

After confirming fast-forward is possible, we update the working directory.

### Algorithm Overview

```
checkout_tree(repo, old_tree_sha, new_tree_sha):
    # 1. Flatten both trees to get all files
    old_files = read_tree_flat(old_tree_sha)  # {path -> (sha, mode)}
    new_files = read_tree_flat(new_tree_sha)  # {path -> (sha, mode)}

    # 2. Update or create files in new tree
    for (path, new_entry) in new_files:
        old_entry = old_files.get(path)

        if old_entry and old_entry.sha == new_entry.sha:
            continue  # File unchanged, skip

        # Write blob content to working directory
        content = read_blob(new_entry.sha)
        write_file(repo.path / path, content)
        set_mode(repo.path / path, new_entry.mode)

    # 3. Delete files removed in new tree
    for (path, old_entry) in old_files:
        if path not in new_files:
            delete_file(repo.path / path)
            # Optionally: remove empty parent directories
```

### Tree Flattening

Git stores trees hierarchically. We flatten to get all file paths:

```
Tree (root):
  blob "README.md"   -> abc123
  tree "src"         -> def456
    blob "main.cpp"  -> 789abc
    blob "utils.cpp" -> fedcba

Flattened:
  "README.md"     -> abc123
  "src/main.cpp"  -> 789abc
  "src/utils.cpp" -> fedcba
```

### File Writing

For each file to update:

1. **Read blob content** from pack file or loose object
2. **Ensure parent directory exists** (create if needed)
3. **Write atomically** (write to temp, rename)
4. **Set file mode** (executable bit for mode 100755)

```cpp
bool checkout_file(GitRepository* repo, const char* path, const char* blob_sha) {
    // Read blob
    size_t size;
    char* content = git_read_blob(repo, blob_sha, &size);
    if (!content) return false;

    // Build full path
    wchar_t full_path[MAX_PATH];
    build_path(full_path, repo->path, path);

    // Ensure parent directory
    ensure_parent_directory(full_path);

    // Write file
    bool ok = write_file_atomic(full_path, content, size);
    delete[] content;
    return ok;
}
```

### Directory Handling

**Creating directories:**
```cpp
void ensure_parent_directory(const wchar_t* path) {
    wchar_t parent[MAX_PATH];
    wcscpy_s(parent, path);

    wchar_t* last_slash = wcsrchr(parent, L'\\');
    if (last_slash) {
        *last_slash = L'\0';
        SHCreateDirectoryExW(nullptr, parent, nullptr);
    }
}
```

**Removing empty directories** (optional cleanup after file deletion):
```cpp
void remove_empty_parents(const wchar_t* path) {
    wchar_t parent[MAX_PATH];
    wcscpy_s(parent, path);

    while (true) {
        wchar_t* last_slash = wcsrchr(parent, L'\\');
        if (!last_slash) break;
        *last_slash = L'\0';

        if (!RemoveDirectoryW(parent)) break;  // Not empty or failed
    }
}
```

---

## Index Update

After updating the working directory, we must update `.git/index` to match.

### Index Structure

The index maps file paths to their blob SHAs and stat information:

```
Index Entry:
  - ctime, mtime (timestamps)
  - dev, ino (device/inode)
  - mode (file mode: 100644 or 100755)
  - uid, gid (owner info)
  - file_size
  - sha (20 bytes)
  - flags (name length, stage bits)
  - path (variable length, NUL-terminated)
```

### Rebuild Strategy

For fast-forward pull, we rebuild the index from the new tree:

```cpp
bool update_index_from_tree(GitRepository* repo, const char* tree_sha) {
    // Lock index
    if (!git_index_lock(repo, 5000)) return false;

    // Read new tree
    GitIndexEntry* entries;
    size_t count = git_read_tree_flat(repo, tree_sha, ...);

    // Build index entries with stat info
    GitIndexFull index = {};
    for (size_t i = 0; i < count; i++) {
        GitIndexEntryFull* entry = &index.entries[index.entry_count++];

        strcpy_s(entry->path, entries[i].path);
        strcpy_s(entry->sha, entries[i].sha);
        entry->mode = entries[i].mode;

        // Stat the file to get timestamps
        stat_file_for_index(repo, entries[i].path, entry);
    }

    // Write and unlock
    bool ok = git_index_write(repo, &index);
    git_index_unlock(repo);
    return ok;
}
```

### Stat Caching

The index caches stat information for efficient status detection:

```cpp
void stat_file_for_index(GitRepository* repo, const char* path,
                         GitIndexEntryFull* entry) {
    wchar_t full_path[MAX_PATH];
    build_path(full_path, repo->path, path);

    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (GetFileAttributesExW(full_path, GetFileExInfoStandard, &attrs)) {
        entry->file_size = attrs.nFileSizeLow;
        entry->mtime_sec = filetime_to_unix(attrs.ftLastWriteTime);
        entry->ctime_sec = filetime_to_unix(attrs.ftCreationTime);
        // ... other fields
    }
}
```

---

## Error Scenarios

### 1. Detached HEAD

**Cause:** User checked out a specific commit, not a branch.

**Error:** `"Cannot pull: HEAD is detached"`

**Recovery:** Checkout a branch first with `git checkout main`.

### 2. Dirty Working Directory

**Cause:** Uncommitted changes in staged or unstaged files.

**Error:** `"Cannot pull: working directory has uncommitted changes"`

**Recovery:**
- Commit changes: `git commit -m "Save work"`
- Or stash changes: `git stash` (not yet implemented in GitSmarter)
- Or discard changes: `git checkout -- .` (use with caution)

### 3. Not Fast-Forward

**Cause:** Local branch has commits that remote doesn't have.

**Error:** `"Cannot fast-forward: local commits exist ahead of remote"`

**Recovery:**
- Push first: `git push` to share local commits
- Or merge manually: `git merge origin/main` (creates merge commit)
- Or rebase: `git rebase origin/main` (rewrites history)

### 4. No Upstream Branch

**Cause:** Remote doesn't have the branch we're trying to pull.

**Error:** `"Failed to read FETCH_HEAD after fetch"`

**Recovery:** Push to create the remote branch first.

### 5. Network/Auth Errors

**Cause:** Fetch phase failed due to network or authentication issues.

**Error:** `"Fetch failed: [specific error]"`

**Recovery:** Check network connection, verify credentials.

### 6. Already Up To Date

**Cause:** Local and remote are at the same commit.

**Message:** `"Already up to date."` (success, not error)

**Action:** No changes needed.

---

## Implementation Notes

### Threading Model

Pull runs on the dedicated worker thread (same as fetch/push):

```
Main Thread                    Worker Thread
     |                              |
     |-- start_async_pull() ------->|
     |                              |-- git_pull()
     |<-- progress updates ---------|    |-- git_fetch()
     |                              |    |-- is_fast_forward()
     |                              |    |-- checkout_tree()
     |                              |    |-- update_index()
     |<-- WM_PULL_COMPLETE ---------|
     |                              |
```

### Progress Reporting

Pull reports progress through multiple phases:

| Phase | Description |
|-------|-------------|
| Fetching | Running git_fetch() internally |
| CheckingFastForward | Verifying ancestor relationship |
| UpdatingWorkdir | Writing files to working directory |
| UpdatingIndex | Rebuilding .git/index |
| UpdatingRefs | Updating branch ref |
| Complete | Success |
| Failed | Error occurred |
| NotFastForward | Local has commits ahead |
| DirtyWorkdir | Uncommitted changes present |

### Atomicity Considerations

Pull is **not atomic** - if it fails partway through:
- Fetch is complete (objects downloaded)
- Working directory may be partially updated
- Index may not match working directory

**Recovery:** Re-running pull should succeed (or user can manually reset).

**Future improvement:** Could checkpoint working directory state before checkout.

### Performance Optimization

For large repositories:
- **Diff trees instead of flattening** - Only process changed paths
- **Parallel file writes** - Multiple threads for I/O
- **Incremental index update** - Only update changed entries

Current implementation prioritizes correctness over performance.

---

## References

- [Git git-pull Documentation](https://git-scm.com/docs/git-pull)
- [Git git-merge Documentation](https://git-scm.com/docs/git-merge)
- [Git Internals - Transfer Protocols](https://git-scm.com/book/en/v2/Git-Internals-Transfer-Protocols)
- [Git Index Format](https://git-scm.com/docs/index-format)
- [GitSmarter GIT_PUSH_PROTOCOL.md](./GIT_PUSH_PROTOCOL.md)

---

**Document Version:** 1.0
**Last Updated:** 2024-12-13
**Author:** Research compiled for GitSmarter project
