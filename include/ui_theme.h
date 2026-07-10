#pragma once

#include <cstdint>
#include <d2d1.h>

// Theme colors (ARGB format)
namespace Theme {
    // Background colors
    constexpr uint32_t BG_PRIMARY = 0xFF1E1E1E;
    constexpr uint32_t BG_SECONDARY = 0xFF252526;
    constexpr uint32_t BG_TERTIARY = 0xFF2D2D30;
    constexpr uint32_t BG_HOVER = 0xFF3E3E42;

    // Text colors
    constexpr uint32_t TEXT_PRIMARY = 0xFFE0E0E0;
    constexpr uint32_t TEXT_SECONDARY = 0xFF9D9D9D;
    constexpr uint32_t TEXT_MUTED = 0xFF6D6D6D;

    // Accent colors
    constexpr uint32_t ACCENT = 0xFF0078D4;
    constexpr uint32_t ACCENT_HOVER = 0xFF1084D8;
    constexpr uint32_t ACCENT_PRESSED = 0xFF006CBE;

    // Title bar
    constexpr uint32_t TITLE_BAR_BG = 0xFF323233;
    constexpr uint32_t TITLE_BAR_BUTTON_HOVER = 0xFF3E3E42;
    constexpr uint32_t TITLE_BAR_CLOSE_HOVER = 0xFFE81123;

    // Borders and elevated surfaces
    constexpr uint32_t BORDER = 0xFF3C3C3C;
    constexpr uint32_t BG_ELEVATED = 0xFF333337;

    // Git status colors
    constexpr uint32_t STATUS_MODIFIED = 0xFF519ABA;  // Blue
    constexpr uint32_t STATUS_ADDED = 0xFF4EC970;     // Green
    constexpr uint32_t STATUS_DELETED = 0xFFF14C4C;   // Red
    constexpr uint32_t STATUS_RENAMED = 0xFFCCA700;   // Yellow

    // Selection
    constexpr uint32_t SELECTION_BORDER = 0xFF0078D4; // Solid accent for left border

    // Layout
    constexpr float CORNER_RADIUS = 4.0f;
    constexpr float PADDING = 8.0f;
    constexpr float PADDING_SMALL = 4.0f;
    constexpr float PADDING_LARGE = 16.0f;

    // Animation Parameters
    constexpr float HOVER_TRANSITION_MS = 150.0f;     // Hover fade duration
    constexpr float PRESS_TRANSITION_MS = 50.0f;      // Press response (snappy)
    constexpr float FOCUS_TRANSITION_MS = 200.0f;     // Focus ring fade
    constexpr float SCROLL_DECEL_RATE = 0.92f;        // Scroll momentum decay per frame
    constexpr float SCROLL_MIN_VELOCITY = 0.5f;       // Stop threshold (pixels/sec)

    // Shadow Parameters
    constexpr float SHADOW_DIALOG_OFFSET_Y = 8.0f;
    constexpr float SHADOW_DIALOG_BLUR = 24.0f;
    constexpr float SHADOW_DIALOG_OPACITY = 0.35f;
    constexpr float SHADOW_TOOLTIP_OFFSET_Y = 4.0f;
    constexpr float SHADOW_TOOLTIP_BLUR = 12.0f;
    constexpr float SHADOW_TOOLTIP_OPACITY = 0.30f;
    constexpr float SHADOW_BUTTON_OFFSET_Y = 2.0f;
    constexpr float SHADOW_BUTTON_BLUR = 4.0f;
    constexpr float SHADOW_BUTTON_OPACITY = 0.20f;

    // Focus Ring Parameters
    constexpr uint32_t FOCUS_GLOW = 0x300078D4;       // 19% accent for outer glow
    constexpr float FOCUS_RING_WIDTH = 2.0f;
    constexpr float FOCUS_RING_OFFSET = 2.0f;
    constexpr float FOCUS_GLOW_SPREAD = 3.0f;

    // Button Press Animation
    constexpr float BUTTON_PRESS_SCALE = 0.98f;
}

// Welcome screen layout constants
namespace Welcome {
    constexpr float LOGO_SIZE = 64.0f;
    constexpr float TITLE_FONT_SIZE = 28.0f;
    constexpr float SUBTITLE_FONT_SIZE = 13.0f;
    constexpr float BUTTON_WIDTH = 180.0f;
    constexpr float BUTTON_HEIGHT = 40.0f;
    constexpr float BUTTON_SPACING = 16.0f;
    constexpr float BUTTON_RADIUS = 4.0f;
    constexpr float SECTION_SPACING = 32.0f;
    constexpr float REPO_ITEM_HEIGHT = 56.0f;
    constexpr float CONTENT_MAX_WIDTH = 500.0f;
    constexpr size_t MAX_RECENT_REPOS = 10;
}

// Sidebar layout constants
namespace Sidebar {
    constexpr float WIDTH_MIN = 180.0f;
    constexpr float WIDTH_MAX = 400.0f;
    constexpr float WIDTH_DEFAULT = 240.0f;

    constexpr float DRAG_HANDLE_HIT = 4.0f;     // Hit area width for resize
    constexpr float DRAG_HANDLE_VISIBLE = 1.0f; // Visible border width

    constexpr float SECTION_HEADER_HEIGHT = 28.0f;
    constexpr float FILE_ITEM_HEIGHT = 26.0f;
    constexpr float BRANCH_ITEM_HEIGHT = 24.0f;

    constexpr float STATUS_BADGE_SIZE = 18.0f;
    constexpr float STATUS_BADGE_RADIUS = 3.0f;

    constexpr float BRANCH_SELECTOR_HEIGHT = 36.0f;

    // Commit box
    constexpr float COMMIT_BOX_MIN_HEIGHT = 50.0f;
    constexpr float COMMIT_BOX_MAX_HEIGHT = 120.0f;
    constexpr float COMMIT_BOX_PADDING = 8.0f;
    constexpr float COMMIT_BUTTON_HEIGHT = 28.0f;
    constexpr float COMMIT_LINE_HEIGHT = 18.0f;

