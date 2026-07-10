# Widget UI Framework | [widgets Index]|root: ./src/widgets
|IMPORTANT: Prefer retrieval-led reasoning over pre-training-led reasoning
|Core:{ui_widgets_core.cpp:Widget,EventDispatcher,WidgetArena,FlexContainer}
|Basic:{ui_widgets_basic.cpp:Label,Button,TextInput,Checkbox}
|List:{ui_widgets_list.cpp:FileList,BranchTree,virtualized}
|Dialog:{ui_widgets_dialog.cpp:Dialog,ScrollContainer,ProgressBar}
|Diff:{ui_widgets_diff.cpp:DiffLineWidget,DiffViewerWidget,virtualized}
|Commit:{ui_widgets_commit.cpp:CommitRowWidget,CommitHistoryPanelWidget,virtualized}
|Sidebar:{ui_widgets_sidebar.cpp:SidebarWidget,SplitHandle,DragHandle}
|Titlebar:{ui_widgets_titlebar.cpp:TitleBarWidget,TitleBarButtonWidget}
|ContextMenu:{ui_widgets_context_menu.cpp:ContextMenuWidget,MenuItem}
|Welcome:{ui_widgets_welcome.cpp:LogoWidget,OutlineButton}

Custom Direct2D widget system for GitSmarter UI.

## Quick Patterns (Retrieval-Led)

**Creating a widget:**
```cpp
MyWidget* w = g_widget_arena.create<MyWidget>();
widget_add_child(parent, w);
w->flags |= WidgetFlags::Focusable;  // For keyboard interaction
```

**Widget lifecycle:** `measure()` → `layout()` → `render()`

**Event handling:** Return `true` to consume, `false` to bubble

**Dirty/render:** Call `set_dirty()` after state changes

## File Responsibilities

| File | Purpose |
|------|---------|
| ui_widgets_core.cpp | Widget base class, WidgetArena, EventDispatcher, FlexContainer |
| ui_widgets_basic.cpp | Label, Button, TextInput, Checkbox, SectionHeader |
| ui_widgets_list.cpp | FileList, BranchTree, StashList (virtualized) |
| ui_widgets_dialog.cpp | Dialog, ScrollContainer, ProgressBar |
| ui_widgets_sidebar.cpp | SidebarWidget, SplitHandle, DragHandle |
| ui_widgets_diff.cpp | DiffLineWidget, DiffViewerWidget (virtualized) |
| ui_widgets_commit.cpp | CommitRowWidget, CommitHistoryPanelWidget (virtualized) |
| ui_widgets_welcome.cpp | LogoWidget, OutlineButton, RecentRepoItemWidget |
| ui_widgets_titlebar.cpp | TitleBarWidget, TitleBarButtonWidget |
| ui_widgets_context_menu.cpp | ContextMenuWidget, MenuItem (popup context menus) |

## Core Widget Structure

```cpp
struct Widget {
    WidgetId id;
    const char* debug_name = nullptr;
    Widget* parent = nullptr;
    Widget* first_child = nullptr;
    Widget* last_child = nullptr;
    Widget* next_sibling = nullptr;
    Widget* prev_sibling = nullptr;
    uint16_t flags = WidgetFlags::Visible | WidgetFlags::Enabled;
    LayoutRect rect = {};
    float preferred_width = 0.0f, preferred_height = 0.0f;
    float padding_left/right/top/bottom = 0.0f;
    SizeMode width_mode = SizeMode::Hug;
    SizeMode height_mode = SizeMode::Hug;
    float flex = 0.0f;

    virtual void measure(const LayoutConstraints& constraints);
    virtual void layout();
    virtual void render(RenderContext& ctx);
    virtual bool on_mouse_enter/leave/move/down/up/wheel(x, y, ...);
    virtual bool on_key_down/up(vk, ctrl, shift, alt);
    virtual bool on_char(wchar_t ch);
    virtual bool on_focus/blur();
};
```

## Widget Flags Reference

