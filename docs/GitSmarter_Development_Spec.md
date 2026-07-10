# GitSmarter Development Specification

**Version:** 1.0 Draft  
**Last Updated:** December 2024

---

## 1. Project Overview

### 1.1 Vision
GitSmarter is a fast, focused Git client for Windows built with C++ and Direct2D. The goal is to create a tool that prioritizes performance, keyboard-driven workflows, and a clean UI that doesn't sacrifice information density.

### 1.2 Technical Stack
- **Language:** C++
- **Rendering:** Direct2D (vector graphics)
- **Platform:** Windows only
- **Dependencies:** Zero external dependencies (single executable)
- **Git Integration:** libgit2 or direct git CLI invocation (TBD)

### 1.3 Design Philosophy
- Performance is non-negotiable — must handle large repos (10k+ commits) smoothly
- Diff viewer is the primary workspace; history is reference
- Balanced information density — not sparse, not overwhelming
- Keyboard-first with full mouse support
- Progressive disclosure — show essentials, reveal detail on demand

---

## 2. Window & Chrome

### 2.1 Window Style
- Borderless window with custom title bar
- Windows 11 rounded corners and shadow (native)
- Native minimize/maximize/restore animations
- Custom fade+scale close animation
- Persist window size and position across sessions

### 2.2 Title Bar
- Height: 32px
- Layout: `[App Name] — [Repository Name]` (left) | `[Window Controls]` (right)
- App name: "GitSmarter" (12pt, semi-bold)
- Repository name: Folder name of open repo (11pt, secondary color)
- Window controls: Minimize, Maximize/Restore, Close (46px width each)
- Close button: Red hover state (#E81123)

### 2.3 Status Bar
- Height: 24px
- Background: Accent color (#0078D4)
- Content (left): Current branch icon + name, sync status (↑N ↓N)
- Content (right): Last fetch timestamp
- Font: 11pt, white

---

## 3. Color Theme

### 3.1 Core Palette
```
Background Primary:   #1E1E1E
Background Secondary: #252526
Background Tertiary:  #2D2D30
Background Elevated:  #333337
Hover:               #3E3E42
Border:              #3C3C3C

Text Primary:        #E0E0E0
Text Secondary:      #9D9D9D
Text Muted:          #6D6D6D

Accent:              #0078D4
Accent Hover:        #1C8AE6

Success (Added):     #4EC970
Danger (Removed):    #F14C4C
Warning:             #CCA700
Modified:            #519ABA
```

### 3.2 Branch Graph Colors (Rotating Palette)
```
Branch 1: #6B9FED (Blue)
Branch 2: #C678DD (Purple)
Branch 3: #E5C07B (Yellow)
Branch 4: #56B6C2 (Cyan)
Branch 5: #E06C75 (Red/Pink)
```

### 3.3 Selection States
- Selected item background: `{Accent}20` (accent at 12% opacity)
- Selected item border-left: 2px solid accent
- Hover: Background Hover color

---

## 4. Typography

### 4.1 Font Families
- **UI Font:** Segoe UI Variable (fallback: Segoe UI)
- **Monospace Font:** Cascadia Code (fallback: Consolas)

### 4.2 Usage
| Context | Font | Size | Weight |
|---------|------|------|--------|
| App title | UI | 12pt | 600 |
| Section headers | UI | 11pt | 600 |
| Body text | UI | 12pt | 400 |
| File names | Mono | 11pt | 400 |
| Commit hashes | Mono | 11pt | 400 |
| Diff content | Mono | 13pt | 400 |
| Keyboard hints | UI | 9pt | 400 |

---

## 5. Main Layout (Vertical Split)

### 5.1 Structure
```
+----------------------------------------------------------+
| Title Bar (32px)                           [_][□][×]     |
+------------+---------------------------------------------+
| Sidebar    |  Diff Viewer (flex: 6)                      |
| (240px)    |                                             |
|            +---------------------------------------------+
| - Branch   |  Commit History (flex: 4)                   |
| - Commit   |                                             |
| - Files    |  [Graph] [Commit List]                      |
| - Branches |                                             |
+------------+---------------------------------------------+
| Status Bar (24px)                                        |
+----------------------------------------------------------+
```

### 5.2 Panel Resizing
- All major panels are resizable via drag handles
- Sidebar: Min 180px, Max 400px, Default 240px
- Diff/History vertical split: Min 20% each, Default 60/40
- Persist user's panel sizes across sessions
- Drag handle visual: 4px hit area, 1px visible border, cursor change on hover

---

## 6. Sidebar

### 6.1 Layout (Top to Bottom)

#### Branch Selector
- Dropdown showing current branch
- Click to open branch picker
- Shows branch icon + branch name
- Padding: 10px 12px

#### Commit Box
- **Amend Indicator** (conditional): Yellow banner when amend mode active, with dismiss button
- **Message Textarea:**
  - Auto-expanding: Min 50px, Max 120px height
  - Grows based on line count (18px per line + 16px padding)
  - Placeholder: "Commit message..."
  - Keyboard hint: "Ctrl+Enter to commit" (bottom-right, muted)
  - **TODO:** Add clipboard paste support (Ctrl+V)
- **Split Commit Button:**
  - Main button: "Commit (N staged)" or "Amend Commit"
  - Disabled state when no staged files OR empty message
  - Dropdown with:
    - "Amend previous commit" (toggle, shows checkmark when active)
    - "Commit & Push" (with Ctrl+Shift+Enter hint)

#### Staged Files Section (Conditional)
- Only visible when ≥1 file is staged
- Header: "Staged (N)" with "−" button to unstage all
- File list with status badges
- Clicking "−" unstages all files

#### Changed Files Section
- Header: "Changed Files (N)" with "+" button to stage all  
- Shows unstaged files
- File list with status badges
- Clicking "+" stages all files

#### Branches Section
- Collapsible (default: expanded)
- Header: Icon + "Branches" + count badge
- List of local branches
- Active branch highlighted with accent background
- Shows ahead/behind counts: `↑N ↓N`

### 6.2 File List Items
- Layout: `[Status Badge] [Filename] [+N] [-N]`
- Status badge: 18×18px, rounded 3px, colored background at 20% opacity
  - M = Modified (blue)
  - A = Added (green)  
  - D = Deleted (red)
- Filename: Truncated with ellipsis, show just filename (not full path)
- Staged indicator: Green checkmark for staged files (when in combined view)
- Selection: Accent background + left border when selected
- Click to select and show diff

### 6.3 Collapsible Sections
- Click header to toggle
- Chevron icon rotates 90° when expanded
- Smooth height transition (150ms ease)
- Count badge in header (pill style, tertiary background)

---

## 7. Diff Viewer

### 7.1 Header Bar
- Background: Secondary
- Layout: `[Status Badge] [Filename + Path] | [+N -N] [Stage/Unstage Button]`
- Filename: 14pt, primary color, medium weight
- Path: 11pt, muted, monospace (shown on second line or inline)
- Stage button: Shows keyboard hint badge (S/U)

### 7.2 Diff Display Modes
- **Unified** (default): Single column, inline changes
- **Side-by-side**: Two columns, old left, new right
- Toggle in header or via keyboard shortcut

### 7.3 Unified Diff Rendering

#### Line Structure
```
[Old Line #] [New Line #] | [+/-/ ] [Content]
```
- Gutter width: 40px per line number column
- Line numbers: Right-aligned, muted color, 11pt mono
- Separator: 1px border between gutter and content
- Content padding-left: 12px

#### Line Styling
| Type | Background | Left Border | Prefix | Text Color |
|------|-----------|-------------|--------|------------|
| Context | transparent | transparent | (space) | Secondary |
| Added | Success 15% | 3px Success | + | Success |
| Removed | Danger 15% | 3px Danger | - | Danger |
| Hunk Header | Accent 15% | none | @@ | Accent |

#### Word-Level Diff Highlighting
- Within changed lines, highlight the specific words/characters that changed
- Added words: Stronger green background (Success 30%)
- Removed words: Stronger red background (Danger 30%)
- Use diff algorithm to identify minimal change regions within lines

### 7.4 Side-by-Side Diff Rendering
- Two equal-width columns with synchronized scrolling
- Left: Old version (removals highlighted)
- Right: New version (additions highlighted)
- Blank lines inserted to maintain alignment
- Line numbers on outer edges of each column

### 7.5 Inline Staging (Future)
- Click gutter to stage/unstage individual lines
- Click hunk header to stage/unstage entire hunk
- Visual indicator for partially staged files
- Priority: Lower (implement after core features)

### 7.6 Diff Font & Spacing
- Font: Cascadia Code / Consolas, 13pt
- Line height: 1.6
- Tab width: 4 spaces (configurable)

---

## 8. Commit History

### 8.1 Header Bar
- Height: 36px (increased to accommodate search box)
- Background: Secondary
- Layout: `HISTORY | N commits | [🔍 Search commits...]`
- Search box: Right-aligned, filters commits in real-time as user types
- Search matches against: message (substring), author name (substring), commit hash (prefix)
- Non-matching commits are hidden when filter is active
- Magnifying glass icon on right side of search box
- Shows "N of M commits" when filter is active

### 8.2 Commit Graph
- Rendered as vector graphics (Direct2D paths)
- Width: ~60px fixed
- Line weight: 2px
- Node radius: 4px (6px for HEAD)
- HEAD node: White stroke ring

#### Graph Rendering (Phase 1 - Simple)
- Vertical lines for branches
- Curved paths for merges (quadratic bezier)
- Colors from branch palette (rotating)
- Optimize for performance: only render visible portion + buffer

#### Graph Rendering (Phase 2 - Full DAG)
- Proper topological sorting
- Lane assignment algorithm
- Handle octopus merges
- Collapse long linear runs

### 8.3 Commit List
- Row height: 44px
- Layout:
  ```
  [Message - truncated with ellipsis]
  [Hash] • [Author] • [Relative Date]
  ```
- Message: 12pt, primary color
- Metadata: 10-11pt, muted color
- Hash: Monospace
- HEAD badge: Accent background pill
- Selection: Accent background + left border

### 8.4 Commit Selection Behavior
- Click to select and enter "commit view mode"
- Sidebar file sections replaced with commit's changed files
- Colored border around file area indicates viewing historical commit
- Click file to show its diff (comparing to parent commit)
- "×" button or Escape to exit commit view and return to working copy
- Double-click: Checkout? (TBD)
- Right-click: Context menu

---

## 9. Context Menus

### 9.1 File Context Menu (Changed Files)
- Stage / Unstage
- Discard Changes
- Open in Explorer
- Copy Path
- (Separator)
- Open in External Diff Tool (if configured)

### 9.2 File Context Menu (Staged Files)
- Unstage
- Open in Explorer
- Copy Path

### 9.3 Commit Context Menu
- Copy Commit Hash
- Copy Commit Message
- (Separator)
- Checkout Commit
- Create Branch Here
- Reset to This Commit → Soft / Mixed / Hard
- Revert Commit
- Cherry-pick Commit
- (Separator)
- Browse Files at Commit (future)

### 9.4 Branch Context Menu
- Checkout
- Merge into Current
- Rebase Current onto This
- (Separator)
- Rename
- Delete
- (Separator)
- Push (if has upstream)
- Set Upstream

### 9.5 Context Menu Styling
- Background: Elevated
- Border: 1px Border color
- Shadow: 0 4px 12px rgba(0,0,0,0.3)
- Border radius: 4px
- Item padding: 8px 12px
- Item hover: Hover background
- Separator: 1px border, 4px vertical margin
- Keyboard hint: Right-aligned, muted

---

## 10. Keyboard Shortcuts

### 10.1 Global
| Action | Shortcut |
|--------|----------|
| Command Palette | Ctrl+Shift+P |
| Focus Commit Message | Ctrl+M |
| Focus File List | Ctrl+1 |
| Focus Diff | Ctrl+2 |
| Focus History | Ctrl+3 |
| Quick Open File | Ctrl+P |
| Focus History Search | Ctrl+F |
| Refresh / Fetch | Ctrl+Shift+F |
| Toggle Sidebar | Ctrl+B |

### 10.2 Commit Actions
| Action | Shortcut |
|--------|----------|
| Commit | Ctrl+Enter |
| Commit & Push | Ctrl+Shift+Enter |
| Amend Toggle | Ctrl+Shift+A |
| Stage All | Ctrl+Shift+S |
| Unstage All | Ctrl+Shift+U |

### 10.3 File List
| Action | Shortcut |
|--------|----------|
| Stage Selected | S |
| Unstage Selected | U |
| Discard Changes | Delete or Backspace |
| Navigate Up/Down | ↑ / ↓ or J / K |

### 10.4 Diff Viewer
| Action | Shortcut |
|--------|----------|
| Toggle Unified/Split | Ctrl+D |
| Next Hunk | ] |
| Previous Hunk | [ |
| Stage Hunk (future) | Enter |
| Next File | Ctrl+↓ |
| Previous File | Ctrl+↑ |

### 10.5 History
| Action | Shortcut |
|--------|----------|
| Pull | Ctrl+Shift+P |
| Push | Ctrl+Shift+U |
| Navigate Commits | ↑ / ↓ or J / K |

---

## 11. Git Operations

### 11.1 Core Operations (v1)
| Operation | Implementation Notes |
|-----------|---------------------|
| **Status** | Poll or watch for changes, update file list |
| **Stage** | Add files to index |
| **Unstage** | Remove files from index |
| **Commit** | Create commit with message |
| **Amend** | Amend previous commit |
| **Push** | Push to remote |
| **Pull** | Pull from remote (fetch + merge) |
| **Fetch** | Fetch from remote |
| **Branch** | Create, delete, rename branches |
| **Checkout** | Switch branches or commits |
| **Merge** | Merge branches |
| **Rebase** | Rebase current branch |
| **Stash** | Stash/unstash changes |
| **Reset** | Soft, mixed, hard reset |
| **Revert** | Revert commits |
| **Cherry-pick** | Cherry-pick commits |
| **Discard** | Discard working directory changes |

### 11.2 Deferred (Post-v1)
- Conflict resolution UI
- Interactive rebase
- Submodule support
- Worktree support
- Blame view
- File history view

### 11.3 Remote Operations
- Support multiple remotes
- Push with upstream tracking
- Force push (with confirmation)
- Prune stale remote branches

---

## 12. Search & Navigation

### 12.1 History Search (Ctrl+F)
- Search box integrated in history panel header
- Real-time filtering as user types
- Matches against: message (substring), author (substring), hash (prefix)
- Non-matching commits are hidden
- Shows "N of M commits" when filter active
- Escape clears search and returns focus to commit list

### 12.2 File Search (Ctrl+P) — Lower Priority
- Quick open dialog
- Search all files in repo
- Fuzzy matching
- Show file in diff view on selection

### 12.3 Command Palette (Ctrl+Shift+P) — Lower Priority
- Search all available commands
- Show keyboard shortcuts
- Execute commands

---

## 13. Welcome / Repository Selector

### 13.1 Layout
- Centered content
- Logo + App name at top
- Action buttons: "Open Repository" | "Clone Repository"
- Recent repositories list below

### 13.2 Recent Repositories List
- Persist list across sessions
- Show: Repo name, full path, last opened timestamp
- Hover state with chevron
- Click to open
- Right-click: Remove from list, Open in Explorer
- Max ~10 recent items

### 13.3 Open Repository
- Native folder picker dialog
- Validate selected folder contains .git
- Error message if not a git repository

### 13.4 Clone Repository (Future)
- URL input
- Destination folder picker
- Progress indication
- Authentication handling (HTTPS / SSH)

---

## 14. Settings & Persistence

### 14.1 Persisted State
- Window position and size
- Panel sizes (sidebar width, diff/history split)
- Recent repositories list
- Last opened repository (auto-reopen option)
- User preferences

### 14.2 User Preferences (Future)
- Default diff mode (unified/split)
- Tab width
- Font size adjustments
- External diff tool path
- External editor path

### 14.3 Storage
- Use Windows registry or local JSON file in AppData
- Location: `%APPDATA%\GitSmarter\`

---

## 15. Performance Requirements

### 15.1 Targets
| Metric | Target |
|--------|--------|
| Startup time (empty) | < 500ms |
| Startup time (open repo) | < 1s for repos up to 10k commits |
| Diff rendering | < 100ms for files up to 10k lines |
| Commit graph | < 200ms for 10k commits |
| UI responsiveness | 60 FPS during scroll/interaction |
| Memory footprint | < 100MB for typical usage |

### 15.2 Optimization Strategies
- Virtualized lists (only render visible items + buffer)
- Lazy loading of commit details
- Background thread for git operations
- Incremental graph rendering
- Cache parsed diff data
- Debounce file system watches

### 15.3 Large Repository Handling
- Pagination for commit history (load more on scroll)
- Truncate very long diffs with "Show more" option
- Async loading with progress indicators

---

## 16. Implementation Phases

See **Section 18: Progress Tracking** for current status of each phase.

---

## 17. Progress Tracking

### Phase 1: Foundation
- [x] Window chrome and basic layout
- [x] Theme system and color constants
- [x] Panel rendering and resizing
- [x] Basic typography and text rendering
- [x] Status bar with branch and fetch info

### Phase 2: Core Git Integration
- [x] Repository loading and validation
- [x] Status polling / file watching (read-only)
- [x] File list display (staged/unstaged)
- [x] File staging/unstaging (write operations)
- [x] Basic commit functionality
- [x] Fetch operations (Smart HTTP Protocol v2, async, progress UI, background sync)
- [x] Push operations (Smart HTTP Protocol v2, pack generation, fast-forward check)
- [x] Pull operations (fast-forward only, tree checkout, index rebuild)

### Phase 3: Diff Viewer
- [x] Unified diff parsing and rendering
- [x] Line number gutters
- [x] Add/remove/context line styling
- [x] Scrolling with viewport clipping (virtualized)
- [x] Word-level diff highlighting
- [ ] Side-by-side mode

### Phase 4: Commit History
- [x] Commit list rendering
- [x] Simple linear graph (with lane assignment)
- [x] Commit selection
- [x] History search/filter (real-time filtering in header, replaces jump-to-commit)
- [x] Commit view mode (sidebar shows commit's changed files)

### Phase 5: Polish & UX
- [ ] Keyboard navigation
- [ ] Context menus (partial: welcome screen only)
- [x] Welcome screen and recent repos
- [x] Settings persistence (recent repos)
- [ ] Split commit button with amend

### Phase 6: Advanced Features
- [ ] Full DAG graph rendering
- [ ] Branch management UI
- [ ] Stash support
- [ ] Command palette
- [ ] Search functionality
- [ ] Inline hunk staging

---

## 18. Code-Level TODOs

These are incomplete implementations marked in the source code:

### Network/Fetch (network.cpp)
- [ ] **Credential helper invocation** - Currently only reads GH_TOKEN/GITHUB_TOKEN env vars; should support git credential helpers
- [ ] **Pack index generation** - Currently relies on git CLI for indexing received packs; should generate .idx files natively

### Platform/UI (platform.cpp)
- [ ] **Commit message paste support (Ctrl+V)** - Textarea doesn't handle clipboard paste
- [ ] **Git config editing** - More sophisticated config file editing for user identity
- [ ] **Branch checkout on click** - Clicking branch in sidebar should checkout
- [ ] **Commit box cursor positioning** - Click should position cursor at click location
- [ ] **History search box text scrolling** - Text should be clipped within the textbox and scroll horizontally so the cursor never leaves the right edge of the control

### Core Git (core.cpp)
- [ ] **packed-refs parsing** - Should also parse packed-refs for refs not in loose files

### Resumable Fetch
- [ ] **HTTP Range resume** - State infrastructure exists but actual Range header resume not implemented

---

## 19. Open Questions

1. ~~**Git Backend:** libgit2 vs shelling out to git CLI?~~ **RESOLVED:** Direct git parsing with no dependencies.

2. **Diff Algorithm:** Use git's diff output or implement own? Own implementation allows word-diff but adds work.

3. ~~**File Watching:** Poll vs native file system events?~~ **RESOLVED:** Polling approach used.

4. ~~**Authentication:** How to handle SSH keys and credential storage?~~ **PARTIAL:** HTTPS only with env var tokens (GH_TOKEN). SSH and credential helpers not yet supported.

5. ~~**Auto-fetch:** Should we periodically fetch in the background?~~ **RESOLVED:** Yes, implemented with 5-minute background sync timer.

---

## Appendix A: Reference Mockup

See accompanying `gitsmarter-mockups.jsx` file for interactive React mockups demonstrating:
- Vertical split layout (Option A)
- Sidebar with commit box placement
- Diff viewer with word-level highlighting
- Commit history with graph
- Welcome screen

---

## Appendix B: Competitive Analysis

| Feature | GitHub Desktop | GitKraken | Fork | Sublime Merge | GitSmarter |
|---------|---------------|-----------|------|---------------|------------|
| Performance | Good | Medium | Excellent | Excellent | Target: Excellent |
| Graph Visualization | None | Full DAG | Full DAG | Full DAG | Phased |
| Keyboard-driven | Limited | Limited | Good | Excellent | Target: Excellent |
| Word-diff | No | Yes | Yes | Yes | Yes |
| Side-by-side diff | No | Yes | Yes | Yes | Yes |
| Inline staging | Yes | Yes | Yes | Yes | Future |
| Price | Free | Paid | Paid | Paid | Free |

---

*End of Specification*