    // Enhanced file layout (two-line display)
    constexpr float FILE_ITEM_HEIGHT_V2 = 42.0f;      // Taller for two lines
    constexpr float FILE_ICON_SIZE = 16.0f;           // Shell icon size
    constexpr float FILE_ICON_PADDING = 8.0f;
    constexpr float FILE_LINE1_OFFSET = 6.0f;         // Y offset for filename
    constexpr float FILE_LINE2_OFFSET = 24.0f;        // Y offset for path
    constexpr float FILE_LINE2_FONT_SIZE = 10.0f;    // Smaller path font

    // Discard button
    constexpr float DISCARD_BUTTON_SIZE = 16.0f;

    // Scrollbar (standardized with diff/history viewer)
    constexpr float SCROLLBAR_WIDTH = 14.0f;
    constexpr float SCROLLBAR_MIN_THUMB = 30.0f;
    constexpr float SCROLLBAR_BUTTON_SIZE = 14.0f;
    constexpr float SCROLLBAR_THUMB_MARGIN = 2.0f;

    // Branch split handle
    constexpr float BRANCH_SPLIT_HEIGHT = 4.0f;
    constexpr float BRANCH_SPLIT_HIT_HEIGHT = 8.0f;

    // Branch tree
    constexpr float BRANCH_INDENT = 16.0f;            // Indent per level
    constexpr float BRANCH_FOLDER_ICON_SIZE = 12.0f;

    // Commit view (historical commit details)
    constexpr float COMMIT_VIEW_FILE_HEIGHT = 26.0f;  // Compact single-line file items
    constexpr float COMMIT_VIEW_MSG_MAX_HEIGHT = 60.0f;  // Max height for commit message area
    constexpr float COMMIT_VIEW_MSG_LINE_HEIGHT = 16.0f; // Line height for message text
}

// Status bar layout constants
namespace StatusBar {
    constexpr float HEIGHT = 24.0f;
    constexpr float PADDING_H = 10.0f;
    constexpr float ICON_SIZE = 14.0f;
    constexpr float ICON_SPACING = 6.0f;

    // Progress bar
    constexpr float PROGRESS_BAR_WIDTH = 200.0f;
    constexpr float PROGRESS_BAR_HEIGHT = 4.0f;
    constexpr float PROGRESS_BAR_CORNER = 2.0f;
    constexpr uint32_t PROGRESS_BG = 0x40000000;      // Semi-transparent black
    constexpr uint32_t PROGRESS_FILL = 0xFFFFFFFF;    // White
    constexpr float CANCEL_BUTTON_SIZE = 16.0f;
}

// Diff viewer layout constants
namespace DiffViewer {
    constexpr float HEADER_HEIGHT = 32.0f;
    constexpr float LINE_NUMBER_WIDTH = 45.0f;  // Width per line number column
    constexpr float GUTTER_WIDTH = 90.0f;       // Total width (old + new line nums)
    constexpr float LEFT_BORDER_WIDTH = 3.0f;   // Colored border for add/remove
    constexpr float CONTENT_PADDING = 12.0f;    // Padding left of content
    constexpr float LINE_HEIGHT = 20.0f;        // Height per diff line
    constexpr float SCROLL_SPEED = 40.0f;       // Pixels per scroll notch
    constexpr float SCROLLBAR_WIDTH = 14.0f;     // Vertical scrollbar width
    constexpr float SCROLLBAR_MIN_THUMB = 30.0f; // Minimum thumb height
    constexpr float SCROLLBAR_BUTTON_SIZE = 14.0f; // Up/down button height
    constexpr float SCROLLBAR_THUMB_MARGIN = 2.0f; // Margin around thumb

    // Background colors at 15% opacity (ARGB format)
    constexpr uint32_t BG_ADDED = 0x264EC970;    // Green 15%
    constexpr uint32_t BG_REMOVED = 0x26F14C4C;  // Red 15%
    constexpr uint32_t BG_HUNK = 0x260078D4;     // Accent 15%

    // Strong intraline highlight backgrounds (30% opacity)
    constexpr uint32_t BG_ADDED_STRONG = 0x4D4EC970;    // Green 30%
    constexpr uint32_t BG_REMOVED_STRONG = 0x4DF14C4C;  // Red 30%

    // Diff algorithm constants
    constexpr int CONTEXT_LINES = 3;             // Lines of context before/after changes

    // Intraline diff algorithm limits
    constexpr int INTRALINE_MAX_UTF16 = 2048;    // Skip intraline for lines longer than this
    constexpr int INTRALINE_MAX_SPANS = 64;      // Max spans per line
    constexpr int INTRALINE_COALESCE_GAP = 1;    // Merge spans separated by this many chars

    // ========================================================================
    // Side-by-Side View Layout
    // ========================================================================
    constexpr float CENTER_GUTTER_WIDTH = 30.0f;     // Width of center gutter (future hunk buttons)
    constexpr float PANE_MIN_WIDTH = 200.0f;         // Minimum width for each pane when resizing
    constexpr float GUTTER_SHADOW_WIDTH = 4.0f;      // Shadow gradient width at pane edges

    // Placeholder row (diagonal stripes for alignment)
    constexpr uint32_t PLACEHOLDER_BG = 0x10808080;           // 6% gray base
    constexpr uint32_t PLACEHOLDER_STRIPE = 0x18606060;       // 9% darker stripes
    constexpr float PLACEHOLDER_STRIPE_WIDTH = 8.0f;          // Stripe thickness
    constexpr float PLACEHOLDER_STRIPE_SPACING = 16.0f;       // Distance between stripes

