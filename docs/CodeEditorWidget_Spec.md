# Code Editor Widget Specification

## Overview

A fully-featured, high-performance text editor widget for GitSmarter built on Direct2D and the existing widget framework. Designed for code file editing with syntax highlighting, virtualized rendering for unlimited file sizes, and future extensibility for merge conflict resolution.

## Design Principles

Following GitSmarter's engineering principles:
- **Tackle complexity head-on**: Piece table data structure, virtualized rendering, smart indentation
- **Optimize ruthlessly**: Zero-copy memory-mapped files, O(1) viewport calculation, widget pooling
- **Question dependencies**: No external editor libraries - built from scratch using DirectWrite
- **Craft exceptional UIs**: Smooth animations, polished visual feedback, modern overlay scrollbars

---

## Architecture

### Component Overview

```
CodeEditorWidget (container)
├── GutterWidget (line numbers, git indicators)
├── ContentWidget (text rendering, cursor, selection)
├── HScrollbarWidget (overlay, horizontal)
└── VScrollbarWidget (overlay, vertical)
```

### Data Flow

```
TextDocument (owns file lifecycle)
    └── PieceTable (edit model)
            └── LineIndex (virtualization index)
                    └── CodeEditorWidget (rendering)
```

### File Structure

```
src/widgets/code_editor/
├── code_editor.h           # Public API, CodeEditorWidget class
├── code_editor.cpp         # Main widget, composition, events
├── piece_table.h           # PieceTable data structure
├── piece_table.cpp         # Edit operations, undo/redo
├── text_document.h         # TextDocument lifecycle management
├── text_document.cpp       # File I/O, encoding, dirty tracking
├── line_index.h            # LineIndex for virtualization
├── line_index.cpp          # Line position tracking
├── gutter_widget.cpp       # Line numbers, git indicators
├── content_widget.cpp      # Text rendering, cursor, selection
├── editor_theme.h          # Extensible theme system
└── editor_theme.cpp        # Default theme matching GitSmarter
```

---

## Core Components

### 1. PieceTable

The edit model optimized for large files and memory efficiency.

**Structure:**
```cpp
enum class PieceSource { Original, Append };

struct Piece {
    PieceSource source;
    size_t offset;      // Offset into source buffer
    size_t length;      // Length in bytes (UTF-8)
};

class PieceTable {
    const char* original;           // Memory-mapped or loaded (immutable)
    size_t original_length;
    std::vector<char> append_buf;   // All insertions accumulate here
    std::vector<Piece> pieces;      // The piece table

    // Undo/redo via piece table snapshots
    struct Snapshot {
        std::vector<Piece> pieces;
        size_t append_size;         // Size of append_buf at snapshot
        size_t cursor_pos;
        size_t selection_start;
        size_t selection_end;
    };
    std::vector<Snapshot> undo_stack;
    size_t undo_position;           // Current position in undo history
};
```

**Operations:**
- `insert(pos, text, len)` - Insert text at byte position
- `delete(pos, len)` - Delete range
- `get_text(pos, buf, len)` - Extract text range
- `total_length()` - Current document length
- `push_undo()` - Snapshot current state
- `undo()` / `redo()` - Navigate history

**Undo Coalescing:**
- Sequential single-character inserts within 1 second coalesce into one undo entry
- Explicit undo boundary on: cursor movement, selection change, paste, delete

### 2. TextDocument

Document lifecycle and file management.

**Structure:**
```cpp
class TextDocument {
    PieceTable content;
    MemoryMap* mmap;                // nullptr if string-backed
    wchar_t source_path[MAX_PATH]; // Empty if in-memory
    bool dirty;

    // Encoding
    enum Encoding { UTF8, UTF8_BOM };
    Encoding encoding;
    bool has_bom;

    // Line endings
    enum LineEnding { LF, CRLF, CR };
    LineEnding line_ending;         // Detected on load, preserved on save
};
```

**Creation Methods:**
```cpp
static TextDocument* from_string(const char* utf8_text);
static TextDocument* from_file(const wchar_t* path);
// Future: from_mmap(MemoryMap* mmap) for external mmap management
```

**File Operations:**
- `save()` - Write to source_path (or prompt if none)
- `save_as(path)` - Write to new path
- `reload()` - Discard changes, reload from disk
- `is_dirty()` - Has unsaved changes

**Validation:**
- UTF-8 validation on load
- Invalid byte sequences replaced with U+FFFD or rejected with error callback