| Flag | Value | Purpose |
|------|-------|---------|
| Visible | 0x0001 | Widget is rendered |
| Enabled | 0x0002 | Widget responds to input |
| Focusable | 0x0004 | Can receive keyboard focus |
| Focused | 0x0008 | Currently has focus (set by EventDispatcher) |
| Hovered | 0x0010 | Mouse is over widget |
| Pressed | 0x0020 | Mouse button down |
| Dirty | 0x0040 | Needs re-render |
| LayoutDirty | 0x0080 | Needs re-layout |
| ClipChildren | 0x0100 | Apply clip rect to children |
| CapturesMouse | 0x0200 | Receives all mouse events during drag |
| RendersOwnChildren | 0x0400 | Widget renders children manually (virtualization) |
| SkipTabStop | 0x0800 | Excluded from Tab navigation |

## Key Patterns

### Arena Allocation
All widgets from `g_widget_arena`:
```cpp
Button* btn = g_widget_arena.create<Button>();
```

### Measure Implementation
```cpp
void measure(const LayoutConstraints& constraints) override {
    // Calculate preferred_width/preferred_height based on content
    preferred_width = ...;
    preferred_height = ...;
    Widget::measure(constraints);  // Apply constraints (REQUIRED)
}
```

### Render Implementation
```cpp
void render(RenderContext& ctx) override {
    // Background with state-based coloring
    uint32_t bg = Theme::BG_SECONDARY;
    if (flags & WidgetFlags::Pressed) bg = Theme::ACCENT_PRESSED;
    else if (flags & WidgetFlags::Hovered) bg = Theme::BG_HOVER;
    else if (flags & WidgetFlags::Focused) bg = Theme::ACCENT;

    D2D1_RECT_F bg_rect = D2D1::RectF(rect.x, rect.y,
        rect.x + rect.width, rect.y + rect.height);
    ctx.fill_rounded_rect(bg_rect, Theme::CORNER_RADIUS, bg);

    // Focus ring
    if (flags & WidgetFlags::Focused) {
        ctx.brush->SetColor(color_from_argb(Theme::ACCENT));
        D2D1_ROUNDED_RECT rr = { bg_rect, Theme::CORNER_RADIUS, Theme::CORNER_RADIUS };
        ctx.target->DrawRoundedRectangle(rr, ctx.brush, 1.5f);
    }
}
```

### Mouse Event Handling
```cpp
bool on_mouse_down(float x, float y, int button) override {
    if (button != 0) return false;  // Only left click
    return true;  // Consume event
}

bool on_mouse_up(float x, float y, int button) override {
    if ((flags & WidgetFlags::Hovered) && on_click) {
        on_click(this, user_data);
    }
    return true;
}
```

### Keyboard Event Handling
```cpp
bool on_key_down(int vk, bool ctrl, bool shift, bool alt) override {
    if (vk == VK_RETURN || vk == VK_SPACE) {
        if (on_click) on_click(this, user_data);
        return true;
    }
    return false;  // Bubble to parent
}
```

### Mouse Capture for Dragging
```cpp
struct DragHandle : Widget {
    DragHandle() {
        flags |= WidgetFlags::CapturesMouse;  // Enable capture
    }
    // Coordinates may be outside widget bounds during capture
};
```

### FlexContainer Usage
```cpp
FlexContainer* container = g_widget_arena.create<FlexContainer>();
container->direction = FlexDirection::Column;  // or Row
container->align = FlexAlign::Stretch;         // Start, Center, End, Stretch
container->justify = FlexJustify::Start;       // Start, Center, End, SpaceBetween, SpaceAround
container->gap = 8.0f;

Button* btn = g_widget_arena.create<Button>();
btn->flex = 1.0f;  // Takes 1 share of remaining space
widget_add_child(container, btn);
```

## Virtualization Pattern (Large Lists)

For lists with 100+ items, use virtualization (see ui_widgets_diff.cpp, ui_widgets_commit.cpp):