    // Center gutter change indicators (gradient bands)
    constexpr uint32_t GUTTER_BG = 0xFF252526;                // Gutter background
    constexpr uint32_t GUTTER_SHADOW = 0x80000000;            // Shadow overlay (50% black)
    constexpr uint32_t GUTTER_REMOVED_BAND = 0x80F14C4C;      // Red gradient (50% opacity)
    constexpr uint32_t GUTTER_ADDED_BAND = 0x804EC970;        // Green gradient (50% opacity)
}

// Syntax highlighting colors (matches VS Code dark theme)
namespace SyntaxColors {
    constexpr uint32_t KEYWORD      = 0xFFC586C0;  // Pink-purple (if, else, return)
    constexpr uint32_t TYPE         = 0xFF4EC9B0;  // Teal (int, void, class)
    constexpr uint32_t STRING       = 0xFFCE9178;  // Orange (string literals)
    constexpr uint32_t COMMENT      = 0xFF6A9955;  // Green (comments)
    constexpr uint32_t NUMBER       = 0xFFB5CEA8;  // Light green (42, 3.14)
    constexpr uint32_t PREPROCESSOR = 0xFF9B9B9B;  // Gray (#include, #define)
    constexpr uint32_t PUNCTUATION  = 0xFFD4D4D4;  // Light gray ({, }, ;)
    constexpr uint32_t ATTRIBUTE    = 0xFF4FC1FF;  // Bright blue ([Serializable])
    constexpr uint32_t TAG          = 0xFF569CD6;  // Blue (<Button>)
    constexpr uint32_t MD_HEADING   = 0xFF569CD6;  // Blue (# Heading)
    constexpr uint32_t MD_CODE      = 0xFFCE9178;  // Orange (`code`)
    constexpr uint32_t MD_LINK      = 0xFF4EC9B0;  // Teal ([link](url))
    constexpr uint32_t MD_BOLD      = 0xFFDCDCAA;  // Yellow-ish (**bold**)
    constexpr uint32_t MD_ITALIC    = 0xFFC586C0;  // Pink (*italic*)

    // Get color for syntax token type (SyntaxToken enum from app.h)
    // Maps token values: 0=Default, 1=Keyword, 2=Type, 3=String, 4=Comment,
    //                    5=Number, 6=Preprocessor, 7=Punctuation, 8=Attribute,
    //                    9=Tag, 10=MdHeading, 11=MdCode, 12=MdLink, 13=MdBold, 14=MdItalic
    inline uint32_t get_token_color(uint8_t token) {
        static constexpr uint32_t colors[] = {
            Theme::TEXT_PRIMARY,  // 0: Default
            KEYWORD,              // 1: Keyword
            TYPE,                 // 2: Type
            STRING,               // 3: String
            COMMENT,              // 4: Comment
            NUMBER,               // 5: Number
            PREPROCESSOR,         // 6: Preprocessor
            PUNCTUATION,          // 7: Punctuation
            ATTRIBUTE,            // 8: Attribute
            TAG,                  // 9: Tag
            MD_HEADING,           // 10: MarkdownHeading
            MD_CODE,              // 11: MarkdownCode
            MD_LINK,              // 12: MarkdownLink
            MD_BOLD,              // 13: MarkdownBold
            MD_ITALIC,            // 14: MarkdownItalic
        };
        return (token < 15) ? colors[token] : Theme::TEXT_PRIMARY;
    }
}

// Commit history layout constants
namespace CommitHistory {
    // Panel layout
    constexpr float HEADER_HEIGHT = 36.0f;        // "HISTORY" header (increased for search box)
    constexpr float ROW_HEIGHT = 28.0f;           // Per-commit row height (compact)
    constexpr float GRAPH_WIDTH = 60.0f;          // Graph column width
    constexpr float SCROLLBAR_WIDTH = 14.0f;
    constexpr float SCROLLBAR_MIN_THUMB = 30.0f;
    constexpr float SCROLLBAR_BUTTON_SIZE = 14.0f;
    constexpr float SCROLLBAR_THUMB_MARGIN = 2.0f;
    constexpr float SCROLL_SPEED = 28.0f;         // One row per scroll notch

    // Search box in header
    constexpr float SEARCH_BOX_WIDTH = 180.0f;
    constexpr float SEARCH_BOX_HEIGHT = 24.0f;    // Compact height for header
    constexpr float SEARCH_BOX_PADDING = 6.0f;    // Internal padding
    constexpr float SEARCH_BOX_MARGIN = 8.0f;     // Margin from edges
    constexpr float SEARCH_ICON_SIZE = 12.0f;     // Magnifying glass icon
    constexpr float SEARCH_BOX_RADIUS = 4.0f;     // Corner radius
    constexpr uint32_t SEARCH_BOX_BG = 0xFF3E3E42;
    constexpr uint32_t SEARCH_BOX_BORDER = 0xFF3C3C3C;
    constexpr uint32_t SEARCH_BOX_FOCUS = 0xFF0078D4;

    // Split handle (resizable divider between diff and history)
    constexpr float SPLIT_HANDLE_HEIGHT = 4.0f;
    constexpr float SPLIT_MIN_RATIO = 0.2f;       // Min 20% for either panel
    constexpr float SPLIT_MAX_RATIO = 0.8f;       // Max 80% for either panel
    constexpr float SPLIT_DEFAULT_RATIO = 0.6f;   // Default: 60% diff, 40% history

    // Graph rendering
    constexpr float GRAPH_LINE_WIDTH = 2.0f;
    constexpr float GRAPH_NODE_RADIUS = 4.0f;
    constexpr float GRAPH_HEAD_RADIUS = 6.0f;
    constexpr float GRAPH_LANE_WIDTH = 14.0f;     // Horizontal spacing between lanes
    constexpr float GRAPH_LANE_OFFSET = 10.0f;    // Left margin to first lane