### 3. LineIndex

Enables O(1) visible range calculation for virtualization.

**Structure:**
```cpp
struct LineEntry {
    size_t byte_offset;     // Byte offset of line start in document
    size_t line_number;     // 1-indexed line number
    uint16_t piece_index;   // Hint for faster piece lookup
    uint16_t flags;         // Git status (added/modified/removed)
};

class LineIndex {
    std::vector<LineEntry> lines;
    size_t total_lines;

    // Rebuild tracking
    bool needs_rebuild;
    size_t rebuild_from_line;   // Incremental rebuild starting point
};
```

**Operations:**
- `build(PieceTable*)` - Full rebuild (on load)
- `update(edit_pos, delta)` - Incremental update after edit
- `get_line(line_num)` - O(1) access by line number
- `get_visible_range(scroll_y, height)` - O(1) first/last visible line
- `byte_to_line(byte_offset)` - Binary search for line containing offset

**Optimization:**
- Fixed line height enables direct math: `first_line = scroll_y / LINE_HEIGHT`
- Incremental updates only rebuild affected lines after edits
- Line entries are 16 bytes for cache efficiency

### 4. GutterWidget

Left margin with line numbers and indicators.

**Features:**
- Line numbers (configurable width based on max line count)
- Git diff indicators (colored bar: green=added, blue=modified, red=removed)
- Click to select entire line
- Current line highlight

**Rendering:**
- Only render visible line numbers (virtualized)
- Cache line number text layouts by value (reuse "42" layout for all line 42s)
- Right-aligned numbers with consistent width

### 5. ContentWidget

Main text editing area.

**Features:**
- Text rendering with syntax highlighting
- Cursor rendering with smooth blink animation (fade, not hard toggle)
- Selection highlight with animated transitions
- Horizontal scrolling for long lines
- Bracket matching highlight

**Text Rendering Pipeline:**
1. Get visible line range from LineIndex
2. For each visible line:
   - Extract UTF-8 from PieceTable
   - Convert to UTF-16 for DirectWrite
   - Apply syntax highlighting spans
   - Create/reuse IDWriteTextLayout
   - Render with clip to viewport

**Viewport-Aware Long Line Handling:**
- For lines exceeding viewport width:
  - Create layout only for visible portion + margin
  - Track actual content width from full measurement (done once on load/edit)
  - Horizontal scroll reveals hidden portions
  - Cursor movement triggers scroll to keep cursor visible

### 6. Syntax Highlighting

Reuse existing syntax.cpp infrastructure.

**Integration:**
```cpp
// Existing SyntaxSpan from app.h
struct SyntaxSpan {
    uint16_t start;     // UTF-16 code unit offset
    uint16_t length;
    SyntaxToken token;  // Keyword, Type, String, Comment, etc.
};

// Per-line highlighting (computed on demand)
void highlight_line(const char* text, size_t len,
                    const char* file_extension,
                    SyntaxSpan* spans, size_t* span_count);
```

**Caching:**
- Cache syntax spans per line
- Invalidate on line modification
- Background re-highlight for large files (non-blocking)

### 7. EditorTheme

Extensible theming system.

**Structure:**
```cpp
struct EditorTheme {
    // Background colors
    uint32_t background;
    uint32_t current_line_bg;
    uint32_t selection_bg;
    uint32_t gutter_bg;

    // Text colors
    uint32_t text;
    uint32_t line_number;
    uint32_t line_number_active;

    // Syntax colors (map SyntaxToken -> color)
    uint32_t syntax_colors[16];

    // Cursor
    uint32_t cursor;
    float cursor_width;

    // Git indicators
    uint32_t git_added;
    uint32_t git_modified;
    uint32_t git_removed;

    // Scrollbar
    uint32_t scrollbar_track;
    uint32_t scrollbar_thumb;
    uint32_t scrollbar_thumb_hover;

    // Bracket matching
    uint32_t bracket_match_bg;
    uint32_t bracket_match_border;
};

// Built-in themes
extern const EditorTheme THEME_GITSMARTER;   // Matches app theme
extern const EditorTheme THEME_DARK;         // VS Code dark style
extern const EditorTheme THEME_LIGHT;        // Light theme option
```

---

## Interactions

### Keyboard

**Navigation:**
| Key | Action |
|-----|--------|
| Arrow keys | Move cursor |
| Ctrl+Left/Right | Word navigation |
| Home/End | Line start/end |
| Ctrl+Home/End | Document start/end |
| Page Up/Down | Scroll by viewport |

