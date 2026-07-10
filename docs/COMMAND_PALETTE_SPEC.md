# Command Palette Specification

**Version:** 1.1 Draft
**Last Updated:** January 2026

---

## 1. Overview

### 1.1 Purpose
The command palette provides a unified, keyboard-driven interface for discovering and executing commands, navigating to files, switching branches, and finding commits. It reduces reliance on memorizing keyboard shortcuts while accelerating expert workflows.

### 1.2 Design Goals
- **Discoverability**: Surface all app functionality in one searchable interface
- **Speed**: Sub-100ms response times, minimal keystrokes to execute
- **Intelligence**: Auto-detect user intent, context-aware filtering
- **Learnability**: Display shortcuts alongside commands to build muscle memory
- **Accessibility**: Full keyboard navigation, screen reader support

---

## 2. Invocation

### 2.1 Keyboard Shortcut
- **Primary**: `Ctrl+P` opens the command palette
- **Alternative**: Clicking the search icon in the title bar (if present)

### 2.2 Focus Behavior
- Opening the palette immediately focuses the text input
- The palette captures all keyboard input until dismissed
- Background UI remains visible but non-interactive (no overlay dimming)

---

## 3. Search Modes

### 3.1 Mode Overview

| Prefix | Mode | Scope | Example |
|--------|------|-------|---------|
| (none) | Auto-detect | All scopes | `main` shows branch, files, commands |
| `>` | Commands | All app commands + file actions | `>stage` |
| `/` | Files | All files in repository | `/main.cpp` |
| `@` | Branches | Local/remote branches + tags | `@feature` |
| `#` | Commits | All commits (paginated) | `#fix bug` |
| `:` | Remotes | Remote repositories | `:upstream` |

### 3.2 Auto-Detect Behavior
When no prefix is typed, the palette searches across **all scopes simultaneously** and displays mixed results grouped by type:

```
─── Commands ───────────────────────────
  Stage All                    Ctrl+A
  Switch to main branch
─── Branches ───────────────────────────
  main
  feature/main-menu
─── Files ──────────────────────────────
 M src/main.cpp                 (staged)
   include/main_window.h
```

Results within each group are sorted by match quality, then recency.

### 3.3 Prefix Activation
- Typing a prefix character (`>`, `/`, `@`, `#`, `:`) switches to that mode
- The prefix transforms into a **colored badge/chip** before the input field
- The input field shows only the query (e.g., typing `>fetch` shows badge "Commands" + input "fetch")
- Deleting all characters removes the badge and returns to auto-detect mode

### 3.4 Mode Cycling
Power users can cycle through modes without retyping:
- `Ctrl+Tab`: Cycle forward: Auto → Commands → Files → Branches → Commits → Remotes → Auto
- `Ctrl+Shift+Tab`: Cycle backward
- Current query is preserved when switching modes

### 3.5 Scope Boundaries

| Mode | What's Searchable |
|------|-------------------|
| Commands (`>`) | All registered commands, file actions (stage, unstage, discard), navigation actions |
| Files (`/`) | All files in repository (tracked, untracked, ignored excluded) |
| Branches (`@`) | Local branches, remote tracking branches, tags |
| Commits (`#`) | All commits, paginated (load more on scroll) |
| Remotes (`:`) | Configured remotes (origin, upstream, forks) |

---

## 4. Search Algorithm

### 4.1 Fuzzy Matching Strategy
**Acronym + Substring matching**:
- Matches contiguous substrings: `stage` matches "**Stage** All"
- Matches first letters of words: `sa` matches "**S**tage **A**ll"
- Matches camelCase/snake_case boundaries: `sf` matches "**S**tage**F**ile"
- Case-insensitive by default

### 4.2 Scoring & Ranking

Results are ranked using concrete scoring weights:

```cpp
int score_match(std::string_view query, std::string_view target, const MatchContext& ctx) {
    int score = 0;

    // Match quality (primary factor)
    if (starts_with_exact(target, query))       score += 100;  // Exact prefix
    else if (word_boundary_match(target, query)) score += 60;  // Word boundary
    else if (acronym_match(target, query))       score += 40;  // Acronym (e.g., "sa" -> "Stage All")
    else if (substring_match(target, query))     score += 20;  // Substring anywhere

    // Per-character bonus for longer matches
    score += matched_char_count * 2;

    // Recency bonus (tiebreaker)
    int recency_position = get_recent_position(target);  // 0 = most recent
    if (recency_position >= 0 && recency_position < 10) {
        score += (10 - recency_position);  // +10 for most recent, +1 for 10th
    }

    // Context relevance bonus
    if (ctx.matches_current_view) score += 15;  // Command relevant to current panel
    if (ctx.is_frequently_used)  score += 10;  // In top 10 most-used commands

    return score;
}
```

### 4.3 Debounce Strategy

Different scopes have different performance characteristics:

| Scope | Debounce | Max Results | Rationale |
|-------|----------|-------------|-----------|
| Commands | 0ms | 50 | Small list (~50 items), instant |
| Files | 16ms | 100 | Cached index, fast lookup |
| Branches | 16ms | 100 | Usually <1000 refs |
| Commits | 100ms | 50 | Expensive, requires async loading |
| Remotes | 0ms | 20 | Tiny list |

**Progressive Loading for Mixed Mode:**
1. Commands and branches appear immediately (0ms)
2. Files appear after 16ms debounce
3. Commits appear after 100ms, with loading indicator
4. Late-arriving results animate in with subtle fade (50ms)

### 4.4 Built-in Aliases
Common Git terminology maps to GitSmarter commands:

| Alias | Maps To |
|-------|---------|
| `checkout`, `co` | Switch Branch |
| `add` | Stage File / Stage All |
| `reset` | Unstage File / Unstage All |
| `ci`, `commit` | Create Commit |
| `pull` | Pull from Origin |
| `push` | Push to Origin |
| `fetch` | Fetch from Origin |
| `branch`, `br` | Create Branch |
| `stash` | Stash Changes |
| `pop` | Pop Stash |

**Planned aliases (not implemented):** `merge`, `rebase`, `abort`, and `continue` will map to the planned merge/rebase/state commands (see Section 16.2) once those operations exist.

### 4.5 User-Configurable Aliases
Users can define custom aliases in settings:

```json
{
  "commandPalette": {
    "aliases": {
      "yolo": "git.push",
      "save": "git.commit",
      "co": "git.switchBranch"
    }
  }
}
```

Aliases are searched alongside canonical command names.

### 4.6 Configurable Keyboard Shortcuts
Users can remap command shortcuts in settings:

```json
{
  "commandPalette": {
    "shortcuts": {
      "git.fetch": "Ctrl+G",
      "git.stageAll": "Ctrl+Shift+S"
    }
  }
}
```

Changes apply globally; the palette displays the user's configured shortcut.

---

## 5. Context-Aware Filtering

### 5.1 Command Visibility
Commands that don't apply in the current context are **hidden** (not grayed out):

| Context | Hidden Commands |
|---------|-----------------|
| No files staged | "Create Commit", "Unstage All" |
| No unstaged changes | "Stage All", "Discard All" |
| Viewing historical commit | "Stage File", "Unstage File" |
| No repository open | All git commands |
| Already on branch X | "Switch to X" |
| No stashes exist | "Apply Stash", "Pop Stash", "Drop Stash" |

### 5.2 Git State-Specific Commands (Planned — not implemented)

> **Status:** Except for the Detached HEAD row, the commands in this section are not yet implemented in the command registry. This section documents the intended design for when merge/rebase/cherry-pick/bisect support lands.

The palette surfaces relevant commands based on transient Git states:

| Git State | Commands Surfaced | Commands Hidden |
|-----------|-------------------|-----------------|
| **Merge in progress** | "Continue Merge", "Abort Merge" | Most commit ops, branch switching |
| **Rebase in progress** | "Continue Rebase", "Abort Rebase", "Skip Commit" | Branch switching, new commits |
| **Cherry-pick in progress** | "Continue Cherry-pick", "Abort Cherry-pick" | Branch switching |
| **Bisect active** | "Bisect Good", "Bisect Bad", "Bisect Reset" | — |
| **Detached HEAD** | "Create Branch from HEAD" (promoted to top) | — |
| **Conflict detected** | "Open Merge Tool", "Mark Resolved", "Abort" | "Create Commit" until resolved |

State detection reads `.git/MERGE_HEAD`, `.git/REBASE_HEAD`, `.git/CHERRY_PICK_HEAD`, `.git/BISECT_LOG`.

### 5.3 Dynamic Command Names
Some commands adjust their names based on context:
- "Pull from Origin" → "Pull from Origin (3 commits behind)" when behind
- "Push to Origin" → "Push to Origin (2 commits ahead)" when ahead
- "Create Commit" → "Create Initial Commit" for empty repositories
- "Abort" → "Abort Merge" / "Abort Rebase" depending on state (planned; see Section 5.2)

---

## 6. File Status Indicators

### 6.1 Status Display
Files in results show their Git status using short-form indicators:

```
─── Files ──────────────────────────────
 M  src/main.cpp                 (staged)
 M  include/window.h           (modified)
 A  src/new_feature.cpp          (staged)
 D  old_file.cpp                 (staged)
 ?  temp/debug.log            (untracked)
 !  build/output.exe            (ignored)
    README.md
```

### 6.2 Status Legend