    // Commit row text layout - empirically tuned for 28px row
    constexpr float MESSAGE_PADDING_LEFT = 8.0f;
    constexpr float LINE1_Y_OFFSET = 6.0f;        // Message line Y offset from row top
    constexpr float LINE2_Y_OFFSET = 8.0f;       // Metadata line Y offset from row top

    // Details panel (shown when commit selected)
    constexpr float DETAILS_PANEL_WIDTH = 320.0f;
    constexpr float DETAILS_FILE_HEIGHT = 24.0f;
    constexpr float DETAILS_HEADER_HEIGHT = 36.0f;

    // Commit view mode (sidebar showing historical commit)
    constexpr uint32_t COMMIT_VIEW_BORDER = 0xFF0078D4;  // Accent color border
    constexpr float COMMIT_VIEW_BORDER_WIDTH = 2.0f;

    // Graph colors (rotating 5-color palette from spec)
    constexpr uint32_t GRAPH_COLORS[] = {
        0xFF6B9FED,  // Blue
        0xFFC678DD,  // Purple
        0xFFE5C07B,  // Yellow
        0xFF56B6C2,  // Cyan
        0xFFE06C75   // Red/Pink
    };
    constexpr size_t GRAPH_COLOR_COUNT = 5;

    // HEAD badge
    constexpr float HEAD_BADGE_WIDTH = 40.0f;
    constexpr float HEAD_BADGE_HEIGHT = 16.0f;
    constexpr float HEAD_BADGE_RADIUS = 3.0f;

    // Decoration badges (branch/tag labels)
    constexpr float DECORATION_HEIGHT = 16.0f;
    constexpr float DECORATION_RADIUS = 2.0f;
    constexpr float DECORATION_PADDING_H = 6.0f;
    constexpr float DECORATION_SPACING = 4.0f;
}

// Common dialog layout constants (base for all dialogs)
namespace DialogBase {
    // Layout
    constexpr float PADDING = 24.0f;
    constexpr float CORNER_RADIUS = 8.0f;
    constexpr float TITLE_HEIGHT = 48.0f;
    constexpr float CLOSE_BUTTON_SIZE = 32.0f;

    // Input fields
    constexpr float INPUT_HEIGHT = 36.0f;
    constexpr float INPUT_RADIUS = 4.0f;
    constexpr float LABEL_HEIGHT = 18.0f;
    constexpr float LABEL_SPACING = 6.0f;
    constexpr float FIELD_SPACING = 16.0f;
    constexpr float COMPACT_FIELD_SPACING = 4.0f;  // For simpler dialogs (rename, tag)

    // Checkbox
    constexpr float CHECKBOX_SIZE = 16.0f;
    constexpr float CHECKBOX_RADIUS = 3.0f;
    constexpr float CHECKBOX_LABEL_SPACING = 8.0f;

    // Buttons
    constexpr float BUTTON_HEIGHT = 36.0f;
    constexpr float BUTTON_RADIUS = 4.0f;
    constexpr float BUTTON_SPACING = 12.0f;

    // Colors
    constexpr uint32_t OVERLAY_BG = 0xB3000000;           // 70% opacity black
    constexpr uint32_t DIALOG_BG = 0xFF2D2D30;
    constexpr uint32_t INPUT_BG = 0xFF1E1E1E;
    constexpr uint32_t INPUT_BORDER = 0xFF3C3C3C;
    constexpr uint32_t INPUT_FOCUS = 0xFF0078D4;
    constexpr uint32_t BUTTON_PRIMARY = 0xFF0078D4;
    constexpr uint32_t BUTTON_PRIMARY_HOVER = 0xFF1C97EA;
    constexpr uint32_t BUTTON_PRIMARY_DISABLED = 0xFF404040;
    constexpr uint32_t BUTTON_SECONDARY = 0xFF3E3E42;
    constexpr uint32_t BUTTON_SECONDARY_HOVER = 0xFF505050;
    constexpr uint32_t ERROR_TEXT = 0xFFF14C4C;
    constexpr uint32_t SUCCESS_TEXT = 0xFF4EC970;
    constexpr uint32_t CHECKBOX_CHECK = 0xFF0078D4;
}

// Auth dialog layout constants
namespace AuthDialog {
    // Dialog-specific values
    constexpr float WIDTH = 500.0f;
    constexpr float BUTTON_WIDTH = 120.0f;
    constexpr uint32_t BUTTON_ACTIVE = 0xFF005A9E;    // Pressed state (unique to auth dialog)

    // Aliases to DialogBase
    constexpr float PADDING = DialogBase::PADDING;
    constexpr float CORNER_RADIUS = DialogBase::CORNER_RADIUS;
    constexpr float INPUT_HEIGHT = DialogBase::INPUT_HEIGHT;
    constexpr float LABEL_HEIGHT = DialogBase::LABEL_HEIGHT;
    constexpr float FIELD_SPACING = DialogBase::FIELD_SPACING;
    constexpr float BUTTON_HEIGHT = DialogBase::BUTTON_HEIGHT;
    constexpr float BUTTON_SPACING = DialogBase::BUTTON_SPACING;

    constexpr uint32_t OVERLAY_BG = DialogBase::OVERLAY_BG;
    constexpr uint32_t DIALOG_BG = DialogBase::DIALOG_BG;
    constexpr uint32_t INPUT_BG = DialogBase::INPUT_BG;
    constexpr uint32_t INPUT_BORDER = DialogBase::INPUT_BORDER;
    constexpr uint32_t INPUT_FOCUS = DialogBase::INPUT_FOCUS;
    constexpr uint32_t ERROR_TEXT = DialogBase::ERROR_TEXT;

    // Button aliases (AuthDialog naming -> DialogBase naming)
    constexpr uint32_t BUTTON_NORMAL = DialogBase::BUTTON_PRIMARY;
    constexpr uint32_t BUTTON_HOVER = DialogBase::BUTTON_PRIMARY_HOVER;
    constexpr uint32_t BUTTON_CANCEL = DialogBase::BUTTON_SECONDARY;
    constexpr uint32_t BUTTON_CANCEL_HOVER = DialogBase::BUTTON_SECONDARY_HOVER;
}