**Selection:**
| Key | Action |
|-----|--------|
| Shift+Arrow | Extend selection |
| Ctrl+Shift+Arrow | Extend by word |
| Ctrl+A | Select all |

**Editing:**
| Key | Action |
|-----|--------|
| Characters | Insert at cursor |
| Backspace | Delete before cursor |
| Delete | Delete at cursor |
| Ctrl+Backspace | Delete word before |
| Ctrl+Delete | Delete word after |
| Enter | New line with smart indent |
| Tab | Insert indent (spaces or tab based on file) |
| Shift+Tab | Outdent current line |

**Clipboard:**
| Key | Action |
|-----|--------|
| Ctrl+C | Copy selection |
| Ctrl+X | Cut selection |
| Ctrl+V | Paste |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |

**Brackets:**
| Key | Action |
|-----|--------|
| `(`, `[`, `{`, `"`, `'` | Auto-insert closing bracket |
| `)`, `]`, `}` | Skip over if next char matches |

### Mouse

**Click:**
- Single click: Position cursor
- Double click: Select word
- Triple click: Select line
- Shift+click: Extend selection from anchor

**Drag:**
- Character-level selection (single click start)
- Word-level selection (double click start)

**Gutter:**
- Click line number: Select entire line
- Drag in gutter: Select multiple lines

**Scrollbars:**
- Drag thumb: Scroll proportionally
- Click track: Page scroll
- Mouse wheel: Vertical scroll (Shift+wheel for horizontal)

### Selection Behaviors (from TextInput)

- Smooth cursor position animation (40ms ease-out)
- Smooth selection highlight animation (40ms ease-out)
- Cursor blink with fade (not hard toggle)
- 500ms pause after movement before blink resumes

---

## Events API

```cpp
// Callback signatures
using OnChangeCallback = void(*)(CodeEditorWidget*, void* user_data);
using OnCursorMoveCallback = void(*)(CodeEditorWidget*, size_t line, size_t col, void* user_data);
using OnSelectionChangeCallback = void(*)(CodeEditorWidget*, size_t start, size_t end, void* user_data);
using OnSaveCallback = void(*)(CodeEditorWidget*, bool success, void* user_data);
using OnDirtyChangedCallback = void(*)(CodeEditorWidget*, bool dirty, void* user_data);
using OnKeyPressCallback = bool(*)(CodeEditorWidget*, int vk, bool ctrl, bool shift, bool alt, void* user_data); // Return true to cancel
using OnContextMenuCallback = void(*)(CodeEditorWidget*, int x, int y, ContextMenuBuilder*, void* user_data);
using OnCloseRequestedCallback = bool(*)(CodeEditorWidget*, void* user_data); // Return false to cancel close

class CodeEditorWidget {
    // Event registration
    void set_on_change(OnChangeCallback cb, void* user_data);
    void set_on_cursor_move(OnCursorMoveCallback cb, void* user_data);
    void set_on_selection_change(OnSelectionChangeCallback cb, void* user_data);
    void set_on_save(OnSaveCallback cb, void* user_data);
    void set_on_dirty_changed(OnDirtyChangedCallback cb, void* user_data);
    void set_on_key_press(OnKeyPressCallback cb, void* user_data);
    void set_on_context_menu(OnContextMenuCallback cb, void* user_data);
    void set_on_close_requested(OnCloseRequestedCallback cb, void* user_data);
};
```

### Context Menu Extension

```cpp
class ContextMenuBuilder {
    void add_item(const char* label, void(*action)(void*), void* user_data);
    void add_separator();
    void add_submenu(const char* label, ContextMenuBuilder* submenu);
};

// Default items added automatically:
// - Undo (if can_undo)
// - Redo (if can_redo)
// - separator
// - Cut (if has_selection)
// - Copy (if has_selection)
// - Paste (if clipboard has text)
// - separator
// - Select All
// Parent can add more via on_context_menu callback
```

---

## Programmatic API