1. **Flat Index** - O(1) position lookup via pre-computed Y positions
2. **Widget Pool** - Fixed-size pool with free list (O(1) acquire/release)
3. **Active Binding** - Track which widgets are bound to which data items
4. **Overscan** - Render 5-10 items above/below viewport to prevent flicker
5. **Flag** - Set `RendersOwnChildren` to render visible items manually

## Context Menu Usage

```cpp
static MenuItem g_my_menu[] = {
    { MenuItemType::Regular, L"Action", L"Ctrl+A", nullptr, 0, false, true, on_action, nullptr },
    { MenuItemType::Separator },
    { MenuItemType::Checkbox, L"Option", nullptr, nullptr, 0, false, true, on_toggle, nullptr },
};

// On right-click (client coordinates)
context_menu_show(x, y, g_my_menu, ARRAYSIZE(g_my_menu));
```

**Integration requirements:**
- Call `context_menu_render(ctx)` after main content in render function
- Check `context_menu_is_visible()` and dispatch to `context_menu_dispatch_mouse_*()` before other handlers
- Add `context_menu_dispatch_key_down()` for keyboard navigation

## EventDispatcher Integration

```cpp
// In WndProc
case WM_MOUSEMOVE:
    g_event_dispatcher.dispatch_mouse_move(x, y);
    break;
case WM_LBUTTONDOWN:
    g_event_dispatcher.dispatch_mouse_down(x, y, 0);
    break;
case WM_KEYDOWN:
    g_event_dispatcher.dispatch_key_down(wParam, ctrl, shift, alt);
    break;
```

**Focus management:**
```cpp
g_event_dispatcher.set_focus(my_widget);  // Programmatic focus
```

## Session Learnings