// OAuth dialog layout constants (GitHub Device Flow)
namespace OAuthDialog {
    // Dialog-specific values
    constexpr float WIDTH = 450.0f;
    constexpr float BUTTON_WIDTH = 140.0f;
    constexpr float FIELD_SPACING = 12.0f;           // Tighter spacing for OAuth flow

    // User code display box (OAuth-specific)
    constexpr float CODE_BOX_WIDTH = 200.0f;
    constexpr float CODE_BOX_HEIGHT = 56.0f;
    constexpr float CODE_FONT_SIZE = 24.0f;
    constexpr uint32_t CODE_BOX_BG = 0xFF1E1E1E;
    constexpr uint32_t CODE_BOX_BORDER = 0xFF0078D4;
    constexpr uint32_t CODE_TEXT = 0xFFFFFFFF;
    constexpr uint32_t STATUS_TEXT = 0xFF9D9D9D;

    // Aliases to DialogBase
    constexpr float PADDING = DialogBase::PADDING;
    constexpr float CORNER_RADIUS = DialogBase::CORNER_RADIUS;
    constexpr float BUTTON_HEIGHT = DialogBase::BUTTON_HEIGHT;
    constexpr float BUTTON_SPACING = DialogBase::BUTTON_SPACING;

    constexpr uint32_t OVERLAY_BG = DialogBase::OVERLAY_BG;
    constexpr uint32_t DIALOG_BG = DialogBase::DIALOG_BG;
    constexpr uint32_t BUTTON_PRIMARY = DialogBase::BUTTON_PRIMARY;
    constexpr uint32_t BUTTON_PRIMARY_HOVER = DialogBase::BUTTON_PRIMARY_HOVER;
    constexpr uint32_t BUTTON_SECONDARY = DialogBase::BUTTON_SECONDARY;
    constexpr uint32_t BUTTON_SECONDARY_HOVER = DialogBase::BUTTON_SECONDARY_HOVER;
    constexpr uint32_t ERROR_TEXT = DialogBase::ERROR_TEXT;
    constexpr uint32_t SUCCESS_TEXT = DialogBase::SUCCESS_TEXT;
}

// Branch creation dialog layout constants
namespace BranchDialog {
    // Dialog-specific values
    constexpr float WIDTH = 400.0f;
    constexpr float FIELD_SPACING = 20.0f;            // Branch dialog uses wider spacing
    constexpr float BUTTON_WIDTH = 100.0f;

    // Aliases to DialogBase (for backward compatibility with existing code)
    constexpr float CORNER_RADIUS = DialogBase::CORNER_RADIUS;
    constexpr float TITLE_HEIGHT = DialogBase::TITLE_HEIGHT;
    constexpr float CLOSE_BUTTON_SIZE = DialogBase::CLOSE_BUTTON_SIZE;

    constexpr float INPUT_HEIGHT = DialogBase::INPUT_HEIGHT;
    constexpr float INPUT_RADIUS = DialogBase::INPUT_RADIUS;
    constexpr float LABEL_HEIGHT = DialogBase::LABEL_HEIGHT;
    constexpr float LABEL_SPACING = DialogBase::LABEL_SPACING;
    constexpr float COMPACT_FIELD_SPACING = DialogBase::COMPACT_FIELD_SPACING;

    constexpr float CHECKBOX_SIZE = DialogBase::CHECKBOX_SIZE;
    constexpr float CHECKBOX_RADIUS = DialogBase::CHECKBOX_RADIUS;
    constexpr float CHECKBOX_LABEL_SPACING = DialogBase::CHECKBOX_LABEL_SPACING;

    constexpr float BUTTON_HEIGHT = DialogBase::BUTTON_HEIGHT;
    constexpr float BUTTON_RADIUS = DialogBase::BUTTON_RADIUS;
    constexpr float BUTTON_SPACING = DialogBase::BUTTON_SPACING;

    constexpr uint32_t OVERLAY_BG = DialogBase::OVERLAY_BG;
    constexpr uint32_t DIALOG_BG = DialogBase::DIALOG_BG;
    constexpr uint32_t INPUT_BG = DialogBase::INPUT_BG;
    constexpr uint32_t INPUT_BORDER = DialogBase::INPUT_BORDER;
    constexpr uint32_t INPUT_FOCUS = DialogBase::INPUT_FOCUS;
    constexpr uint32_t BUTTON_PRIMARY = DialogBase::BUTTON_PRIMARY;
    constexpr uint32_t BUTTON_PRIMARY_HOVER = DialogBase::BUTTON_PRIMARY_HOVER;
    constexpr uint32_t BUTTON_PRIMARY_DISABLED = DialogBase::BUTTON_PRIMARY_DISABLED;
    constexpr uint32_t BUTTON_SECONDARY = DialogBase::BUTTON_SECONDARY;
    constexpr uint32_t BUTTON_SECONDARY_HOVER = DialogBase::BUTTON_SECONDARY_HOVER;
    constexpr uint32_t ERROR_TEXT = DialogBase::ERROR_TEXT;
    constexpr uint32_t CHECKBOX_CHECK = DialogBase::CHECKBOX_CHECK;
}

// Command Palette layout constants
namespace CommandPalette {
    // Dimensions
    constexpr float WIDTH = 500.0f;
    constexpr float TOP_OFFSET = 80.0f;              // Distance from top of window
    constexpr float MAX_VISIBLE_ITEMS = 10;
    constexpr float CORNER_RADIUS = 8.0f;

    // Input row
    constexpr float INPUT_ROW_HEIGHT = 40.0f;
    constexpr float INPUT_PADDING = 12.0f;