```cpp
class CodeEditorWidget {
    // Document management
    void set_document(TextDocument* doc);
    TextDocument* get_document();

    // Content access
    size_t get_text(char* buf, size_t buf_size);  // Returns bytes written
    void set_text(const char* utf8_text);

    // Cursor
    void set_cursor_position(size_t line, size_t col);
    void get_cursor_position(size_t* line, size_t* col);

    // Selection
    void set_selection(size_t start_line, size_t start_col, size_t end_line, size_t end_col);
    void get_selection(size_t* start_line, size_t* start_col, size_t* end_line, size_t* end_col);
    bool has_selection();
    void select_all();
    void clear_selection();

    // Editing
    void insert_text(const char* utf8_text);
    void delete_selection();
    void undo();
    void redo();
    bool can_undo();
    bool can_redo();

    // Clipboard
    void copy();
    void cut();
    void paste();

    // Navigation
    void scroll_to_line(size_t line);
    void scroll_to_position(size_t line, size_t col);
    void ensure_visible(size_t line, size_t col);

    // View state
    size_t get_first_visible_line();
    size_t get_last_visible_line();
    size_t get_line_count();

    // Theme
    void set_theme(const EditorTheme* theme);

    // Options
    void set_read_only(bool read_only);
    bool is_read_only();
    void set_show_whitespace(bool show);
    bool get_show_whitespace();
    void set_tab_size(int spaces);
    int get_tab_size();

    // Git indicators (for gutter)
    void set_line_git_status(size_t line, GitLineStatus status);
    void clear_git_status();

    // Background highlighting (for merge editor)
    void add_highlight_range(size_t start_line, size_t end_line, uint32_t bg_color);
    void clear_highlights();
};
```

---

## Implementation Phases

### Phase 1: Foundation ✓ COMPLETE
- [x] Create `src/widgets/code_editor/` directory structure
- [x] Implement PieceTable with basic insert/delete
- [x] Implement TextDocument with string-backed content
- [x] Basic unit tests for PieceTable operations (46 tests)

### Phase 2: Line Index & Virtualization ✓ COMPLETE
- [x] Implement LineIndex with full build
- [x] Add incremental LineIndex updates
- [x] Create basic ContentWidget with fixed-height lines
- [x] Implement widget pooling for visible lines (slot-based)
- [x] Verify virtualization with 100K+ line test file (30 tests)

### Phase 3: Basic Rendering ✓ COMPLETE
- [x] Implement line text rendering (no highlighting)
- [x] Add GutterWidget with line numbers
- [x] Implement vertical scrolling
- [x] Add scroll position caching

### Phase 4: Cursor & Basic Editing ✓ COMPLETE
- [x] Implement cursor positioning (click to place)
- [x] Add cursor rendering with blink animation
- [x] Implement character insertion
- [x] Implement backspace/delete
- [x] Add keyboard arrow navigation
- [x] Add Home/End, Ctrl+Home/End, Page Up/Down navigation
- [x] Add basic undo/redo (Ctrl+Z, Ctrl+Y)

### Phase 5: Selection ✓ COMPLETE
- [x] Implement click-drag selection
- [x] Add shift+click extend selection
- [x] Implement double-click word select
- [x] Implement triple-click line select
- [x] Add selection rendering
- [x] Implement Ctrl+A select all

### Phase 6: Undo/Redo ✓ COMPLETE
- [x] Implement undo stack with snapshots (done in Phase 1)
- [x] Add undo coalescing for typing (1-second timer groups sequential inserts)
- [x] Implement redo (done in Phase 1)
- [x] Wire Ctrl+Z / Ctrl+Y (done in Phase 4)

### Phase 7: Clipboard ✓ COMPLETE
- [x] Implement copy to clipboard
- [x] Implement paste from clipboard
- [x] Implement cut (copy + delete)

### Phase 8: Word Operations ✓ COMPLETE
- [x] Implement word boundary detection (done in Phase 5 for double-click word select)
- [x] Add Ctrl+Left/Right word navigation (done - crosses line boundaries)
- [x] Add Ctrl+Backspace/Delete word deletion
- [x] Add Ctrl+Shift+Arrow word selection (done - extends selection by word)

### Phase 9: Horizontal Scrolling
- [ ] Implement horizontal scroll offset
- [ ] Add viewport-aware long line rendering
- [ ] Implement horizontal scrollbar (overlay style)
- [ ] Add auto-scroll to keep cursor visible

### Phase 10: Syntax Highlighting
- [ ] Integrate with existing syntax.cpp
- [ ] Implement per-line syntax span caching
- [ ] Add syntax highlighting to rendering pipeline
- [ ] Support language detection by file extension

### Phase 11: Smart Features
- [ ] Implement smart indent on Enter
- [ ] Add bracket auto-close
- [ ] Implement bracket matching highlight
- [ ] Add closing bracket skip-over

