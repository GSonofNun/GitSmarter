# GitSmarter UI Design Brief

## Current State
- Borderless window with custom title bar
- Dark theme (#1E1E1E background, #323233 title bar)
- Windows 11 rounded corners and shadow
- Native minimize/maximize/restore animations
- Custom fade+scale close animation

## Core Views to Design

### 1. Repository Selector / Welcome Screen
- Shown when no repo is open
- Recent repositories list
- "Open Repository" button
- "Clone Repository" option (future)

### 2. Main Layout
Typical git client layouts:
```
+------------------------------------------+
| Title Bar              [_][O][X]         |
+--------+---------------------------------+
| Sidebar|  Main Content Area              |
| (200px)|                                 |
|        |  - Commit history graph         |
| Branches|  - File diff view              |
| Remotes |  - Staging area                |
| Tags   |                                 |
| Stashes|                                 |
+--------+---------------------------------+
| Status Bar: branch name, sync status    |
+------------------------------------------+
```

### 3. Commit History View
- Commit graph (branching/merging visualization)
- Commit list: hash, author, date, message
- Consider: horizontal timeline vs vertical list?

### 4. File/Changes View
- Staged vs unstaged changes
- Diff viewer with syntax highlighting
- File tree or flat list?

### 5. Commit Detail View
- Full commit message
- Changed files list
- Diff for selected file

## Design Questions to Answer

### Visual Style
- **Minimal/Clean**: Lots of whitespace, simple icons (GitHub Desktop style)
- **Information Dense**: Compact, shows more at once (SourceTree/GitKraken style)
- **Novel/Unique**: Something different? Graph-centric? Timeline-based?

### Graph Visualization
- Traditional: Colored lines connecting commits vertically
- Subway map: Metro/transit style
- Radial: Circular/spiral timeline
- Minimal: No graph, just list with branch labels

### Color Usage
- Monochrome with single accent color?
- Color-coded branches?
- Semantic colors (green=added, red=removed, blue=modified)?

### Typography
- Currently using Segoe UI 12pt
- Consider: Monospace for hashes/code, variable for UI?

## Current Theme Colors (ui_theme.h)
```
Background Primary:  #1E1E1E
Background Secondary: #252526
Background Tertiary:  #2D2D30
Hover:               #3E3E42
Text Primary:        #E0E0E0
Text Secondary:      #9D9D9D
Text Muted:          #6D6D6D
Accent:              #0078D4 (Windows blue)
Close Hover:         #E81123 (Red)
```

## Inspiration to Consider
- **GitHub Desktop**: Clean, simple, beginner-friendly
- **GitKraken**: Colorful, graph-focused, feature-rich
- **SourceTree**: Dense, powerful, traditional
- **Fork**: Fast, clean, good balance
- **Sublime Merge**: Minimal, keyboard-driven, fast
- **lazygit**: Terminal UI, keyboard-centric

## Constraints
- Direct2D rendering (vector graphics, no HTML/CSS)
- Windows-only (can use Windows conventions)
- Performance-focused (should handle large repos smoothly)
- Single executable (no external dependencies)