    // Result items
    constexpr float ITEM_HEIGHT = 32.0f;
    constexpr float GROUP_HEADER_HEIGHT = 24.0f;
    constexpr float HINT_ROW_HEIGHT = 24.0f;
    constexpr float BATCH_ACTION_HEIGHT = 36.0f;  // Batch action bar height

    // Item layout
    constexpr float CHECKBOX_WIDTH = 20.0f;          // For multi-select mode
    constexpr float STATUS_WIDTH = 16.0f;            // File status indicator
    constexpr float ICON_SIZE = 16.0f;
    constexpr float ITEM_PADDING = 8.0f;
    constexpr float SHORTCUT_PADDING = 12.0f;
    constexpr float TEXT_LINE_HEIGHT = 14.0f;        // Single line text height

    // Badge dimensions
    constexpr float BADGE_HEIGHT = 18.0f;
    constexpr float BADGE_PADDING_H = 8.0f;
    constexpr float BADGE_RADIUS = 3.0f;

    // Selection
    constexpr float SELECTION_BORDER_WIDTH = 2.0f;

    // Colors
    constexpr uint32_t BG = 0xFF333337;              // Elevated background
    constexpr uint32_t BORDER = 0xFF3C3C3C;
    constexpr uint32_t INPUT_BG = 0xFF252526;        // Secondary background
    constexpr uint32_t INPUT_TEXT = 0xFFE0E0E0;
    constexpr uint32_t PLACEHOLDER_TEXT = 0xFF6D6D6D;
    constexpr uint32_t GROUP_HEADER_TEXT = 0xFF9D9D9D;
    constexpr uint32_t ITEM_TEXT = 0xFFE0E0E0;
    constexpr uint32_t MATCH_HIGHLIGHT = 0xFF4EC970;  // Success green for matched chars
    constexpr uint32_t SHORTCUT_TEXT = 0xFF9D9D9D;
    constexpr uint32_t SELECTED_BG = 0x330078D4;     // 20% accent
    constexpr uint32_t HOVER_BG = 0xFF3E3E42;
    constexpr uint32_t MULTI_SELECT_CHECK = 0xFF0078D4;
    constexpr uint32_t BATCH_ACTION_BG = 0xFF0E639C;    // Primary action background
    constexpr uint32_t BATCH_ACTION_TEXT = 0xFFFFFFFF;

    // Badge colors by mode
    constexpr uint32_t BADGE_COMMANDS = 0xFF0078D4;  // Accent
    constexpr uint32_t BADGE_FILES = 0xFFE0E0E0;     // Primary text
    constexpr uint32_t BADGE_BRANCHES = 0xFF6B9FED; // Blue (Branch1)
    constexpr uint32_t BADGE_COMMITS = 0xFFCCA700;   // Yellow (Warning)
    constexpr uint32_t BADGE_REMOTES = 0xFFC678DD;   // Purple (Branch2)

    // Animation
    constexpr float DISMISS_DURATION_MS = 80.0f;
    constexpr float RESULT_FADE_DURATION_MS = 50.0f;

    // Spring animation (single-layer model)
    constexpr float SPRING_PERIOD = 0.22f;            // 220ms - snappy but smooth
    constexpr float SPRING_DAMPING_RATIO = 0.65f;     // Moderate bounce (underdamped)
    constexpr float SPRING_START_SCALE = 0.92f;       // Start at 92% (subtle "pop")

    // Blur effect (subtle, suggests depth without obscuring content)
    constexpr float BLUR_RADIUS = 8.0f;               // Max blur during animation
    constexpr float OPACITY_DURATION = 0.15f;         // 150ms fade (snappy)

    // Search
    constexpr int COMMAND_DEBOUNCE_MS = 0;           // Instant
    constexpr int FILE_DEBOUNCE_MS = 16;
    constexpr int BRANCH_DEBOUNCE_MS = 16;
    constexpr int COMMIT_DEBOUNCE_MS = 100;
    constexpr int REMOTE_DEBOUNCE_MS = 0;

    // Max results per scope
    constexpr size_t MAX_COMMAND_RESULTS = 50;
    constexpr size_t MAX_FILE_RESULTS = 100;
    constexpr size_t MAX_BRANCH_RESULTS = 100;
    constexpr size_t MAX_COMMIT_RESULTS = 50;
    constexpr size_t MAX_REMOTE_RESULTS = 20;

    // Mixed mode result limits
    constexpr size_t MIXED_COMMANDS = 5;
    constexpr size_t MIXED_BRANCHES = 5;
    constexpr size_t MIXED_FILES = 10;
    constexpr size_t MIXED_COMMITS = 3;
}

// StashDialog layout constants
namespace StashDialog {
    // Dialog-specific values
    constexpr float WIDTH = 500.0f;
    constexpr float BUTTON_WIDTH = 120.0f;

    // File list (stash-specific)
    constexpr float FILE_LIST_HEIGHT = 180.0f;
    constexpr float FILE_ITEM_HEIGHT = 24.0f;
    constexpr float FILE_CHECKBOX_SIZE = 14.0f;
    constexpr float FILE_CHECKBOX_SPACING = 8.0f;
    constexpr uint32_t FILE_LIST_BG = 0xFF252526;
    constexpr uint32_t FILE_ITEM_HOVER = 0xFF2A2D2E;

    // Aliases to DialogBase
    constexpr float PADDING = DialogBase::PADDING;
    constexpr float CORNER_RADIUS = DialogBase::CORNER_RADIUS;
    constexpr float TITLE_HEIGHT = DialogBase::TITLE_HEIGHT;
    constexpr float CLOSE_BUTTON_SIZE = DialogBase::CLOSE_BUTTON_SIZE;

    constexpr float INPUT_HEIGHT = DialogBase::INPUT_HEIGHT;
    constexpr float INPUT_RADIUS = DialogBase::INPUT_RADIUS;
    constexpr float LABEL_HEIGHT = DialogBase::LABEL_HEIGHT;
    constexpr float LABEL_SPACING = DialogBase::LABEL_SPACING;
    constexpr float FIELD_SPACING = DialogBase::FIELD_SPACING;