### Phase 12: Theming
- [ ] Implement EditorTheme structure
- [ ] Create THEME_GITSMARTER matching app theme
- [ ] Wire theme colors throughout rendering
- [ ] Add theme switching API

### Phase 13: Git Integration
- [ ] Add git status field to LineEntry
- [ ] Implement gutter indicator rendering
- [ ] Create API for external git status updates

### Phase 14: File I/O
- [ ] Implement file loading with mmap
- [ ] Add UTF-8 validation
- [ ] Implement save functionality
- [ ] Add dirty tracking and change notification
- [ ] Implement line ending detection/preservation

### Phase 15: Context Menu
- [ ] Implement ContextMenuBuilder
- [ ] Add default menu items
- [ ] Wire on_context_menu callback
- [ ] Integrate with existing ContextMenuWidget

### Phase 16: Whitespace Display
- [ ] Implement space dot rendering
- [ ] Implement tab arrow rendering
- [ ] Add toggle API
- [ ] Wire to settings

### Phase 17: Polish & Integration
- [ ] Performance profiling with large files
- [ ] Memory usage optimization
- [ ] Integration testing in GitSmarter
- [ ] Documentation and examples

---

## Performance Targets

| Metric | Target |
|--------|--------|
| Initial load (10K lines) | < 50ms |
| Initial load (100K lines) | < 200ms |
| Initial load (1M lines) | < 2s |
| Keystroke latency | < 16ms (60fps) |
| Scroll latency | < 8ms (120fps capable) |
| Memory (1M line file) | < 50MB heap |
| Memory (mmap backing) | Near-zero for original content |

---

## Future Considerations (Not in MVP)

- Multi-cursor editing (design data structures to support)
- Code folding (syntax-aware regions)
- Minimap sidebar
- Search/replace with regex
- Full accessibility (UI Automation)
- Vim mode
- Language server integration
- Auto-complete / IntelliSense

---

## Testing Strategy

### Unit Tests
- PieceTable: insert, delete, edge cases, large operations
- LineIndex: build, incremental update, visible range
- TextDocument: load, save, encoding, line endings

### Integration Tests
- Widget rendering with mocked content
- Keyboard event handling
- Mouse interaction
- Scroll behavior

### Manual Testing
- Open various file types and sizes
- Edit, undo, redo cycles
- Selection behaviors
- Syntax highlighting accuracy
- Performance with large files

---

## Implementation Notes & Learnings

### Phase 1 Learnings (2026-01-10)

**PieceTable get_text() Bug:**
The initial implementation of `get_text()` had a subtle bug when reading text spanning multiple pieces. The position variable `pos` was being modified inside the iteration loop while also being used in limit calculations, causing double-counting of bytes read.

*Symptom:* Reading "hello world" (11 chars across 2 pieces) returned only "hello " (6 chars).

*Fix:* Rewrote `get_text()` with clearer logic:
1. Calculate `max_to_read` upfront (min of remaining doc length and buffer size)
2. Use separate `doc_offset` variable to track position in document
3. Skip pieces entirely before start position with `continue`
4. Calculate bytes to copy per-piece without modifying loop state

**PieceTable Design Decisions:**
- Used `std::vector<char>` for append buffer (simple, cache-friendly for sequential writes)
- Used `std::vector<Piece>` for piece table (random access needed, insertions relatively rare)
- Snapshot-based undo stores copy of pieces vector + append buffer size (O(pieces) memory per snapshot)
- Line operations scan pieces sequentially - acceptable for Phase 1, may need caching for Phase 2+

**TextDocument Line Ending Detection:**
- Scans content on load to detect first line ending style (LF, CRLF, or CR)
- Defaults to LF if no line endings found
- Preserves detected style for future save operations

**Unity Build Integration:**
- Added includes to `src/main.cpp` in the UI Framework section
- Added includes to `test/test_main.cpp` as new Tier 16
- No header changes to `app.h` needed since PieceTable/TextDocument are self-contained in their namespace

### Phase 2 Learnings (2026-01-10)

**LineIndex Design:**
- 1-indexed line numbers (matches UI convention, lines start at 1)
- `LineEntry` is 16 bytes for cache efficiency: `{byte_offset, line_number, piece_hint, flags}`
- O(1) viewport calculation with fixed line height: `first_visible = scroll_y / line_height + 1`
- O(log n) binary search for `byte_to_line()` reverse lookup
- Incremental updates adjust byte offsets and insert/remove line entries

