// text_document.cpp - TextDocument implementation

#include "text_document.h"
#include <cstring>
#include <cstdlib>
#include <Windows.h>  // For WideCharToMultiByte

namespace CodeEditor {

// ============================================================================
// File Type Detection (mirrors logic in git_diff.cpp)
// ============================================================================

static FileType detect_file_type_from_path(const char* path) {
    if (!path) return FileType::Unknown;

    // Find extension
    const char* ext = strrchr(path, '.');
    if (!ext || !ext[1]) return FileType::Unknown;
    ext++;  // Skip dot

    // Fast path: check length first (most extensions are 1-4 chars)
    size_t ext_len = strlen(ext);
    if (ext_len > 4) return FileType::Unknown;

    // Case-insensitive comparison helper
    auto to_lower = [](char c) -> char {
        return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
    };

    char lower_ext[8] = {};
    for (size_t i = 0; i < ext_len && i < 7; i++) {
        lower_ext[i] = to_lower(ext[i]);
    }

    // C/C++ source files
    if (strcmp(lower_ext, "cpp") == 0 ||
        strcmp(lower_ext, "c") == 0 ||
        strcmp(lower_ext, "cc") == 0 ||
        strcmp(lower_ext, "cxx") == 0) {
        return FileType::Cpp;
    }

    // Header files
    if (strcmp(lower_ext, "h") == 0 ||
        strcmp(lower_ext, "hpp") == 0 ||
        strcmp(lower_ext, "hxx") == 0) {
        return FileType::Header;
    }

    // C# files
    if (strcmp(lower_ext, "cs") == 0) {
        return FileType::CSharp;
    }

    // XAML files
    if (strcmp(lower_ext, "xaml") == 0) {
        return FileType::Xaml;
    }

    // Markdown files
    if (strcmp(lower_ext, "md") == 0) {
        return FileType::Markdown;
    }

    return FileType::Unknown;
}

// ============================================================================
// Construction / Destruction
// ============================================================================

TextDocument::TextDocument()
    : dirty(false)
    , line_ending(LineEnding::LF)
    , file_type_(FileType::Unknown)
    , source_path_(nullptr)
    , clean_undo_pos(0)
{
}

TextDocument::~TextDocument() {
    // PieceTable handles its own cleanup
    if (source_path_) {
        delete[] source_path_;
        source_path_ = nullptr;
    }
}

TextDocument* TextDocument::from_string(const char* utf8_text) {
    return from_string(utf8_text, utf8_text ? strlen(utf8_text) : 0);
}

TextDocument* TextDocument::from_string(const char* utf8_text, size_t len) {
    TextDocument* doc = new TextDocument();

    if (utf8_text && len > 0) {
        doc->piece_table.init_from_string(utf8_text, len);

        // Detect line ending from content
        bool found_cr = false;
        for (size_t i = 0; i < len; i++) {
            if (utf8_text[i] == '\r') {
                found_cr = true;
                if (i + 1 < len && utf8_text[i + 1] == '\n') {
                    doc->line_ending = LineEnding::CRLF;
                    break;
                }
            } else if (utf8_text[i] == '\n') {
                if (found_cr) {
                    doc->line_ending = LineEnding::CR;
                } else {
                    doc->line_ending = LineEnding::LF;
                }
                break;
            }
        }
        if (found_cr && doc->line_ending == LineEnding::LF) {
            // Only \r found, no \n
            doc->line_ending = LineEnding::CR;
        }
    }

    return doc;
}

TextDocument* TextDocument::from_file(const wchar_t* path) {
    if (!path) return nullptr;

    // Convert wide path to UTF-8 for storage
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
    if (utf8_len <= 0) return nullptr;

    char* utf8_path = new (std::nothrow) char[utf8_len];
    if (!utf8_path) return nullptr;

    WideCharToMultiByte(CP_UTF8, 0, path, -1, utf8_path, utf8_len, nullptr, nullptr);

    TextDocument* doc = from_file(utf8_path);
    delete[] utf8_path;
    return doc;
}

TextDocument* TextDocument::from_file(const char* utf8_path) {
    if (!utf8_path) return nullptr;

    // Read file content
    FILE* file = nullptr;
    if (fopen_s(&file, utf8_path, "rb") != 0 || !file) {
        return nullptr;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0) {
        fclose(file);
        return nullptr;
    }

    // Read content
    char* content = new (std::nothrow) char[file_size + 1];
    if (!content) {
        fclose(file);
        return nullptr;
    }

    size_t read = fread(content, 1, file_size, file);
    fclose(file);
    content[read] = '\0';

    // Create document from content
    TextDocument* doc = from_string(content, read);
    delete[] content;

    if (doc) {
        // Store source path and detect file type
        doc->set_source_path(utf8_path);
    }

    return doc;
}

void TextDocument::set_source_path(const char* path) {
    // Free existing path
    if (source_path_) {
        delete[] source_path_;
        source_path_ = nullptr;
    }

    if (path) {
        // Copy new path
        size_t len = strlen(path);
        source_path_ = new (std::nothrow) char[len + 1];
        if (source_path_) {
            strcpy_s(source_path_, len + 1, path);
        }
    }

    // Re-detect file type
    detect_file_type();
}

void TextDocument::detect_file_type() {
    file_type_ = detect_file_type_from_path(source_path_);
}

// ============================================================================
// Dirty State
// ============================================================================

void TextDocument::mark_dirty() {
    dirty = true;
}

void TextDocument::mark_clean() {
    dirty = false;
    // Remember current undo position as "clean" state
    clean_undo_pos = piece_table.undo_position();
}



// ============================================================================
// Edit Operations
// ============================================================================

bool TextDocument::insert(size_t pos, const char* text, size_t len) {
    bool result = piece_table.insert(pos, text, len);
    if (result && len > 0) {
        mark_dirty();
    }
    return result;
}

bool TextDocument::delete_range(size_t pos, size_t len) {
    bool result = piece_table.delete_range(pos, len);
    if (result && len > 0) {
        mark_dirty();
    }
    return result;
}

void TextDocument::push_undo() {
    piece_table.push_undo();
}

bool TextDocument::undo() {
    bool result = piece_table.undo();
    if (result) {
        if (piece_table.undo_position() == clean_undo_pos) {
            dirty = false;
        } else {
            dirty = true;
        }
    }
    return result;
}

bool TextDocument::redo() {
    bool result = piece_table.redo();
    if (result) {
        dirty = (piece_table.undo_position() != clean_undo_pos);
    }
    return result;
}

} // namespace CodeEditor