    constexpr float CHECKBOX_SIZE = DialogBase::CHECKBOX_SIZE;
    constexpr float CHECKBOX_RADIUS = DialogBase::CHECKBOX_RADIUS;
    constexpr float CHECKBOX_LABEL_SPACING = DialogBase::CHECKBOX_LABEL_SPACING;

    constexpr float BUTTON_HEIGHT = DialogBase::BUTTON_HEIGHT;
    constexpr float BUTTON_RADIUS = DialogBase::BUTTON_RADIUS;
    constexpr float BUTTON_SPACING = DialogBase::BUTTON_SPACING;

    constexpr uint32_t OVERLAY_BG = DialogBase::OVERLAY_BG;
    constexpr uint32_t DIALOG_BG = DialogBase::DIALOG_BG;
    constexpr uint32_t INPUT_BG = DialogBase::INPUT_BG;
    constexpr uint32_t INPUT_BORDER = DialogBase::INPUT_BORDER;
    constexpr uint32_t INPUT_FOCUS = DialogBase::INPUT_FOCUS;
    constexpr uint32_t BUTTON_PRIMARY = DialogBase::BUTTON_PRIMARY;
    constexpr uint32_t BUTTON_PRIMARY_HOVER = DialogBase::BUTTON_PRIMARY_HOVER;
    constexpr uint32_t BUTTON_PRIMARY_DISABLED = DialogBase::BUTTON_PRIMARY_DISABLED;
    constexpr uint32_t BUTTON_SECONDARY = DialogBase::BUTTON_SECONDARY;
    constexpr uint32_t BUTTON_SECONDARY_HOVER = DialogBase::BUTTON_SECONDARY_HOVER;
    constexpr uint32_t ERROR_TEXT = DialogBase::ERROR_TEXT;
    constexpr uint32_t CHECKBOX_CHECK = DialogBase::CHECKBOX_CHECK;
}

// CloneDialog layout constants
namespace CloneDialog {
    // Dialog-specific values
    constexpr float WIDTH = 520.0f;
    constexpr float BUTTON_WIDTH = 100.0f;

    // Browse button (clone-specific)
    constexpr float BROWSE_BUTTON_WIDTH = 90.0f;
    constexpr float INPUT_BUTTON_GAP = 8.0f;

    // Phase progress row layout
    constexpr float PHASE_ROW_HEIGHT = 44.0f;      // Height of each phase row
    constexpr float PHASE_ICON_SIZE = 16.0f;       // Icon size for status indicator
    constexpr float PHASE_BAR_HEIGHT = 4.0f;       // Thin progress bar height
    constexpr float PHASE_SPACING = 8.0f;          // Vertical spacing between rows
    constexpr float PHASE_LABEL_HEIGHT = 18.0f;    // Height for label text
    constexpr float PHASE_TIME_WIDTH = 50.0f;      // Width for elapsed time display

    // Phase progress colors
    constexpr uint32_t PHASE_COMPLETE_COLOR = 0xFF4CAF50;   // Green checkmark
    constexpr uint32_t PHASE_FAILED_COLOR = 0xFFF44336;     // Red X
    constexpr uint32_t PHASE_PENDING_COLOR = 0xFF666666;    // Gray for pending
    constexpr uint32_t PHASE_ACTIVE_COLOR = 0xFF2196F3;     // Blue for active

    // Aliases to DialogBase
    constexpr float PADDING = DialogBase::PADDING;
    constexpr float CORNER_RADIUS = DialogBase::CORNER_RADIUS;
    constexpr float TITLE_HEIGHT = DialogBase::TITLE_HEIGHT;
    constexpr float CLOSE_BUTTON_SIZE = DialogBase::CLOSE_BUTTON_SIZE;

    constexpr float INPUT_HEIGHT = DialogBase::INPUT_HEIGHT;
    constexpr float INPUT_RADIUS = DialogBase::INPUT_RADIUS;
    constexpr float LABEL_HEIGHT = DialogBase::LABEL_HEIGHT;
    constexpr float LABEL_SPACING = DialogBase::LABEL_SPACING;
    constexpr float FIELD_SPACING = DialogBase::FIELD_SPACING;

    constexpr float BUTTON_HEIGHT = DialogBase::BUTTON_HEIGHT;
    constexpr float BUTTON_RADIUS = DialogBase::BUTTON_RADIUS;
    constexpr float BUTTON_SPACING = DialogBase::BUTTON_SPACING;

    constexpr uint32_t OVERLAY_BG = DialogBase::OVERLAY_BG;
    constexpr uint32_t DIALOG_BG = DialogBase::DIALOG_BG;
    constexpr uint32_t INPUT_BG = DialogBase::INPUT_BG;
    constexpr uint32_t INPUT_BORDER = DialogBase::INPUT_BORDER;
    constexpr uint32_t INPUT_FOCUS = DialogBase::INPUT_FOCUS;
    constexpr uint32_t BUTTON_PRIMARY = DialogBase::BUTTON_PRIMARY;
    constexpr uint32_t BUTTON_PRIMARY_HOVER = DialogBase::BUTTON_PRIMARY_HOVER;
    constexpr uint32_t BUTTON_PRIMARY_DISABLED = DialogBase::BUTTON_PRIMARY_DISABLED;
    constexpr uint32_t BUTTON_SECONDARY = DialogBase::BUTTON_SECONDARY;
    constexpr uint32_t BUTTON_SECONDARY_HOVER = DialogBase::BUTTON_SECONDARY_HOVER;
    constexpr uint32_t ERROR_TEXT = DialogBase::ERROR_TEXT;
    constexpr uint32_t SUCCESS_TEXT = DialogBase::SUCCESS_TEXT;
}