**LineIndex `last_visible_line()` Edge Case:**
The initial implementation double-counted partially visible lines. When viewport ends exactly on a line boundary (e.g., scroll=0, height=60, line_height=20), line 4's top is at 60px and shouldn't be included.

*Fix:* Only add +1 for partial visibility when there's a fractional remainder:
```cpp
float remainder = bottom_edge - (line_num * line_height_);
if (remainder > 0.001f && line_num < total_lines_) {
    line_num++;
}
```

**ContentWidget Slot-Based Pooling:**
- Fixed array of `LineSlot[128]` instead of dynamic widget creation
- `line_number == 0` indicates free slot
- `bind_line()` / `unbind_line()` manage slot lifecycle
- Each slot caches: UTF-16 text, IDWriteTextLayout*, actual rendered width
- No dynamic allocation during scroll - all memory pre-allocated

**Unity Build Extern Declarations:**
Global resources (g_dwrite_factory, g_text_format_mono) are defined in platform.cpp in the global namespace. Extern declarations in CodeEditor namespace files must be outside the namespace block:
```cpp
// Correct: Outside namespace
extern IDWriteFactory* g_dwrite_factory;

namespace CodeEditor {
    // Use g_dwrite_factory here
}
```

**Performance:**
- 100K line LineIndex build: < 200ms (test validates this)
- 10K byte_to_line() lookups: < 10ms total (< 1us per lookup)
- ContentWidget only binds visible lines (typically 50-100 at most)

### Phase 3 Learnings (2026-01-10)

**CodeEditorWidget Container Pattern:**
- Main container composes GutterWidget + ContentWidget + inline scrollbar
- Children created via `g_widget_arena.create<T>()` in constructor
- Layout calculates gutter width dynamically based on line count digits
- Scrollbar rendered inline (not a separate widget) following ScrollContainer pattern

**GutterWidget Line Number Rendering:**
- Right-aligned text via `DWRITE_TEXT_ALIGNMENT_TRAILING` on TextLayout
- Dynamic width calculation: `PADDING_LEFT + (digits * DIGIT_WIDTH) + PADDING_RIGHT`
- Minimum 3 digits for visual consistency
- Current line highlight via background fill before text rendering

**Scrollbar Implementation:**
- Follows existing ScrollContainer pattern from ui_widgets_dialog.cpp
- Track/thumb/button geometry cached in `update_scrollbar_geometry()`
- Thumb size proportional to visible/total content ratio
- Thumb position tracks scroll position proportionally
- Mouse drag handled via `scrollbar_dragging_` flag and delta calculation

**Scroll Position Caching:**
- Fixed-size cache (32 entries) with LRU eviction
- Cache keyed by `TextDocument*` pointer
- Position cached on document switch-out, restored on switch-in
- Enables seamless navigation between multiple open documents

**Test Build Consideration:**
- Widget files (content_widget.cpp, gutter_widget.cpp, code_editor.cpp) excluded from test build
- They depend on Direct2D globals (g_dwrite_factory, g_text_format_mono) from platform.cpp
- Data structure tests (PieceTable, LineIndex, TextDocument) remain in test build

**Scrollbar Inline Rendering vs Reusable Widget:**
- CodeEditorWidget renders its scrollbar inline (not using ScrollContainer or separate VScrollbarWidget)
- This follows the pattern established by DiffViewerWidget which also renders scrollbars inline
- Reasons for current approach:
  1. **Dual-widget sync**: CodeEditorWidget must keep GutterWidget and ContentWidget scroll positions synchronized; ScrollContainer assumes single scrollable child
  2. **Custom layout**: Gutter has dynamic width based on line count; scrollbar positioning needs to account for this
  3. **Event routing**: Mouse events need coordinated handling across gutter, content, and scrollbar areas
- Code duplication exists between ScrollContainer, DiffViewerWidget, and CodeEditorWidget scrollbar rendering
- **Future refactoring opportunity**: Extract a `VScrollbarRenderer` helper class or standalone `VScrollbarWidget` that can be composed into complex layouts without assuming container ownership of scroll state

**Mouse Event Coordinate Systems:**
- Widget event handlers normally receive LOCAL coordinates (0 to width/height range)
- Scrollbar geometry (track_top_, thumb_top_, etc.) stored in ABSOLUTE screen coordinates (set during layout/render)
- Must convert local → absolute in handlers: `float abs_x = x + rect.x; float abs_y = y + rect.y;`
- This mismatch caused initial scrollbar interaction bugs where clicks weren't being detected correctly