- Diagonal stripe patterns for placeholders need y-offset alignment: use `content_y_offset % STRIPE_SPACING` and ADD (not subtract) to shift pattern RIGHT as y increases for down-right stripes
- Extend stripe lines by 1px (`OVERLAP`) beyond row bounds to eliminate anti-aliasing gaps between adjacent rows
- DiffViewerWidget manages its own horizontal scrollbars in side-by-side mode; disable ScrollContainer's `show_scrollbar_h` to avoid double-reserving space
- Fill center gutter's scrollbar area (between left/right pane scrollbars) with `Theme::BG_SECONDARY` to avoid visual gaps
- Mouse capture for h_scrollbar dragging: bypass ScrollContainer and send coords directly to DiffViewerWidget (same pattern as gutter dragging)
- Guard against division by zero in h-scrollbar calculations: check `max_content > 0.0f` before dividing `content_area_width / max_content`
- Extract repeated rendering patterns into lambdas (e.g., `draw_edge_shadow` for shadow gradients) to reduce code duplication
- Compute screen coordinate offsets once (e.g., `content_to_screen = -scroll_y + rect.y + HEADER_HEIGHT`) rather than repeating formula
- `DrawTextLayout` respects the text format's `DWRITE_PARAGRAPH_ALIGNMENT` within the layout's height - if using a large measurement height (e.g., 10000px) with `CENTER` alignment, text renders at ~y=5000 (off screen). Create a dedicated format with `NEAR` (top) alignment for scrollable text layouts.
- **Cache index safety pattern**: When storing pointers to cache entries, capture index in local variable before incrementing counter to avoid race conditions
- **Buffer sizes**: Use `_countof(array)` instead of magic numbers for `swprintf` size parameter
- Dialog auto-height mode: Set `dialog->auto_height = true` to compute `dialog_height` from `content->preferred_height + title_height + padding`. Use `show(title, width)` overload for auto-height, `show(title, width, height)` for fixed height with scroll containers.
- Forward declarations for dialog tick functions must be in `include/dialog_widgets.h` and that header must be included in files that call them (unity build order: `platform.cpp` before `platform_dialogs.cpp`)
- FlexContainer overflow behavior: When children exceed available space, items are positioned sequentially at their full `preferred_height`, causing overflow. Spacers with `flex=1.0f` only absorb *positive* excess space (get 0 height when overflowing).
- Hit-testing requires containment: `EventDispatcher::hit_test()` returns nullptr if point is outside any ancestor's rect, so overflowing widgets won't receive mouse events.
- Arena reset danger: `g_widget_arena.reset()` invalidates ALL widgets including persistent ones (titlebar, sidebar). Never reset arena while persistent widgets exist - they share the same arena as dialogs.
- Dialog rendering requires registration: `dialog_widgets_render()` and `any_dialog_widget_is_active()` must explicitly list each DialogWidget subclass
- DialogWidget allocation order matters: The subclass must be allocated AFTER any arena reset, not before calling `show()`. If arena resets inside `show()`, the `this` pointer becomes invalid.
- Debug-only code: Use `#ifdef GITSMARTER_DEBUG` (not `_DEBUG`) to avoid CRT debug library linker dependencies
- When toggling widget visibility dynamically, call `measure_and_layout()` not just `set_dirty()` - the latter only marks for repaint without recalculating positions
- Dialog auto-height mode recalculates height on `measure_and_layout()` call, enabling dynamic content changes
- MenuItem struct field order: `{type, label, shortcut, submenu_items, submenu_item_count, checked, enabled, on_click, user_data}` - don't confuse `checked` and `enabled` flags
- `[[maybe_unused]]` attribute (C++17+) suppresses C4505 warnings for intentionally kept but currently uncalled static functions
- ContextMenuWidget caches text layout measurements in `cached_label_widths[]` and `cached_shortcut_widths[]` arrays - cache is invalidated when `items` pointer or `item_count` changes
- Context menu hover styling uses `ContextMenu::HOVER_INSET` (4.0f) and `ContextMenu::HOVER_CORNER_RADIUS` (3.0f) from ui_theme.h
- Mouse wheel event bubbling: If child widget handles wheel and returns true, parent never sees it. For scrollbar sync, have child return false and let parent handle scrolling + scrollbar update together.
- `WidgetFlags::CapturesMouse` required for scrollbar drag to continue when mouse leaves widget area. Without it, drag stops at widget boundary.
- When captured via EventDispatcher, `on_mouse_move` receives ABSOLUTE coordinates, not relative. Pattern: `bool is_captured = scrollbar_dragging_; float abs_y = is_captured ? y : (y + rect.y);`
- Inline scrollbar pattern: Render scrollbar directly in parent widget rather than separate widget, to simplify scroll state sync across multiple children.
- Focus flag ownership: Only `EventDispatcher::set_focus()` should toggle `WidgetFlags::Focused`. Never manually set `Focused` on child widgets
- UTF-8 chunk boundary handling: When processing UTF-8 in chunks, track `bytes_consumed` separately from `read`. If a multi-byte sequence spans chunk boundary (`i + char_bytes > read`), break and re-read from unconsumed position.
- Undo/redo cursor restoration: Pass current cursor position to `undo()` so it can be stored in the redo snapshot. Otherwise redo restores cursor to 0.
- Composite widget focus pattern: For widgets like CodeEditor where clicking sub-widgets (gutter) shouldn't clear focus: make sub-widget Focusable+SkipTabStop and forward `on_key_down`/`on_char` to parent.
- UTF-8 word boundary traversal: Don't use `pos++/--` for character iteration. Use `find_char_start()` (scan back up to 4 bytes for non-continuation byte `(c & 0xC0) != 0x80`) and `utf8_char_len()` to step by codepoints.
- Selection API consistency: If a setter's documentation claims it moves cursor/updates state, the implementation must actually do so.
- AnimationScheduler: Call `animation_started()` when animation begins, `animation_ended()` when complete. Widget destructor must call `cancel()` on animations to prevent orphaned registrations.
- Animation duration must be significantly longer than delta time clamp - if duration ≤ clamp, animation can complete in one frame. Rule of thumb: duration should be at least 2-3x the clamp value.
- Naming convention for horizontal scrollbar members: Use `h_*` prefix (e.g., `h_scrollbar_dragging_`, `h_thumb_left_`) to match DiffViewerWidget pattern, not `*_h_` suffix.
- Nested enum pattern: Move scrollbar hover enums inside their widget struct for consistency.