// Context menu layout constants
namespace ContextMenu {
    // Dimensions
    constexpr float ITEM_HEIGHT = 26.0f;
    constexpr float SEPARATOR_HEIGHT = 9.0f;       // 1px line + 4px padding each side
    constexpr float PADDING_H = 12.0f;             // Horizontal padding
    constexpr float PADDING_V = 4.0f;              // Vertical padding (top/bottom of menu)
    constexpr float ICON_SIZE = 16.0f;
    constexpr float ICON_SPACING = 8.0f;           // Space between icon and text
    constexpr float ICON_COLUMN_WIDTH = 24.0f;     // Reserved width for icon area
    constexpr float SHORTCUT_SPACING = 24.0f;      // Min space between label and shortcut
    constexpr float CHEVRON_SIZE = 8.0f;           // Submenu arrow size
    constexpr float MIN_WIDTH = 160.0f;
    constexpr float MAX_WIDTH = 320.0f;
    constexpr float CORNER_RADIUS = 4.0f;
    constexpr float CHECKMARK_SIZE = 12.0f;
    constexpr float SEPARATOR_INSET = 8.0f;        // Indent from left edge
    constexpr float HOVER_INSET = 4.0f;            // Inset from menu edge for hover highlight
    constexpr float HOVER_CORNER_RADIUS = 3.0f;    // Corner radius for hover highlight

    // Submenu behavior
    constexpr uint32_t SUBMENU_DELAY_MS = 200;     // Delay before opening submenu on hover
    constexpr float SUBMENU_OFFSET_X = -4.0f;      // Overlap with parent menu

    // Colors
    constexpr uint32_t BG = 0xFF252526;
    constexpr uint32_t BORDER = 0xFF3C3C3C;
    constexpr uint32_t HOVER_BG = 0xFF2D4F7C;      // Accent-tinted hover
    constexpr uint32_t TEXT = 0xFFE0E0E0;
    constexpr uint32_t TEXT_DISABLED = 0xFF6D6D6D;
    constexpr uint32_t TEXT_SHORTCUT = 0xFF9D9D9D;
    constexpr uint32_t SEPARATOR = 0xFF3C3C3C;
    constexpr uint32_t CHECKMARK = 0xFF0078D4;

    // Animation
    constexpr float ANIM_CLOSE_DURATION_MS = 80.0f;   // Close fade duration

    // Spring animation constants (Windows Composition-style)
    constexpr float SPRING_PERIOD = 0.26f;            // 260ms per oscillation - slower/smoother
    constexpr float SPRING_DAMPING_RATIO = 0.65f;     // Moderate bounce (underdamped)
    constexpr float CONTENT_SLIDE_DISTANCE = 60.0f;   // Content slides down this many pixels
    constexpr float CONTENT_BLUR_RADIUS = 12.0f;      // Max blur radius for content (pixels)
}

namespace TextInputTheme {
    // Animation timing (milliseconds)
    constexpr float CURSOR_BLINK_PERIOD_MS = 1000.0f;   // Full blink cycle
    constexpr float CURSOR_FADE_MS = 150.0f;            // Fade in/out duration
    constexpr float CURSOR_MOVE_MS = 40.0f;             // Cursor position animation (snappy)
    constexpr float SELECTION_ANIM_MS = 40.0f;          // Selection highlight animation
    constexpr float FOCUS_TRANSITION_MS = 150.0f;       // Border color transition

    // Multi-click detection
    constexpr float DOUBLE_CLICK_MS = 500.0f;           // Max time between clicks
    constexpr float DOUBLE_CLICK_DIST = 4.0f;           // Max movement between clicks (pixels)

    // Error animation
    constexpr float SHAKE_DURATION_MS = 400.0f;         // Total shake duration
    constexpr float SHAKE_AMPLITUDE = 4.0f;             // Max horizontal displacement
    constexpr float SHAKE_FREQUENCY = 20.0f;            // Oscillation frequency (Hz)

    // Scrolling
    constexpr float SCROLL_MARGIN = 20.0f;              // Keep cursor this far from edges

    // Colors
    constexpr uint32_t CURSOR_COLOR = 0xFFE0E0E0;       // Cursor line color
    constexpr uint32_t BORDER_ERROR = 0xFFF14C4C;       // Red border for validation errors
    constexpr uint32_t BORDER_HOVER = 0xFF5A5A5E;       // Lighter border on hover
}

// Helper to convert ARGB to D2D1_COLOR_F
inline D2D1_COLOR_F color_from_argb(uint32_t argb) {
    return D2D1::ColorF(
        ((argb >> 16) & 0xFF) / 255.0f,
        ((argb >> 8) & 0xFF) / 255.0f,
        (argb & 0xFF) / 255.0f,
        ((argb >> 24) & 0xFF) / 255.0f
    );
}

// Interpolate between two ARGB colors (for smooth animations)
inline uint32_t interpolate_color(uint32_t c1, uint32_t c2, float t) {
    t = fmaxf(0.0f, fminf(1.0f, t));
    int a1 = (c1 >> 24) & 0xFF, a2 = (c2 >> 24) & 0xFF;
    int r1 = (c1 >> 16) & 0xFF, r2 = (c2 >> 16) & 0xFF;
    int g1 = (c1 >> 8) & 0xFF,  g2 = (c2 >> 8) & 0xFF;
    int b1 = c1 & 0xFF,         b2 = c2 & 0xFF;
    uint8_t a = static_cast<uint8_t>(a1 + static_cast<int>((a2 - a1) * t));
    uint8_t r = static_cast<uint8_t>(r1 + static_cast<int>((r2 - r1) * t));
    uint8_t g = static_cast<uint8_t>(g1 + static_cast<int>((g2 - g1) * t));
    uint8_t b = static_cast<uint8_t>(b1 + static_cast<int>((b2 - b1) * t));
    return (a << 24) | (r << 16) | (g << 8) | b;
}