**Mouse Capture for Scrollbar Drag:**
- Set `WidgetFlags::CapturesMouse` flag in constructor for widgets with scrollbar drag
- When captured, EventDispatcher sends ABSOLUTE coordinates to `on_mouse_move`, not relative
- Pattern: `bool is_captured = scrollbar_dragging_; float abs_y = is_captured ? y : (y + rect.y);`
- Without capture, dragging stops when mouse leaves the scrollbar area (even with button held)
- EventDispatcher clears capture on mouse_up automatically

### Phase 4 Learnings (2026-01-10)

**Cursor State Architecture:**
- Cursor state lives in ContentWidget (byte position, line, column, animation state)
- CodeEditorWidget handles keyboard events and delegates cursor operations to ContentWidget
- Separate `cursor_preferred_column_` for vertical navigation (remembers horizontal position when moving up/down through lines of varying lengths)

**Cursor Position Conversion:**
- UTF-8 byte position ↔ line/column requires careful handling
- Line is 1-indexed (matches visual convention), column is UTF-16 code units (matches DirectWrite)
- `line_column_to_byte_pos()` walks UTF-8 bytes counting UTF-16 code units to convert
- `byte_pos_to_line_column()` uses LineIndex for line, then reads text and converts to UTF-16 for column

**Click-to-Position Using DirectWrite:**
- Use `IDWriteTextLayout::HitTestPoint()` to convert pixel X to text position
- Returns `textPosition` (0-indexed character) and `isTrailing` flag
- If `isTrailing == TRUE`, cursor is after the character (col = textPosition + 1)
- Requires having a TextLayout for the clicked line (get from slot pool)

**Cursor Rendering:**
- Cursor is a filled rectangle (2px wide × line height)
- Only render when focused (`flags & WidgetFlags::Focused`)
- Apply opacity for blink animation: `cursor_color.a *= cursor_opacity_`
- Position: `rect.x + PADDING_LEFT + cursor_x - scroll_x`, adjusted for current line

**Cursor Blink Animation (matching TextInput pattern):**
- 1000ms full cycle, 150ms fade duration
- Pause timer (500ms) keeps cursor visible after movement
- Phase calculation: `fmodf(blink_timer, period)` determines current animation phase
- Use `Easing::ease_out_quad()` for smooth fade in/out
- Only animate when focused and cursor not moving

**Focus Handling:**
- `WidgetFlags::Focusable` on CodeEditorWidget enables focus
- `wants_arrow_navigation()` returns true to receive arrow key events
- `on_focus()` sets Focused flag on ContentWidget and resets cursor blink
- `on_blur()` clears Focused flag (cursor stops rendering)

**Unity Build Namespace Considerations:**
- Easing functions defined in ui_widgets_core.cpp, forward declare in code_editor files
- Don't redefine inline functions that exist elsewhere in unity build
- Widget virtual methods (on_focus, on_blur) return bool, not void

**Editing Operations:**
- Push undo before any edit: `document_->content()->push_undo()`
- After insert/delete: update LineIndex, unbind all slots, update visible lines
- Count newlines in edited text for LineIndex update calls
- Update gutter width if line count changed

**UTF-8 Character Handling:**
- Backspace: convert column-1 to byte position to find character start
- Delete: convert column+1 to byte position to find character end
- Line joining: deleting newline at start of line = delete from prev line end to current line start
- Tab: converted to 4 spaces for simplicity

### Phase 5 Learnings (2026-01-12)

**Selection State Architecture:**
- Selection stored as byte positions (selection_start_, selection_end_) in ContentWidget
- Selection can be "backwards" (start > end) - normalize when rendering/processing
- Separate drag_anchor_pos_ tracks where drag started for proper extension behavior
- PieceTable Snapshot extended with selection_start/selection_end for undo/redo

**Multi-Click Detection Pattern:**
- Track last_click_time_, last_click_x_, last_click_y_
- click_count_ cycles 1→2→3→1 when clicks within DOUBLE_CLICK_MS and DOUBLE_CLICK_DIST
- Double-click: select word (alphanumeric + underscore groups)
- Triple-click: select entire line (including newline)
- Word-level drag: on double-click drag, extend selection by word boundaries

**Selection Rendering:**
- Selection color: semi-transparent accent (0x400078D4 = 40% opacity blue)
- Multi-line selection renders per-line rectangles:
  - First line: from start column to end of visible area
  - Middle lines: full visible width
  - Last line: from start of line to end column
