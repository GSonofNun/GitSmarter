# UI Development Guidelines

Derived from Ryan Fleury's UI Series (Parts 1-9).
Source articles are external; see `docs/REFERENCES.md` (https://www.rfleury.com/).

---

## Part 1: The Interaction Medium

### Core Philosophy
- UI is a **communication channel** between user and programmer
- Goal: minimize ratio of "bits sent" to "useful bits received"
- Standard widgets (buttons, checkboxes, sliders) are a shared vocabulary - reuse them

### Architecture
- **Two-layer separation**: Core (layout, rendering, animation) + Builder (arranges widgets)
- **Escape hatches are mandatory**: Core must expose low-level primitives for custom widgets

### Design Priority
1. Minimize user time investment
2. Reduce information transmission overhead
3. Maintain code simplicity
4. Support rapid iteration

---

## Part 2: Immediate Mode (Build Every Frame)

### The IMGUI Insight
- All code for one widget should be **entirely localized** - not scattered across construction, modification, and event handling
- Rebuild widget hierarchy fresh each frame

### Single-Frame Layout Delay
- Problem: Widgets need coordinates immediately, but layout depends on later widgets
- Solution: Use **previous frame's layout data** for building -> run autolayout -> render
- Acceptable because "interfaces only move when animating"

### Semantic Sizing
```
SizeKind_Pixels          - Direct pixel values
SizeKind_TextContent     - Size from text dimensions
SizeKind_PercentOfParent - Percentage-based
SizeKind_ChildrenSum     - Computed from children
```
Each has a **strictness** (0.0-1.0) controlling flexibility under space constraints.

### Widget Cache (Hybrid Retained/Immediate)
- **Tree links** (rebuilt each frame): parent, children, siblings
- **Hash links** (persistent): for cached data - animations, hover state, positions

### Keying
- Widgets need unique IDs across frames via string hashing
- `##` suffix: hashed but not displayed
- `###` suffix: only what follows is hashed

### Animation Data
Two persistent values per widget handle most animations:
- `hot_t` - hover state (0.0 -> 1.0)
- `active_t` - pressed state (0.0 -> 1.0)

---

## Part 3: Feature Flags over Widget Types

### Don't do this:
```cpp
enum WidgetKind { Button, Checkbox, Slider... }
```

### Do this instead:
```cpp
BoxFlag_Clickable
BoxFlag_DrawText
BoxFlag_DrawBorder
BoxFlag_HotAnimation
BoxFlag_ClipRect
// etc.
```

### Why
N features = N codepaths. 2^N combinations possible, but you only implement N.

**Critical Insight**: "Why do I care what 'is a button'? Feature combinations matter; widget types don't."

### Memory Reality
- 1,024 widgets x 512 bytes = 0.5 MiB (fits in L2 cache)
- Don't optimize here prematurely

---

## Part 4: The Widget Is A Lie (Node Composition)

### Key Insight
A "widget" is not a single node. It's **composed of multiple boxes**.

**Rename**: `UI_Widget` -> `UI_Box` (a building block, not a high-level concept)

### List Box Decomposition Example
```
* List Box Region (child layout on x)
  * Scrollable Region (fill space, clip rect, overflow y)
    * List Item 1 (clickable text)
    * List Item 2 (clickable text)
    * ...
  * Scroll Bar Region (fixed size, child layout on y)
    * Scroll-Up Button
    * Space Before Scroller
    * Scroller (draggable)
    * Space After Scroller
    * Scroll-Down Button
```

### Single-Line Text Field Decomposition
```
* Container (allow overflow x, clip rect, scrollable, clickable)
  * Text Content (text + cursor/selection rendering)
```

### Tooltips/Dropdowns
Pre-build roots at frame start, push content anywhere. Test mouse against special subtree before consuming events.

---

## Part 5: Visual Content (Spacing & Styling)

### Spacers
- Boxes with no features except participating in layout
- `UI_BoxMake(0, "")` - empty string = no caching (null ID)
- Can have visual info attached, just not interaction

### Style Stacks
Instead of passing colors/sizes to every call, use implicit stacks:
```cpp
UI_PushTextColor(red);
  UI_Button("A");
  UI_Button("B");
UI_PopTextColor();
```

### Defer Loop Pattern (C)
```cpp
#define UI_TextColor(v) \
  for(int _i_ = (UI_PushTextColor(v), 0); !_i_; _i_++, UI_PopTextColor())

UI_TextColor(red) {
  UI_Button("Red Button");
}
```

### Windowing (Virtual Lists)
```cpp
F32 space_before = index_range.min * item_height;
F32 space_after  = (total_count - index_range.max) * item_height;

UI_Spacer(UI_Pixels(space_before, 1));
for(idx = index_range.min; idx <= index_range.max; idx++) {
  // build visible item
}
UI_Spacer(UI_Pixels(space_after, 1));
```

---

## Part 6: Rendering (Bottom-Up Shader Design)

### Philosophy
Don't implement each feature as a separate codepath. Build one unified pipeline.

**Bottom-Up Approach**: Layer constraints gradually instead of overfitting to subset of features.

### Single Shader Supports
1. Solid rectangles
2. Textured rectangles (text, icons via glyph atlas)
3. Gradients (per-vertex colors -> emboss/deboss effects)
4. Rounded corners (signed-distance function)
5. Soft edges/shadows (edge_softness parameter)
6. Hollow rectangles (border_thickness parameter)

### Per-Instance Data
```cpp
struct VS_Input {
  float2 dst_p0, dst_p1;      // screen position
  float2 src_p0, src_p1;      // texture coordinates
  float4 colors[4];           // per-vertex colors
  float corner_radius;
  float edge_softness;
  float border_thickness;
  uint vertex_id;
};
```

### SDF for Rounded Rectangles
```cpp
float RoundedRectSDF(vec2 sample_pos, vec2 center, vec2 half_size, float r) {
  vec2 d2 = abs(center - sample_pos) - half_size + vec2(r, r);
  return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
}
```

### Batching
Non-overlapping widgets can be batched together -> few draw calls for complex UIs.

---

## Part 7: Where IMGUI Ends (Windows, Panels, Tabs)

### Critical Insight
Not everything should be immediate-mode. Entity state (windows, panels, tabs) should be **managed by builder code, not core**.

### Why Core-Managed Window State Fails
1. User opens window at default position
2. User moves/resizes window
3. User closes window
4. User re-opens window
5. **What position?** (Should remember user's preference, but core doesn't know the rules)

### Window State Belongs in Builder Code
```cpp
struct Window {
  Window *next, *prev;
  Rect rect;
  String8 title;
};

struct AppState {
  Window *first_window, *last_window;
  Window *free_window;  // free list for closed windows
};
```

### Panel System (non-overlapping tiles)
```cpp
struct Panel {
  Panel *first, *last, *next, *prev, *parent;
  Axis2 split_axis;
  F32 pct_of_parent;
};
```

### Command Buffer for Deferred Mutations
Don't mutate state immediately during UI building (e.g., `WindowClose`). Queue commands to process after building completes.

---

## Part 8: State Mutation, Jank, and Hotkeys

### The Jank Problem
- UI is cyclical: visually represents state that it's also changing
- Mutating state mid-frame causes **disagreement** - some parts see old state, some see new
- Results in: text rendering outside bounds, flickering, elements one frame behind

### Pure Functional Ground Truth
```
UI: State -> State'
```

But full deep-copy is wasteful. Instead: **Delta-Based State Mutation**

### The Pattern
1. Builder code produces **state delta** (commands), not direct mutations
2. Commands applied **next frame** before any building
3. All code during build sees consistent state

### Command Buffer
```cpp
struct Cmd { String8 string; };
struct CmdNode { CmdNode *next; Cmd cmd; };

void PushCmd(State *state, String8 string);
```

### Frame Order
1. Get OS events
2. Take hotkey events -> add to command buffer
3. **Apply commands** (mutate state)
4. Build UI (produces new commands for next frame)
5. Layout UI
6. Render UI

### Benefits
- Hotkeys and UI produce identical commands -> single mutation codepath
- Can log, replay, type commands in dev console
- Textual encoding forces ability to refer to entities by name

### Incomplete Information Problem
Some commands need info from later in frame (e.g., "Go To Line" needs font size, panel size for scroll position). Solution: Command sets "program the main loop" state that gets used during building.

---

## Part 9: Keyboard and Gamepad Navigation

### User Context Switches
- Position A: Both hands on keyboard
- Position B: One hand keyboard, one hand mouse
- Switching costs user time -> minimize by supporting keyboard navigation

### Default Navigation Rule
- Filter depth-first pre-order traversal of box tree
- Include only boxes with `BoxFlag_Clickable`
- Tab moves forward, Shift+Tab moves backward

### The Windowed List Problem
- User selects item in list, scrolls away
- Selected item's box no longer exists
- Box key alone insufficient for keyboard selection state

### Solution: Two Separate Concepts

**1. Interface Stack** - which interface has keyboard focus
- Box keys work here (windows, panels are stable)
- Push on enter, pop on Escape
```cpp
UI_PushNavInterfaceKey(UI_Key key);
UI_PopNavInterfaceKey();
UI_TopNavInterfaceKey();
```

**2. Keyboard Selection State** - which item within interface is selected
- Default: box keys + depth-first traversal
- Custom: builder code maintains own state for complex cases (windowed lists, grids)

### The Balance
- Simple interfaces: automatic depth-first navigation "just works"
- Complex interfaces: builder code controls selection state type and navigation rules

---

## Application to GitSmarter

### Current Alignment

| Principle | Status | Notes |
|-----------|--------|-------|
| Two-layer architecture | Aligned | Core in `widgets/` + builder code in specific widgets |
| Feature flags | Aligned | `WidgetFlags` enum with composable flags |
| Escape hatches | Aligned | `on_paint()` override for custom rendering |
| Arena allocator | Aligned | `g_widget_arena` for widget allocation |

### Potential Improvements

| Principle | Current | Recommendation |
|-----------|---------|----------------|
| Naming | "Widget" | Consider "Box" to emphasize composition |
| Layout timing | Retained with dirty flags | Current approach works; IMGUI delay not needed |
| Style stacks | Explicit params per widget | Could simplify builder code |
| Animation state | Separate `Animation` objects | Per-widget `hot_t`/`active_t` might simplify |
| Command buffer | Direct state mutation | Add for hotkey/UI parity |
| Interface stack | Not implemented | Needed for keyboard navigation |
| Virtual lists | Partial (some widgets) | Ensure consistent windowing pattern |

### Key Takeaways for New Widget Development

1. **Compose, don't enumerate** - Build complex widgets from simple boxes with flags
2. **Localize widget code** - All logic for a widget in one place
3. **Defer state mutations** - Queue commands during build, apply before next frame
4. **Plan for keyboard nav** - Consider navigation scope and selection state type upfront
5. **Use semantic sizing** - Pixels, percent, text-content, children-sum with strictness
6. **Keep entity state in builder** - Windows, panels, tabs managed outside core
