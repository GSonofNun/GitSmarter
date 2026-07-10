// text_document.h - Document lifecycle management for the code editor
//
// TextDocument wraps a PieceTable with file lifecycle (dirty tracking,
// future: file I/O, encoding detection, line ending handling).

#pragma once

#include "piece_table.h"
#include "app.h"  // For FileType enum

namespace CodeEditor {

// ============================================================================
// Line Ending Types
// ============================================================================

enum class LineEnding {
    LF,     // Unix (\n)
    CRLF,   // Windows (\r\n)
    CR      // Old Mac (\r)
};

// ============================================================================
// TextDocument Class
// ============================================================================

class TextDocument {
public:
    // Factory methods
    static TextDocument* from_string(const char* utf8_text);
    static TextDocument* from_string(const char* utf8_text, size_t len);

    // File-based creation with type detection
    static TextDocument* from_file(const wchar_t* path);
    static TextDocument* from_file(const char* utf8_path);

    ~TextDocument();

    // ========================================================================
    // Content Access
    // ========================================================================

    PieceTable* content() { return &piece_table; }
    const PieceTable* content() const { return &piece_table; }

    // Convenience accessors
    size_t length() const { return piece_table.total_length(); }
    bool empty() const { return piece_table.empty(); }
    size_t line_count() const { return piece_table.count_lines(); }

    // ========================================================================
    // Dirty State
    // ========================================================================

    bool is_dirty() const { return dirty; }
    void mark_dirty();
    void mark_clean();

    // ========================================================================
    // Document Properties
    // ========================================================================

    LineEnding get_line_ending() const { return line_ending; }
    void set_line_ending(LineEnding le) { line_ending = le; }

    FileType file_type() const { return file_type_; }
    void set_file_type(FileType ft) { file_type_ = ft; }

    // Source path for file type detection
    const char* source_path() const { return source_path_; }
    void set_source_path(const char* path);

    // ========================================================================
    // Edit Operations (convenience wrappers that update dirty state)
    // ========================================================================

    bool insert(size_t pos, const char* text, size_t len);
    bool delete_range(size_t pos, size_t len);

    // Undo/redo with dirty state tracking
    bool undo();
    bool redo();
    void push_undo();

    bool can_undo() const { return piece_table.can_undo(); }
    bool can_redo() const { return piece_table.can_redo(); }

private:
    TextDocument();

    PieceTable piece_table;
    bool dirty;
    LineEnding line_ending;
    FileType file_type_;

    // Source path for file type detection (stored as UTF-8)
    char* source_path_;

    // Track clean state position in undo stack for accurate dirty detection
    size_t clean_undo_pos;

    // Detect file type from source_path_ extension
    void detect_file_type();
};

} // namespace CodeEditor