| Indicator | Meaning | Color |
|-----------|---------|-------|
| `M` | Modified | `Theme::Modified` (#519ABA) |
| `A` | Added (new file) | `Theme::Success` (#4EC970) |
| `D` | Deleted | `Theme::Danger` (#F14C4C) |
| `R` | Renamed | `Theme::Modified` (#519ABA) |
| `C` | Copied | `Theme::Modified` (#519ABA) |
| `?` | Untracked | `Theme::Warning` (#CCA700) |
| `!` | Ignored | `Theme::Text::Muted` (#6D6D6D) |
| (space) | Unmodified | — |

### 6.3 Staging State
Right-aligned label indicates staging:
- `(staged)` - File is in index
- `(modified)` - Unstaged working tree changes
- `(both)` - Staged and has additional unstaged changes
- `(untracked)` - Not tracked by Git
- (none) - Clean, tracked file

---

## 7. Multi-Select for Batch Operations

### 7.1 Enabling Multi-Select
In file mode (`/`) or auto-detect mode with file results:
- `Ctrl+Click` or `Ctrl+Enter`: Toggle item selection without executing
- Selected items show checkmark icon and accent background
- Selection count badge appears: "3 selected"

### 7.2 Batch Action Bar
When items are selected, an action bar appears above results:

```
┌─────────────────────────────────────────────────────┐
│ [Files] │ src/                                   🔍 │
├─────────────────────────────────────────────────────┤
│ 3 files selected    [Stage All] [Discard All] [×]  │
├─────────────────────────────────────────────────────┤
│ ✓ M  src/main.cpp                                  │
│ ✓ M  src/window.cpp                                │
│ ✓ A  src/palette.cpp                               │
│      src/utils.cpp                                  │
└─────────────────────────────────────────────────────┘
```

### 7.3 Batch Commands
- `Stage Selected`: Stage all selected files
- `Unstage Selected`: Unstage all selected files
- `Discard Selected`: Revert selected files (with confirmation)
- `Escape` or `×`: Clear selection

### 7.4 Keyboard Shortcuts for Multi-Select
| Key | Action |
|-----|--------|
| `Ctrl+Enter` | Toggle selection on current item |
| `Ctrl+A` | Select all visible results (in file mode only) |
| `Ctrl+Shift+A` | Deselect all |
| `Enter` (with selection) | Execute batch action (shows action picker) |

---

## 8. Visual Design

### 8.1 Dimensions & Position
- **Position**: Top-center of window, 80px from top edge
- **Width**: 500px fixed
- **Max visible items**: 10 (scrollable beyond)
- **Border radius**: 8px (matches app theme)
- **Shadow**: Elevated shadow matching app dialogs

### 8.2 Layout Structure

```
┌─────────────────────────────────────────────────────┐
│ [Badge] │ Search input...                        🔍 │  <- Input row (40px)
├─────────────────────────────────────────────────────┤
│ ─── Recent ─────────────────────────────────────    │  <- Group header
│   Fetch from Origin                    Ctrl+Shift+F │  <- Item row (32px)
│   Stage All                            Ctrl+A       │
│   Create Commit                        Ctrl+Enter   │
├─────────────────────────────────────────────────────┤
│ ─── Commands ───────────────────────────────────    │
│   Push to Origin                       Ctrl+Shift+U │
│   Pull from Origin                     Ctrl+Shift+P │
│ ...                                                 │
├─────────────────────────────────────────────────────┤
│ > commands  / files  @ branches  # commits     [×]  │  <- Hint row (24px, dismissible)
└─────────────────────────────────────────────────────┘
```

### 8.3 Hint Row Behavior
- Shown by default for new users
- `[×]` button dismisses permanently (stored in settings)
- Setting: `"commandPalette.showHints": false` to disable
- Hint text updates based on current mode

### 8.4 Colors (Theme Integration)

| Element | Color | Notes |
|---------|-------|-------|
| Background | `Theme::Background::Elevated` | #333337 |
| Border | `Theme::Border` | #3C3C3C |
| Input background | `Theme::Background::Secondary` | #252526 |
| Input text | `Theme::Text::Primary` | #E0E0E0 |
| Placeholder text | `Theme::Text::Muted` | #6D6D6D |
| Group header text | `Theme::Text::Secondary` | #9D9D9D |
| Item text | `Theme::Text::Primary` | #E0E0E0 |
| Shortcut text | `Theme::Text::Secondary` | #9D9D9D |
| Selected item bg | `Theme::Accent` at 20% | #0078D433 |
| Selected item border-left | `Theme::Accent` | #0078D4, 2px |
| Hover item bg | `Theme::Hover` | #3E3E42 |
| Multi-select check | `Theme::Accent` | #0078D4 |
| Badge: Commands | `Theme::Accent` | #0078D4 |
| Badge: Files | `Theme::Text::Primary` | #E0E0E0 |
| Badge: Branches | `Theme::Branch1` | #6B9FED |
| Badge: Commits | `Theme::Warning` | #CCA700 |
| Badge: Remotes | `Theme::Branch2` | #C678DD |

### 8.5 Typography

| Element | Font | Size | Weight |
|---------|------|------|--------|
| Input text | Segoe UI Variable | 14px | Regular |
| Item text | Segoe UI Variable | 13px | Regular |
| Shortcut text | Segoe UI Variable | 12px | Regular |
| Group header | Segoe UI Variable | 11px | SemiBold |
| Hint text | Segoe UI Variable | 11px | Regular |
| Badge text | Segoe UI Variable | 11px | SemiBold |
| File status indicator | Cascadia Code | 12px | Regular |

### 8.6 Item Layout

```
┌─────────────────────────────────────────────────────┐
│ [Check][Status][Icon]  Item Name        [Shortcut]  │
│   20px   16px   16px   <- flex ->       right-align │
└─────────────────────────────────────────────────────┘
         8px gap                          8px padding
```

- Checkbox: 20px, visible only in multi-select mode
- Status: 16px, for file status indicators (M/A/D/?)
- Icon: 16x16px, optional per item type
- Keyboard shortcuts: Always visible, right-aligned, secondary color

---

## 9. Animation

### 9.1 Appearance
- **Style**: Fade in + subtle slide down
- **Duration**: 100ms
- **Easing**: ease-out
- **Transform**: opacity 0 → 1, translateY(-8px) → 0

### 9.2 Dismissal
- **Style**: Fade out
- **Duration**: 80ms (slightly faster than appear)
- **Easing**: ease-in

### 9.3 Late-Arriving Results
When async results (commits) arrive after initial render:
- New items fade in (50ms) at their sorted position
- Existing items don't shift abruptly; new items insert smoothly

### 9.4 Selection Changes
- **Style**: Instant background color change (no animation)
- Keeps interaction feeling snappy

### 9.5 Command Execution Feedback
For instant commands (non-long-running):
- Brief status bar flash with result: "Staged 3 files" (1500ms auto-dismiss)
- Palette closes with success checkmark micro-animation (optional polish)

---

## 10. Keyboard Interaction

### 10.1 Navigation

| Key | Action |
|-----|--------|
| `↑` / `↓` | Move selection up/down |
| `Enter` | Execute selected command |
| `Escape` | Close palette (single press) |
| `Ctrl+U` | Clear input, stay open |
| `Ctrl+Tab` | Cycle to next mode |
| `Ctrl+Shift+Tab` | Cycle to previous mode |
| `Tab` | Move to next group |
| `Shift+Tab` | Move to previous group |
| `Page Up` / `Page Down` | Jump 5 items |
| `Home` | Select first item |
| `End` | Select last item |

### 10.2 Text Input

| Key | Action |
|-----|--------|
| Any printable character | Append to query |
| `Backspace` | Delete character before cursor |
| `Ctrl+Backspace` | Delete word before cursor |
| `Ctrl+A` | Select all text in input field |

**Note:** `Ctrl+A` always selects text when the input field is focused. The "Stage All" command uses `Ctrl+A` only when the palette is closed (existing app behavior).

### 10.3 Escape Behavior
- **Single press**: Close palette immediately
- If in inline-input mode: Cancel input, return to command list (then Escape again to close)

**Rationale:** Matches VS Code and user muscle memory expectations. `Ctrl+U` provides clear-without-close for users who want that workflow.

---

## 11. Mouse Interaction

### 11.1 Behaviors

| Action | Result |
|--------|--------|
| Click on item | Execute immediately |
| `Ctrl+Click` on item | Toggle multi-select (file mode) |
| Hover on item | Highlight (no selection change) |
| Click outside palette | Close palette |
| Scroll wheel | Scroll results list |

### 11.2 Mouse vs Keyboard Selection
- Keyboard arrows change the "selected" item (highlighted with accent border)
- Mouse hover shows a separate "hover" state (lighter highlight)
- Both can coexist; keyboard selection takes precedence for Enter key

---

## 12. Inline Input Mode

### 12.1 Triggering
When a command requires simple input (e.g., "Create Branch"), selecting it transforms the palette:

```
┌─────────────────────────────────────────────────────┐
│ [Create Branch]  Enter branch name...            🔍 │
├─────────────────────────────────────────────────────┤
│                                                     │
│   Branch will be created from current HEAD          │
│                                                     │
│   Suggestions:                                      │
│     feature/                                        │
│     bugfix/                                         │
│     hotfix/                                         │
└─────────────────────────────────────────────────────┘
```

### 12.2 Inline Input Autocomplete
For certain commands, provide contextual suggestions:

| Command | Autocomplete Source |
|---------|---------------------|
| Create Branch | Common prefixes from existing branches (`feature/`, `bugfix/`) |
| Switch Branch | Fuzzy match existing branches |
| Fetch from Remote | List of configured remotes |
| Add Remote | (no autocomplete) |

### 12.3 Commands Using Inline Input
- Create Branch (name input + autocomplete)
- Rename Branch (new name input)
- Search Commits (search query)
- Go to Line (line number)
- Add Remote (name + URL, two-step)

### 12.4 Commands Using Dialogs
Complex operations that require multiple inputs open dedicated dialogs:
- Create Stash (file selection + message)
- Authentication (username + password)
- Clone Repository (URL + destination)

### 12.5 Inline Input Behavior
- `Enter`: Submit input and execute command
- `Escape`: Cancel and return to command list
- `↑` / `↓`: Navigate autocomplete suggestions
- `Tab`: Accept autocomplete suggestion
- Input validation shown inline (e.g., "Branch name cannot contain spaces")

---

## 13. Initial State & Recent Commands

### 13.1 On Open (Empty Query)
When the palette opens, before any typing:

```
─── Recent ─────────────────────────────
  Fetch from Origin              Ctrl+Shift+F
  Stage All                      Ctrl+A
  Switch to main
  Create Commit                  Ctrl+Enter
  Push to Origin                 Ctrl+Shift+U
─────────────────────────────────────────
> commands  / files  @ branches  # commits
```

- Shows last 5-8 unique commands used
- Sorted by most recent first
- Once user starts typing, switches to search results

### 13.2 Persistence
- Recent commands stored **globally** (not per-repository)
- Persisted to settings file as JSON array
- Maximum 50 items stored, oldest pruned
- User aliases and their resolutions both recorded

---

## 14. Empty State & Errors

### 14.1 No Results

```
┌─────────────────────────────────────────────────────┐
│ [Commands] │ xyznonexistent                      🔍 │
├─────────────────────────────────────────────────────┤
│                                                     │
│   No commands found                                 │
│   Try removing the > prefix to search all          │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### 14.2 Context-Specific Suggestions

| Mode | Empty State Message |
|------|---------------------|
| Commands (`>`) | "No commands found. Try removing > to search all." |
| Files (`/`) | "No files match. Check the path or try a different pattern." |
| Branches (`@`) | "No branches match. Try a different name." |
| Commits (`#`) | "No commits match. Try searching by message or author." |
| Remotes (`:`) | "No remotes configured." |

---

## 15. Long-Running Commands

### 15.1 Behavior
When a command triggers a long-running operation (Fetch, Push, Pull):
1. Palette closes immediately
2. Status bar shows progress (existing behavior)
3. No change to palette behavior

### 15.2 Rationale
Keeping the palette open during operations would block user interaction. The status bar is the established location for progress feedback in GitSmarter.

---

## 16. Command Registry

### 16.1 Command Structure

```cpp
struct PaletteCommand {
    std::string_view id;              // Unique identifier: "git.stageAll"
    std::string_view name;            // Display name: "Stage All"
    std::string_view category;        // Group: "Git", "Navigation", "View"
    std::string_view shortcut;        // Display shortcut: "Ctrl+A" (empty if none)
    std::span<const std::string_view> aliases;  // Built-in aliases: {"add", "a"}
    bool (*is_available)();           // Context check function
    void (*execute)();                // Action function
    bool requires_input;              // Opens inline input mode
    std::string_view input_prompt;    // "Enter branch name..."
};
```

### 16.2 Initial Command Set

**Git Operations**
| ID | Name | Shortcut | Category |
|----|------|----------|----------|
| `git.stageAll` | Stage All | `Ctrl+A` | Git |
| `git.unstageAll` | Unstage All | `Ctrl+Shift+A` | Git |
| `git.fetch` | Fetch from Origin | `Ctrl+Shift+F` | Git |
| `git.push` | Push to Origin | `Ctrl+Shift+U` | Git |
| `git.pull` | Pull from Origin | `Ctrl+Shift+P` | Git |
| `git.commit` | Create Commit | `Ctrl+Enter` | Git |
| `git.stash` | Stash Changes | `Ctrl+Shift+S` | Git |
| `git.stashPop` | Pop Stash | | Git |
| `git.stashApply` | Apply Stash | | Git |
| `git.stashDrop` | Drop Stash | | Git |
| `git.createBranch` | Create Branch | | Git |
| `git.switchBranch` | Switch Branch | | Git |
| `git.discardAll` | Discard All Changes | | Git |

**Git State Commands** (context-dependent visibility)
| ID | Name | Visible When | Category |
|----|------|--------------|----------|
| `git.createBranchFromHead` | Create Branch from HEAD | Detached HEAD | Git |

**Planned Git State Commands (not implemented)** — these require merge/rebase/cherry-pick/bisect support (see Section 5.2) and do not exist in the registry yet:
| ID | Name | Visible When | Category |
|----|------|--------------|----------|
| `git.mergeContinue` | Continue Merge | Merge in progress | Git |
| `git.mergeAbort` | Abort Merge | Merge in progress | Git |
| `git.rebaseContinue` | Continue Rebase | Rebase in progress | Git |
| `git.rebaseAbort` | Abort Rebase | Rebase in progress | Git |
| `git.rebaseSkip` | Skip Commit | Rebase in progress | Git |
| `git.cherrypickContinue` | Continue Cherry-pick | Cherry-pick in progress | Git |
| `git.cherrypickAbort` | Abort Cherry-pick | Cherry-pick in progress | Git |
| `git.bisectGood` | Bisect Good | Bisect active | Git |
| `git.bisectBad` | Bisect Bad | Bisect active | Git |
| `git.bisectReset` | Bisect Reset | Bisect active | Git |

**Navigation**
| ID | Name | Shortcut | Category |
|----|------|----------|----------|
| `nav.focusSidebar` | Focus Sidebar | `F6` | Navigation |
| `nav.focusDiff` | Focus Diff Viewer | `Shift+F6` | Navigation |
| `nav.searchCommits` | Search Commits | `Ctrl+F` | Navigation |
| `nav.nextHunk` | Next Hunk | `]` | Navigation |
| `nav.prevHunk` | Previous Hunk | `[` | Navigation |

**View**
| ID | Name | Shortcut | Category |
|----|------|----------|----------|
| `view.toggleStaged` | Toggle Staged Section | | View |
| `view.toggleChanged` | Toggle Changed Section | | View |
| `view.toggleBranches` | Toggle Branches Section | | View |
| `view.toggleStashes` | Toggle Stashes Section | | View |

---

## 17. Large Repository Handling

### 17.1 File Indexing
For repositories with 100k+ files:
- Background indexing starts on repository open
- Show "Indexing files..." placeholder if search attempted before ready
- Index stored in memory; rebuilt on each session (fast for most repos)
- File watcher updates index incrementally on changes

### 17.2 Result Limiting
| Scope | Max Results | Overflow Behavior |
|-------|-------------|-------------------|
| Commands | 50 | Show all (list is bounded) |
| Files | 100 | "...and 1,234 more matches" with scroll-to-load |
| Branches | 100 | "...and 45 more branches" |
| Commits | 50 | Paginated, "Load more" on scroll to bottom |

### 17.3 Truncation
- File paths > 60 chars: Truncate middle with `...` (e.g., `src/.../deeply/nested/file.cpp`)
- Branch names > 40 chars: Truncate end with `...`, full name in tooltip
- Commit messages > 80 chars: Truncate end with `...`

---

## 18. Edge Cases

### 18.1 Repository States

| Scenario | Behavior |
|----------|----------|
| Empty repository (no commits) | Hide commit-related commands; show "Create Initial Commit" |
| Bare repository | Most commands hidden; read-only branch/commit browsing only |
| No repository open | Only app commands visible (Open, Clone, Recent); Git commands hidden |
| Corrupted .git | Show error in status bar; degrade gracefully to limited functionality |

### 18.2 Content Edge Cases

| Scenario | Behavior |
|----------|----------|
| Very long branch name | Truncate with ellipsis; full name in hover tooltip |
| Unicode branch/file names | Full UTF-8 support; ensure Segoe UI fallback for missing glyphs |
| Binary files in results | Show file, mark as "(binary)" in status area |
| Submodules | Show in file list with "submodule" indicator; clicking opens submodule path |

### 18.3 Network States

| Scenario | Behavior |
|----------|----------|
| Network unavailable | Fetch/Push/Pull available but will fail gracefully with error message |
| Remote unreachable | Show error in status bar; command remains in palette for retry |
| Auth required | Trigger auth dialog on command execution |

### 18.4 Performance Edge Cases

| Scenario | Behavior |
|----------|----------|
| >10,000 branches | Limit to 1000 most recent; show "Too many branches, type to filter" |
| >1M commits | Only search loaded commits; show "Searching recent history" |
| Slow file system | Show loading indicator after 100ms; don't block UI |

---

## 19. Accessibility

### 19.1 Keyboard Navigation
- All functionality accessible via keyboard
- Clear focus indicators on all interactive elements
- Focus ring: 2px solid `Theme::Accent`, 2px offset

### 19.2 Screen Reader Support
Implement UI Automation provider for:
- **Palette open**: Announce "Command palette, search, [N] results"
- **Selection change**: Announce selected item name and position ("Stage All, 1 of 5")
- **Group change**: Announce group name ("Commands group")
- **Mode change**: Announce new mode ("Searching branches")
- **No results**: Announce "No results found"
- **Results update**: Announce result count change

### 19.3 Live Regions
- Results list is an ARIA live region (polite)
- Status messages announced immediately

### 19.4 High Contrast Support
- Respect Windows High Contrast mode
- Ensure 4.5:1 contrast ratio for all text
- Use system colors when High Contrast is active

### 19.5 Motion Sensitivity
- Respect `prefers-reduced-motion` system setting
- Disable fade/slide animations when reduced motion preferred
- Instant show/hide as fallback

---

## 20. Implementation Notes

### 20.1 Widget Structure
The command palette should be implemented as a modal widget:

```
CommandPaletteWidget : Widget
├── InputRowWidget
│   ├── ModeBadgeWidget (optional, shows "Commands"/"Branches"/etc.)
│   └── TextInputWidget
├── BatchActionBarWidget (visible when multi-select active)
├── ResultsListWidget (virtualized for performance)
│   ├── GroupHeaderWidget
│   └── ResultItemWidget (repeated)
└── HintRowWidget (dismissible)
```

### 20.2 Performance Requirements
- Search must complete in <16ms for commands/branches/files
- Results list must be virtualized (only render visible items)
- File index must be cached on repository load
- Commit search uses debouncing (100ms) with loading indicator

### 20.3 Settings Storage

```json
{
  "commandPalette": {
    "recentCommands": ["git.fetch", "git.stageAll", "git.commit"],
    "aliases": {
      "yolo": "git.push"
    },
    "shortcuts": {
      "git.fetch": "Ctrl+G"
    },
    "showHints": true,
    "maxRecentCommands": 50
  }
}
```

**Note:** Using JSON for settings storage rather than fixed-size arrays for flexibility and consistency with the alias storage pattern.

---

## 21. Future Enhancements (Out of Scope for v1)

- **Command chaining**: Execute multiple commands in sequence
- **Fuzzy file preview**: Show file contents preview on selection
- **Command history search**: `Ctrl+R` style reverse search through command history
- **Custom commands**: User-defined command macros
- **Theming**: Custom badge colors per category
- **Submodule navigation**: Deep-link into submodule palettes
- **Stagger animations**: Results list items fade in with 50ms offset (polish)

---

## 22. References

- [VS Code Command Palette UX Guidelines](https://code.visualstudio.com/api/ux-guidelines/command-palette)
- [Designing Command Palettes](https://solomon.io/designing-command-palettes/)
- [Command Palette UX Patterns](https://medium.com/design-bootcamp/command-palette-ux-patterns-1-d6b6e68f30c1)
- [FZF Fuzzy Finder Algorithm](https://github.com/junegunn/fzf)
- [Windows UI Automation](https://docs.microsoft.com/en-us/windows/win32/winauto/entry-uiauto-win32)