- Only render visible lines (clamp to viewport)
- Render selection BEFORE text so text appears on top

**Keyboard Selection Pattern:**
- Shift+arrow keys extend selection from cursor
- If no selection exists when shift pressed, start selection at current cursor
- Without shift, navigation clears selection
- Lambda `handle_selection(new_byte_pos)` encapsulates this logic cleanly

**Word Boundary Detection:**
- is_word_char(c): alphanumeric or underscore
- get_word_boundary_left/right: scan until character type changes or newline
- Used for double-click selection and word-level drag

**Mouse Event Handling:**
- on_mouse_down: handle click count, shift+click, start drag
- on_mouse_move: update selection during drag (character or word level)
- on_mouse_up: end drag state
- Snap cursor position instantly during drag (no animation delay)

### Phase 7 Learnings (2026-01-12)

**Clipboard API Pattern:**
- Use `CF_UNICODETEXT` format for full Unicode support (UTF-16 on Windows)
- Convert between UTF-8 (document model) and UTF-16 (Windows clipboard)
- `MultiByteToWideChar(CP_UTF8, ...)` for UTF-8 → UTF-16
- `WideCharToMultiByte(CP_UTF8, ...)` for UTF-16 → UTF-8

**Clipboard Memory Management:**
- `GlobalAlloc(GMEM_MOVEABLE, ...)` allocates clipboard memory
- After `SetClipboardData()` succeeds, Windows owns the HGLOBAL - do NOT free
- Always `GlobalUnlock()` before `SetClipboardData()`
- Balance `OpenClipboard()` / `CloseClipboard()` calls

**Selection Helper Methods:**
- `get_selected_text()` returns heap-allocated UTF-8 buffer (caller must `delete[]`)
- `delete_selection()` handles newline counting, LineIndex update, cursor positioning
- Both methods handle no-selection case gracefully (return nullptr / cursor_byte_pos_)

**Paste Flow:**
1. Open clipboard, get CF_UNICODETEXT data
2. Convert UTF-16 to UTF-8
3. Push undo snapshot (before any modification)
4. Delete existing selection if present (updates byte_pos)
5. Count newlines for LineIndex update
6. Insert text at cursor
7. Update LineIndex with newline offsets
8. Invalidate affected lines and move cursor to end of pasted text

**Cut Flow:**
1. Push undo snapshot
2. Copy to clipboard (same as Ctrl+C)
3. Call delete_selection() to remove text
4. Update gutter and scrollbar

### Phase 6 & 8 Learnings (2026-01-15)

**Undo Coalescing Design:**
- Timer-based grouping: Sequential character inserts within 1 second coalesce into single undo entry
- State tracked: `coalesce_pending_`, `coalesce_start_pos_`, `coalesce_expected_pos_`, selection state
- Timer ID: `EDITOR_COALESCE_TIMER_ID = 13` in app.h
- Static owner pointer `g_coalesce_timer_owner` tracks which editor owns the active timer
- Coalescing breaks on: cursor movement, selection, newlines, other edit operations, 1-second timeout
- `flush_coalesce()` called before cut/paste/backspace/delete/undo/redo operations

**Coalescing Flow in `on_char()`:**
1. Check `can_coalesce`: pending exists, no selection, not newline, cursor at expected position
2. If cannot coalesce: flush pending, start new group with current cursor/selection state
3. Delete selection if present (selection means replacing, not coalescing)
4. Insert character
5. Update `coalesce_expected_pos_` to new cursor position
6. If newline: flush immediately; else restart timer

**Word Deletion (Ctrl+Backspace/Delete):**
- Uses existing `get_word_boundary_left()` and `get_word_boundary_right()` functions
- Selection exists → delete selection instead of word (consistent with single char delete)
- Line boundary handling: Ctrl+Backspace at line start deletes to end of previous line
- Newline skipping: Ctrl+Delete at end of line skips newline and deletes next word
- Dynamic buffer allocation for newline counting in deleted range (no fixed-size limit)

**Timer Integration:**
- Global function `editor_handle_coalesce_timer()` in `CodeEditor` namespace
- Called from `handle_wm_timer()` in platform.cpp when `wParam == EDITOR_COALESCE_TIMER_ID`
- Timer started with `SetTimer(g_main_window, ...)`, killed with `KillTimer(...)`
- Destructor clears owner pointer and stops timer if this editor owns it
